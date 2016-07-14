#include "sentience_system.h"

#include "game/messages/damage_message.h"
#include "game/messages/health_event.h"
#include "game/components/sentience_component.h"
#include "game/components/render_component.h"
#include "game/messages/health_event.h"

#include "game/cosmos.h"
#include "game/entity_id.h"

#include "game/components/physics_component.h"
#include "game/components/container_component.h"
#include "game/components/position_copying_component.h"

#include "game/components/animation_component.h"
#include "game/components/relations_component.h"
#include "game/components/movement_component.h"

#include "game/detail/inventory_slot.h"
#include "game/detail/inventory_slot_id.h"
#include "game/detail/inventory_utils.h"

#include "game/step.h"

components::sentience::meter::damage_result components::sentience::meter::calculate_damage_result(float amount) const {
	components::sentience::meter::damage_result result;

	if (amount > 0) {
		if (value > 0) {
			if (value <= amount) {
				result.dropped_to_zero = true;
				result.effective = value;
			}
			else {
				result.effective = amount;
			}
		}
	}
	else {
		if (value - amount > maximum) {
			result.effective = -(maximum - value);
		}
		else
			result.effective = amount;
	}

	result.ratio_effective_to_maximum = std::abs(result.effective) / maximum;

	return result;
}

void sentience_system::consume_health_event(messages::health_event h, fixed_step& step) const {
	auto& cosmos = step.cosm;
	auto subject = cosmos[h.subject];
	auto& sentience = subject.get<components::sentience>();

	switch (h.target) {
	case messages::health_event::HEALTH: sentience.health.value -= h.effective_amount; ensure(sentience.health.value >= 0) break;;
	case messages::health_event::CONSCIOUSNESS: sentience.consciousness.value -= h.effective_amount; ensure(sentience.health.value >= 0); break;
	case messages::health_event::SHIELD: ensure(0); break;
	case messages::health_event::AIM:
		auto punched = subject;

		if (punched[sub_entity_name::CHARACTER_CROSSHAIR].alive() && punched[sub_entity_name::CHARACTER_CROSSHAIR][sub_entity_name::CROSSHAIR_RECOIL_BODY].alive()) {
			auto owning_crosshair_recoil = punched[sub_entity_name::CHARACTER_CROSSHAIR][sub_entity_name::CROSSHAIR_RECOIL_BODY];

			sentience.aimpunch.shoot_and_apply_impulse(owning_crosshair_recoil, 1 / 15.f, true,
				(h.point_of_impact - punched.get<components::transform>().pos).cross(h.impact_velocity) / 100000000.f * 3.f / 25.f
			);
		}

		break;
	}

	if (h.special_result == messages::health_event::DEATH) {
		auto* container = subject.find<components::container>();

		if (container)
			drop_from_all_slots(subject, step);

		auto sub_def = subject[sub_entity_name::CORPSE_OF_SENTIENCE];

		auto corpse = cosmos.clone_entity(sub_def);

		auto place_of_death = subject.get<components::transform>();
		place_of_death.rotation = h.impact_velocity.degrees();

		corpse.get<components::physics>().set_transform(place_of_death);
		
		subject.get<components::physics>().set_activated(false);
		subject.get<components::position_copying>().set_target(corpse);

		corpse.get<components::physics>().apply_force(vec2().set_from_degrees(place_of_death.rotation).set_length(27850 * 2));

		h.spawned_remnants = corpse;
		corpse.map_associated_entity(associated_entity_name::ASTRAL_BODY, subject);

		corpse.add_standard_components();
	}

	step.messages.post(h);
}

void sentience_system::apply_damage_and_generate_health_events(fixed_step& step) const {
	auto& damages = step.messages.get_queue<messages::damage_message>();
	auto& healths = step.messages.get_queue<messages::health_event>();
	auto& cosmos = step.cosm;

	healths.clear();

	for (auto& d : damages) {
		auto subject = cosmos[d.subject];

		auto* sentience = subject.find<components::sentience>();

		messages::health_event event;
		event.subject = d.subject;
		event.point_of_impact = d.point_of_impact;
		event.impact_velocity = d.impact_velocity;

		auto aimpunch_event = event;
		aimpunch_event.target = messages::health_event::AIM;

		if (sentience)
			aimpunch_event.subject = subject;
		else
			aimpunch_event.subject = subject.get_owning_transfer_capability();

		if (d.amount > 0 && cosmos[aimpunch_event.subject].alive())
			consume_health_event(aimpunch_event, step);

		if (sentience) {
			auto& s = *sentience;

			event.effective_amount = 0;
			event.objective_amount = d.amount;
			event.special_result = messages::health_event::NONE;

			if (s.health.enabled) {
				event.target = messages::health_event::HEALTH;

				auto damaged = s.health.calculate_damage_result(d.amount);
				event.effective_amount = damaged.effective;
				event.ratio_effective_to_maximum = damaged.ratio_effective_to_maximum;

				if (damaged.dropped_to_zero) {
					event.special_result = messages::health_event::DEATH;
				}

				if (event.effective_amount != 0)
					consume_health_event(event, step);
			}

			if (s.consciousness.enabled) {
				event.target = messages::health_event::CONSCIOUSNESS;

				auto damaged = s.consciousness.calculate_damage_result(d.amount);
				event.effective_amount = damaged.effective;
				event.ratio_effective_to_maximum = damaged.ratio_effective_to_maximum;

				if (damaged.dropped_to_zero) {
					event.special_result = messages::health_event::LOSS_OF_CONSCIOUSNESS;
				}

				if (event.effective_amount != 0)
					consume_health_event(event, step);
			}
		}
	}
}

void sentience_system::cooldown_aimpunches(fixed_step& step) const {
	for (auto& t : step.cosm.get(processing_subjects::WITH_SENTIENCE)) {
		t.get<components::sentience>().aimpunch.cooldown(step.get_delta().in_milliseconds());
	}
}

void sentience_system::regenerate_values(fixed_step& step) const {
	for (auto& t : step.cosm.get(processing_subjects::WITH_SENTIENCE)) {
		t.get<components::sentience>().aimpunch.cooldown(step.get_delta().in_milliseconds());
	}
}

void sentience_system::set_borders(fixed_step& step) const {
	int timestamp_ms = static_cast<int>(step.get_delta().total_time_passed_in_seconds() * 1000.0);

	for (auto& t : step.cosm.get(processing_subjects::WITH_SENTIENCE)) {
		auto& sentience = t.get<components::sentience>();

		auto hr = sentience.health.ratio();
		auto one_less_hr = 1 - hr;

		int pulse_duration = static_cast<int>(1250 - 1000 * (1 - hr));
		float time_pulse_ratio = (timestamp_ms % pulse_duration) / float(pulse_duration);

		hr *= 1.f - (0.2f * time_pulse_ratio);

		auto* render = t.find<components::render>();

		if (render) {
			if (hr < 1.f) {
				render->draw_border = true;
				render->border_color = augs::rgba(255, 0, 0, static_cast<augs::rgba_channel>(one_less_hr * one_less_hr * one_less_hr * one_less_hr * 255 * time_pulse_ratio));
			}
			else
				render->draw_border = false;
		}
	}
}
