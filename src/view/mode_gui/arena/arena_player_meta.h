#pragma once
#include <array>
#include "augs/network/network_types.h"
#include "application/network/requested_client_settings.h"

struct arena_player_avatar_payload {
	std::vector<std::byte> png_bytes;
};

struct arena_player_network_stats {
	int ping = 0;
};

struct arena_player_meta {
	arena_player_avatar_payload avatar;
	arena_player_network_stats stats;
	public_client_settings public_settings;

	void clear() {
		avatar.png_bytes.clear();
	}
};

using arena_player_metas = std::array<arena_player_meta, max_mode_players_v>;
