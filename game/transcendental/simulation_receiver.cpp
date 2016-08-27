#include "simulation_receiver.h"
#include "game/transcendental/cosmic_delta.h"

void simulation_receiver::read_entropy_for_next_step(augs::stream& in) {
	jitter_buffer.acquire_new_command(simulation_exchange::read_entropy_for_next_step(in));
}

void simulation_receiver::read_entropy_with_heartbeat_for_next_step(augs::stream& in) {
	jitter_buffer.acquire_new_command(simulation_exchange::read_entropy_with_heartbeat_for_next_step(in));
}

simulation_receiver::unpacked_steps simulation_receiver::unpack_deterministic_steps(cosmos& properly_stepped_cosmos, cosmos& extrapolated_cosmos, cosmos& last_delta_unpacked) {
	unpacked_steps result;

	auto new_commands = jitter_buffer.unpack_commands_once();

	bool unpacked_successfully = new_commands.size() > 0;

	if (unpacked_successfully) {
		result.use_extrapolated_cosmos = false;

		for (size_t i = 0; i < new_commands.size(); ++i) {
			if (new_commands[i].step_type == packaged_step::type::NEW_ENTROPY_WITH_HEARTBEAT) {
				cosmic_delta::decode(last_delta_unpacked, new_commands[i].delta);
				properly_stepped_cosmos = last_delta_unpacked;

				result.steps_for_proper_cosmos.clear();
			}
			else
				ensure(new_commands[i].step_type == packaged_step::type::NEW_ENTROPY);

			result.steps_for_proper_cosmos.emplace_back(new_commands[i].entropy);
		}
	}
	else {
		if (jitter_buffer.get_steps_extrapolated() == 1)
			extrapolated_cosmos = properly_stepped_cosmos;

		result.use_extrapolated_cosmos = true;
	}

	return std::move(result);
}

bool simulation_receiver::unpacked_steps::has_next_entropy() const {
	return steps_for_proper_cosmos.size() > 0;
}

cosmic_entropy simulation_receiver::unpacked_steps::unpack_next_entropy(const cosmos& guid_mapper) {
	ensure(has_next_entropy());

	cosmic_entropy next = std::move(steps_for_proper_cosmos.front());
	steps_for_proper_cosmos.erase(steps_for_proper_cosmos.begin());

	return map_guids_to_ids(next, guid_mapper);
}