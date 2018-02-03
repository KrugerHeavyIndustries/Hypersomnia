#pragma once
#include "game/container_sizes.h"

namespace assets {
	enum class game_image_id {
		// GEN INTROSPECTOR enum class assets::game_image_id
		INVALID,

#if BUILD_TEST_SCENES
		BLANK,
test_plain_sprited_bodys
		CRATE,

		TRUCK_INSIDE,
		TRUCK_FRONT,

		JMIX114,
		
		TEST_CROSSHAIR,

		STANDARD_HEAD,

		SMOKE_1,
		SMOKE_2,
		SMOKE_3,
		SMOKE_4,
		SMOKE_5,
		SMOKE_6,

		PIXEL_THUNDER_1,
		PIXEL_THUNDER_2,
		PIXEL_THUNDER_3,
		PIXEL_THUNDER_4,
		PIXEL_THUNDER_5,

		ASSAULT_RIFLE,
		test_shootable_weapons,
		KEK9,
		SUBMACHINE,
		URBAN_CYAN_MACHETE,
		ROCKET_LAUNCHER,

		TEST_BACKGROUND,
test_container_items
		SAMPLE_MAGAZINE,
		SMALL_MAGAZINE,
		ROUND_TRACE,
		ENERGY_BALL,
		PINK_CHARGE,
		test_shootable_charges,
		CYAN_CHARGE,
		CYAN_SHELL,
		RED_CHARGE,
		RED_SHELL,
		GREEN_CHARGE,
		GREEN_SHELL,
		BACKPACK,
test_sprite_decorations
		test_sprite_decorations,
		test_sprite_decorations,
		METROPOLIS,

		test_sprite_decorations,
		ROAD,
		ROAD_FRONT_DIRT,

		CAST_BLINK_1,
		CAST_BLINK_2,
		CAST_BLINK_3,
		CAST_BLINK_4,
		CAST_BLINK_5,
		CAST_BLINK_6,
		CAST_BLINK_7,
		CAST_BLINK_8,
		CAST_BLINK_9,
		CAST_BLINK_10,
		CAST_BLINK_11,
		CAST_BLINK_12,
		CAST_BLINK_13,
		CAST_BLINK_14,
		CAST_BLINK_15,
		CAST_BLINK_16,
		CAST_BLINK_17,
		CAST_BLINK_18,
		CAST_BLINK_19,

		TRUCK_ENGINE,

		HEALTH_ICON,
		PERSONAL_ELECTRICITY_ICON,
		CONSCIOUSNESS_ICON,
test_shootable_weapons
		AMPLIFIER_ARM,

		SPELL_HASTE_ICON,
		SPELL_ELECTRIC_SHIELD_ICON,
		SPELL_ELECTRIC_TRIAD_ICON,
		SPELL_FURY_OF_THE_AEONS_ICON,
		SPELL_ULTIMATE_WRATH_OF_THE_AEONS_ICON,
		SPELL_EXALTATION_ICON,
		SPELL_ECHOES_OF_THE_HIGHER_REALMS_ICON,

		PERK_HASTE_ICON,
		PERK_ELECTRIC_SHIELD_ICON,
test_throwable_explosives
		test_throwable_explosives,
		test_throwable_explosives,
		INTERFERENCE_GRENADE,

		FORCE_GRENADE_RELEASED,
		PED_GRENADE_RELEASED,
		INTERFERENCE_GRENADE_RELEASED,
		FORCE_ROCKET,
#endif
		COUNT = MAX_GAME_IMAGE_COUNT + 1
		// END GEN INTROSPECTOR
	};
}