#pragma once
#include "augs/network/network_types.h"
#include "augs/misc/constant_size_string.h"
#include "application/setups/client/client_vars.h"
#include "game/per_character_input_settings.h"

struct public_client_settings {
	// GEN INTROSPECTOR struct public_client_settings
	per_character_input_settings character_input;
	// END GEN INTROSPECTOR

	bool operator==(const public_client_settings&) const = default;
};

using rcon_password_type = augs::constant_size_string<max_rcon_password_length_v>;

/* 
	NOTE: this struct, as well as many other important network structs,
   	is serialized manually in net_serialize.h!!!

	It's not enough to just add fields here.
	You have to apply properties in client_setup::apply as well as in the relevant network serialize function.
*/

struct requested_client_settings {
	static constexpr bool force_read_field_by_field = true;

	// GEN INTROSPECTOR struct requested_client_settings
	client_nickname_type chosen_nickname;
	rcon_password_type rcon_password;

	public_client_settings public_settings;
	client_net_vars net;

	bool suppress_webhooks = false;
	// END GEN INTROSPECTOR

	bool operator==(const requested_client_settings&) const = default;
};
