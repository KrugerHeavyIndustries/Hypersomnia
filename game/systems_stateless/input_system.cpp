#include <random>

#include "game/transcendental/entity_id.h"
#include "game/transcendental/cosmos.h"

#include "input_system.h"

#include "augs/filesystem/file.h"
#include "augs/filesystem/directory.h"
#include "augs/misc/time_utils.h"

#include "game/components/transform_component.h"
#include "game/components/gun_component.h"

#include "game/messages/intent_message.h"

#include "augs/window_framework/event.h"
#include "game/transcendental/logic_step.h"
#include "game/transcendental/entity_handle.h"

using namespace augs::window;

void input_system::make_intent_messages(const logic_step step) {
	for (const auto& per_entity : step.input.entropy.intents_per_entity) {
		for (const auto& raw : per_entity.second) {
			messages::intent_message intent;
			intent.game_intent::operator=(raw);
			intent.subject = per_entity.first;
			step.transient.messages.post(intent);
		}
	}
}