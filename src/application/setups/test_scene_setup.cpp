#include "game/transcendental/logic_step.h"
#include "game/organization/all_messages_includes.h"

#include "view/viewables/viewables_loading_type.h"

#include "application/config_lua_table.h"
#include "application/setups/test_scene_setup.h"

#include "generated/introspectors.h"

using namespace augs::event::keys;

test_scene_setup::test_scene_setup(
	sol::state& lua,
	const bool make_minimal_test_scene,
	const input_recording_type recording_type
) {
#if BUILD_TEST_SCENES
	scene.make_test_scene(lua, make_minimal_test_scene);
	characters.acquire_available_characters(scene.world);
#endif

	if (recording_type != input_recording_type::DISABLED) {
		//if (player.try_to_load_or_save_new_session(LOCAL_FILES_DIR "sessions/", "recorded.inputs")) {
		//
		//}
	}
}

void test_scene_setup::customize_for_viewing(config_lua_table& config) const {
	config.window.name = "Hypersomnia test scene";
	return;
}

void test_scene_setup::control(const cosmic_entropy& events) {
	total_collected_entropy += events;
}

void test_scene_setup::accept_game_gui_events(const cosmic_entropy& events) {
	total_collected_entropy += events;
}