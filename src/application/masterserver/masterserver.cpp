#if PLATFORM_UNIX
#include <csignal>
#endif
#include <shared_mutex>
#include "application/masterserver/masterserver.h"
#include "3rdparty/include_httplib.h"
#include "augs/log.h"

#include "application/config_lua_table.h"
#include "augs/network/netcode_sockets.h"
#include "augs/readwrite/pointer_to_buffer.h"
#include "augs/readwrite/memory_stream.h"
#include "application/masterserver/server_heartbeat.h"
#include "augs/templates/thread_templates.h"
#include "augs/misc/time_utils.h"
#include "augs/readwrite/byte_readwrite.h"
#include "augs/network/netcode_socket_raii.h"
#include "application/network/resolve_address.h"
#include "augs/readwrite/byte_file.h"
#include "augs/readwrite/to_bytes.h"
#include "application/masterserver/masterserver_requests.h"
#include "application/masterserver/netcode_address_hash.h"
#include "augs/string/parse_url.h"
#include "application/detail_file_paths.h"
#include "application/setups/server/webhooks.h"

std::string ToString(const netcode_address_t&);

#if PLATFORM_UNIX
extern std::atomic<int> signal_status;
#endif

#define LOG_MASTERSERVER 1

template <class... Args>
void MSR_LOG(Args&&... args) {
#if LOG_MASTERSERVER
	LOG(std::forward<Args>(args)...);
#else
	((void)args, ...);
#endif
}

#if LOG_MASTERSERVER
#define MSR_LOG_NVPS LOG_NVPS
#else
#define MSR_LOG_NVPS MSR_LOG
#endif

struct masterserver_client_meta {
	double appeared_when;

	masterserver_client_meta() {
		appeared_when = augs::date_time::secs_since_epoch();
	}
};

struct masterserver_client {
	double time_of_last_heartbeat;

	masterserver_client_meta meta;
	server_heartbeat last_heartbeat;
};

bool operator==(const netcode_address_t& a, const netcode_address_t& b) {
	auto aa = a;
	auto bb = b;
	return 1 == netcode_address_equal(&aa, &bb);
}

bool operator!=(const netcode_address_t& a, const netcode_address_t& b) {
	auto aa = a;
	auto bb = b;
	return 0 == netcode_address_equal(&aa, &bb);
}

bool host_equal(const netcode_address_t& a, const netcode_address_t& b) {
	auto aa = a;
	auto bb = b;
	aa.port = 0;
	bb.port = 0;

	return 1 == netcode_address_equal(&aa, &bb);
}

double yojimbo_time();
void yojimbo_sleep(double);

void perform_masterserver(const config_lua_table& cfg) try {
	using namespace httplib;

	const auto& settings = cfg.masterserver;

	auto udp_command_sockets = std::vector<netcode_socket_raii>();

	const auto num_sockets = settings.num_udp_command_ports;

	LOG("Creating %x masterserver sockets for UDP commands.", num_sockets);

	for (int i = 0; i < num_sockets; ++i) {
		const auto new_port = settings.first_udp_command_port + i;

		if (const auto new_local_address = to_netcode_addr(settings.ip, new_port)) {
			udp_command_sockets.emplace_back(*new_local_address);
		}
		else {
			LOG("There was a problem binding masterserver to %x:%x! Quitting.", settings.ip, new_port);
			return;
		}

		LOG("Created masterserver socket at: %x", ::ToString(udp_command_sockets.back().socket.address));
	}

	auto find_socket_by_port = [&](const port_type port) -> netcode_socket_raii* {
		const auto first = settings.first_udp_command_port;
		const auto index = static_cast<std::size_t>(port - first);

		if (index < udp_command_sockets.size()) {
			return std::addressof(udp_command_sockets[index]);
		}

		return nullptr;
	};

	std::unordered_map<netcode_address_t, masterserver_client> server_list;

	std::vector<std::byte> serialized_list;
	std::shared_mutex serialized_list_mutex;

	httplib::Server http;

	const auto masterserver_dump_path = augs::path_type(USER_FILES_DIR) / "masterserver.dump";

	auto reserialize_list = [&]() {
		MSR_LOG("Reserializing the server list.");

		std::lock_guard<std::shared_mutex> lock(serialized_list_mutex);

		serialized_list.clear();

		auto ss = augs::ref_memory_stream(serialized_list);

		for (auto& server : server_list) {
			const auto address = server.first;

			augs::write_bytes(ss, address);
			augs::write_bytes(ss, server.second.meta.appeared_when);
			augs::write_bytes(ss, server.second.last_heartbeat);
		}
	};

	auto dump_server_list_to_file = [&]() {
		const auto n = server_list.size();

		if (n > 0) {
			LOG("Saving %x servers to %x", n, masterserver_dump_path);
			augs::bytes_to_file(std::as_const(serialized_list), masterserver_dump_path);
		}
		else {
			LOG("The server list is empty: deleting the dump file.");
			augs::remove_file(masterserver_dump_path);
		}
	};

	auto load_server_list_from_file = [&]() {
		try {
			auto source = augs::open_binary_input_stream(masterserver_dump_path);

			LOG("%x found.\nLoading the server list from file.", masterserver_dump_path);

			const auto current_time = yojimbo_time();

			while (source.peek() != EOF) {
				const auto address = augs::read_bytes<netcode_address_t>(source);

				masterserver_client entry;
				augs::read_bytes(source, entry.meta.appeared_when);
				augs::read_bytes(source, entry.last_heartbeat);

				entry.time_of_last_heartbeat = current_time;

				server_list.try_emplace(address, std::move(entry));
			}

			reserialize_list();
		}
		catch (const augs::file_open_error& err) {
			LOG("Could not load the server list file: %x.\nStarting from an empty server list. Details:\n%x", masterserver_dump_path, err.what());

			server_list.clear();
		}
		catch (const augs::stream_read_error& err) {
			LOG("Failed to read the server list from file: %x.\nStarting from an empty server list. Details:\n%x", masterserver_dump_path, err.what());

			server_list.clear();
		}
	};

	load_server_list_from_file();

	auto make_list_streamer_lambda = [&]() {
		return [data=serialized_list](uint64_t offset, uint64_t length, DataSink& sink) {
			return sink.write(reinterpret_cast<const char*>(&data[offset]), length);
		};
	};

	auto remove_from_list = [&](const auto& by_external_addr) {
		server_list.erase(by_external_addr);
		reserialize_list();
	};

	auto define_http_server = [&]() {
		http.Get("/server_list_binary", [&](const Request&, Response& res) {
			std::shared_lock<std::shared_mutex> lock(serialized_list_mutex);

			if (serialized_list.size() > 0) {
				MSR_LOG("List request arrived. Sending list of size: %x", serialized_list.size());

				res.set_content_provider(
					serialized_list.size(),
					"application/octet-stream",
					make_list_streamer_lambda()
				);
			}
		});
	};

	define_http_server();

	LOG("Hosting a server list at port: %x (HTTP)", settings.server_list_port);

	auto listening_thread = std::thread([&http, in_settings=settings]() {
		http.listen(in_settings.ip.c_str(), in_settings.server_list_port);
		LOG("The HTTP listening thread has quit.");
	});

	uint8_t packet_buffer[NETCODE_MAX_PACKET_BYTES];

	struct webhook_job {
		std::unique_ptr<std::future<std::string>> job;
	};

	std::vector<webhook_job> pending_jobs;

	auto finalize_webhook_jobs = [&]() {
		auto finalize = [&](auto& webhook_job) {
			return is_ready(*webhook_job.job);
		};

		erase_if(pending_jobs, finalize);
	};

	auto push_webhook_job = [&](auto&& f) {
		auto ptr = std::make_unique<std::future<std::string>>(
			std::async(std::launch::async, std::forward<decltype(f)>(f))
		);

		pending_jobs.emplace_back(webhook_job{ std::move(ptr) });
	};

	auto push_new_server_webhook = [&](const netcode_address_t& from, const server_heartbeat& data) {
		const auto ip_str = ::ToString(from);

		auto webhook_url = parsed_url(cfg.private_server.webhook_url);

		push_webhook_job(
			[ip_str, data, webhook_url]() -> std::string {
				const auto ca_path = CA_CERT_PATH;
				http_client_type http_client(webhook_url.host);

#if BUILD_OPENSSL
				http_client.set_ca_cert_path(ca_path.c_str());
				http_client.enable_server_certificate_verification(true);
#endif
				http_client.set_follow_location(true);
				http_client.set_read_timeout(5);
				http_client.set_write_timeout(5);

				const auto game_mode_name = data.game_mode.dispatch([](auto d) {
					using D = decltype(d);
					return format_field_name(get_type_name<D>());
				});

				auto items = discord_webhooks::form_new_community_server(
					"Server list",
					data.server_name,
					ip_str,
					data.current_arena,
					game_mode_name,
					data.max_online,
					nat_type_to_string(data.nat.type)
				);

				http_client.Post(webhook_url.location.c_str(), items);

				return "";
			}
		);
	};

	while (true) {
#if PLATFORM_UNIX
		if (signal_status != 0) {
			const auto sig = signal_status.load();

			LOG("%x received.", strsignal(sig));

			if(
				sig == SIGINT
				|| sig == SIGSTOP
				|| sig == SIGTERM
			) {
				LOG("Gracefully shutting down.");
				break;
			}
		}
#endif

		const auto current_time = yojimbo_time();

		finalize_webhook_jobs();

		auto process_socket_messages = [&](auto& socket) {
			netcode_address_t from;
			const auto packet_bytes = netcode_socket_receive_packet(&socket, &from, packet_buffer, NETCODE_MAX_PACKET_BYTES);

			if (packet_bytes < 1) {
				return;
			}

			MSR_LOG("Received packet bytes: %x", packet_bytes);

			try {
				auto send_to_with_socket = [&](auto which_socket, auto to, const auto& typed_response) {
					auto bytes = augs::to_bytes(masterserver_response(typed_response));
					netcode_socket_send_packet(&which_socket, &to, bytes.data(), bytes.size());
				};

				auto send_to = [&](auto to, const auto& typed_response) {
					send_to_with_socket(socket, to, typed_response);
				};

				auto send_back = [&](const auto& typed_response) {
					send_to(from, typed_response);
				};

				auto send_to_gameserver = [&](const auto& typed_command, netcode_address_t server_address) {
					auto& socket_readable_by_gameserver_address = udp_command_sockets[0];

					auto bytes = make_gameserver_command_bytes(typed_command);
					netcode_socket_send_packet(&socket_readable_by_gameserver_address.socket, &server_address, bytes.data(), bytes.size());
				};

				auto handle = [&](const auto& typed_request) {
					using R = remove_cref<decltype(typed_request)>;

					if constexpr(std::is_same_v<R, masterserver_in::goodbye>) {
						if (const auto entry = mapped_or_nullptr(server_list, from)) {
							LOG("The server at %x (%x) has sent a goodbye.", ::ToString(from), entry->last_heartbeat.server_name);
							remove_from_list(from);
						}
					}
					else if constexpr(std::is_same_v<R, masterserver_in::heartbeat>) {
						if (typed_request.is_valid()) {
							auto it = server_list.try_emplace(from);

							const bool is_new_server = it.second;
							auto& server_entry = (*it.first).second;

							const auto heartbeat_before = server_entry.last_heartbeat;
							server_entry.last_heartbeat = typed_request;
							server_entry.time_of_last_heartbeat = current_time;

							const bool heartbeats_mismatch = heartbeat_before != server_entry.last_heartbeat;

							if (is_new_server) {
								if (!typed_request.suppress_new_community_server_webhook) {
									push_new_server_webhook(from, typed_request);
								}
							}

							MSR_LOG_NVPS(is_new_server, heartbeats_mismatch);

							if (is_new_server || heartbeats_mismatch) {
								reserialize_list();
							}
						}
					}
					else if constexpr(std::is_same_v<R, masterserver_in::tell_me_my_address>) {
						masterserver_out::tell_me_my_address response;
						response.address = from;
						response.session_timestamp = typed_request.session_timestamp;

						MSR_LOG("TELL_ME_MY_ADDRESS arrived from: %x", ::ToString(from));
						send_back(response);
					}
					else if constexpr(std::is_same_v<R, masterserver_in::stun_result_info>) {
						/* Just relay this */

						const auto session_guid = typed_request.session_guid;

						auto response = masterserver_out::stun_result_info();
						response.session_guid = session_guid;

						{
							const auto resolved_port = typed_request.resolved_external_port;

							if (resolved_port == 0) {
								/* Let masterserver resolve it */
								const auto masterserver_visible_port = from.port;
								response.resolved_external_port = masterserver_visible_port;
							}
							else {
								/* Result of server STUNning on its own */
								response.resolved_external_port = resolved_port;
							}
						}

						const auto& recipient = typed_request.client_origin;
						const auto client_used_probe = recipient.probe;

						MSR_LOG("Received stun_result_info from a gameserver (session guid: %f, resolved port: %x). Relaying this to: %x (original probe port: %x)", session_guid, response.resolved_external_port, ::ToString(recipient.address), client_used_probe);

						if (const auto socket = find_socket_by_port(client_used_probe)) {
							send_to_with_socket(socket->socket, recipient.address, response);
						}
						else {
							MSR_LOG("Invalid client_used_probe: %x", client_used_probe); 
						}
					}
					else if constexpr(std::is_same_v<R, masterserver_in::nat_traversal_step>) {
						auto punched_server = typed_request.target_server;

						const auto address_string = ::ToString(punched_server);
						MSR_LOG("A request arrived from %x to traverse %x", ::ToString(from), address_string);

						const bool should_send_request = [&]() {
							if (address_string == "NONE" || punched_server.type == NETCODE_ADDRESS_NONE) {
								MSR_LOG("The target address was invalid. Dropping the request.");
								return false;
							}

							if (const auto entry = mapped_or_nullptr(server_list, punched_server)) {
								MSR_LOG("Found the requested server.");

								const bool is_behind_nat = entry->last_heartbeat.is_behind_nat();

								if (is_behind_nat) {
									MSR_LOG("The requested server is behind NAT. Deciding to send the request.");
									return true;
								}

								MSR_LOG("The requested server is not behind NAT. Ignoring the request.");
								return false;
							}

							MSR_LOG("The requested server was not found.");
							return false;
						}();

						if (should_send_request) {
							auto step_request = masterserver_out::nat_traversal_step();

							step_request.client_origin = { from, socket.address.port };
							step_request.payload = typed_request.payload;

							send_to_gameserver(step_request, punched_server);
						}
					}
				};

				const auto request = augs::from_bytes<masterserver_request>(packet_buffer, packet_bytes);
				std::visit(handle, request);
			}
			catch (...) {
				if (const auto entry = mapped_or_nullptr(server_list, from)) {
					LOG("The server at %x (%x) has sent invalid data.", ::ToString(from), entry->last_heartbeat.server_name);
					remove_from_list(from);
				}
			}
		};

		for (auto& s : udp_command_sockets) {
			process_socket_messages(s.socket);
		}

		const auto timeout_secs = settings.server_entry_timeout_secs;

		auto process_entry_logic = [&](auto& server_entry) {
			(void)server_entry;
		};

		auto erase_if_dead = [&](auto& server_entry) {
			const bool timed_out = current_time - server_entry.second.time_of_last_heartbeat >= timeout_secs;

			if (timed_out) {
				LOG("The server at %x (%x) has timed out.", ::ToString(server_entry.first), server_entry.second.last_heartbeat.server_name);
			}
			else {
				process_entry_logic(server_entry);
			}

			return timed_out;
		};

		const auto previous_size = server_list.size();
		erase_if(server_list, erase_if_dead);

		if (previous_size != server_list.size()) {
			reserialize_list();
		}

		yojimbo_sleep(settings.sleep_ms / 1000);
	}

	LOG("Stopping the HTTP masterserver.");
	http.stop();
	LOG("Joining the HTTP listening thread.");
	listening_thread.join();

	dump_server_list_to_file();
}
catch (const netcode_socket_raii_error& err) {
	LOG(err.what());
}
