#include "game/detail/inventory/perform_transfer.h"

#include "game/components/item_component.h"
#include "game/components/missile_component.h"
#include "game/components/fixtures_component.h"
#include "game/components/rigid_body_component.h"
#include "game/components/special_physics_component.h"
#include "game/components/item_slot_transfers_component.h"
#include "game/components/type_component.h"
#include "game/components/motor_joint_component.h"
#include "game/components/sound_existence_component.h"

#include "game/transcendental/logic_step.h"
#include "game/transcendental/data_living_one_step.h"

#include "augs/templates/container_templates.h"
#include "game/transcendental/cosmos.h"

void drop_from_all_slots(const components::container& container, const entity_handle handle, const logic_step step) {
	drop_from_all_slots(container, handle, [step](const auto& result) { result.notify(step); });
}

void perform_transfer_result::notify(const logic_step step) const {
	const auto& cosmos = step.get_cosmos();

	step.post_message_if(picked);
	step.post_messages(interpolation_corrected);
	step.post_message_if(destructed);

	if (dropped.has_value()) {
		auto& d = *dropped;

		d.sound.create_sound_effect_entity(
			step,
			d.sound_transform,
			cosmos[d.sound_subject]
		).add_standard_components(step);
	}
}

void perform_transfer(
	const item_slot_transfer_request r,
	const logic_step step
) {
	perform_transfer(r, step.get_cosmos()).notify(step);
}

void detail_add_item(const inventory_slot_handle handle, const entity_handle new_item) {
	new_item.get<components::item>().current_slot = handle;
	new_item.get_cosmos().get_solvable_inferred({}).relational.set_current_slot(new_item, handle.get_id());
}

void detail_remove_item(const inventory_slot_handle handle, const entity_handle removed_item) {
	removed_item.get<components::item>().current_slot.unset();
	removed_item.get_cosmos().get_solvable_inferred({}).relational.set_current_slot(removed_item, {});
}

perform_transfer_result perform_transfer(
	const item_slot_transfer_request r, 
	cosmos& cosmos
) {
	perform_transfer_result output;

	const auto transferred_item = cosmos[r.item];
	auto& item = transferred_item.get<components::item>();

	const auto result = query_transfer_result(cosmos, r);

	if (!is_successful(result.result)) {
		LOG("Warning: an item-slot transfer was not successful.");
		return output;
	}

	const auto previous_slot = cosmos[item.get_current_slot()];
	const auto target_slot = cosmos[r.target_slot];

	const auto previous_slot_container = previous_slot.get_container();
	const auto target_slot_container = target_slot.get_container();

	const bool is_pickup = result.result == item_transfer_result_type::SUCCESSFUL_PICKUP;
	const bool target_slot_exists = result.result == item_transfer_result_type::SUCCESSFUL_TRANSFER || is_pickup;
	const bool is_drop_request = result.result == item_transfer_result_type::SUCCESSFUL_DROP;

	const auto initial_transform_of_transferred = transferred_item.get_logic_transform();

	/*
		if (result.result == item_transfer_result_type::UNMOUNT_BEFOREHAND) {
			ensure(false);
			ensure(previous_slot.alive());

			item.request_unmount(r.get_target_slot());
			item.mark_parent_enclosing_containers_for_unmount();

			return;
		}
	*/
	entity_id target_item_to_stack_with_id;

	if (target_slot_exists) {
		for (const auto potential_stack_target : target_slot.get_items_inside()) {
			if (can_stack_entities(transferred_item, cosmos[potential_stack_target])) {
				target_item_to_stack_with_id = potential_stack_target;
			}
		}
	}

	const bool whole_item_grabbed = item.charges == result.transferred_charges;

	components::transform previous_container_transform;

	if (previous_slot.alive()) {
		previous_container_transform = previous_slot_container.get_logic_transform();

		if (whole_item_grabbed) {
			detail_remove_item(previous_slot, transferred_item);
		}

		if (previous_slot.is_hand_slot()) {
			unset_input_flags_of_orphaned_entity(transferred_item);
		}
	}

	const auto target_item_to_stack_with = cosmos[target_item_to_stack_with_id];

	if (target_item_to_stack_with.alive()) {
		if (whole_item_grabbed) {
			output.destructed.emplace(transferred_item);
		}
		else {
			item.charges -= result.transferred_charges;
		}

		target_item_to_stack_with.get<components::item>().charges += result.transferred_charges;

		return output;
	}

	entity_id grabbed_item_part;

	if (whole_item_grabbed) {
		grabbed_item_part = transferred_item;
	}
	else {
		grabbed_item_part = cosmos.clone_entity(transferred_item);
		item.charges -= result.transferred_charges;
		cosmos[grabbed_item_part].get<components::item>().charges = result.transferred_charges;
	}

	const auto grabbed_item_part_handle = cosmos[grabbed_item_part];

	if (target_slot_exists) {
		detail_add_item(target_slot, grabbed_item_part_handle);
	}

	const auto physics_updater = [&output, previous_container_transform, initial_transform_of_transferred, &cosmos](
		const entity_handle descendant, 
		auto... args
	) {
		const auto& cosmos = descendant.get_cosmos();

		const auto slot = cosmos[descendant.get<components::item>().get_current_slot()];

		entity_id owner_body = descendant;
		bool should_fixtures_persist = true;
		bool should_body_persist = true;
		components::transform fixtures_offset;
		components::transform force_joint_offset;
		components::transform target_transform = initial_transform_of_transferred;
		bool slot_requests_connection_of_bodies = false;

		if (slot.alive()) {
			should_fixtures_persist = slot.is_physically_connected_until();

			if (should_fixtures_persist) {
				const auto first_with_body = slot.get_first_ancestor_with_body_connection();

				fixtures_offset = sum_attachment_offsets(cosmos, descendant.get_address_from_root(first_with_body));

				if (slot->physical_behaviour == slot_physical_behaviour::CONNECT_AS_JOINTED_BODY) {
					slot_requests_connection_of_bodies = true;
					force_joint_offset = fixtures_offset;
					target_transform = first_with_body.get_logic_transform() * force_joint_offset;
					fixtures_offset.reset();
				}
				else {
					should_body_persist = false;
					owner_body = first_with_body;
				}
			}
			else {
				should_body_persist = false;
				owner_body = descendant;
			}
		}

		auto def = descendant.get<components::fixtures>().get_raw_component();
		def.offsets_for_created_shapes[colliders_offset_type::ITEM_ATTACHMENT_DISPLACEMENT] = fixtures_offset;
		def.activated = should_fixtures_persist;
		def.owner_body = owner_body;

		descendant.get<components::fixtures>() = def;

		const auto rigid_body = descendant.get<components::rigid_body>();
		rigid_body.set_activated(should_body_persist);

		if (should_body_persist) {
			rigid_body.set_transform(target_transform);
			rigid_body.set_velocity({ 0.f, 0.f });
			rigid_body.set_angular_velocity(0.f);
		}

		{
			const auto motor_handle = descendant.get<components::motor_joint>();
			components::motor_joint motor = motor_handle.get_raw_component();

			if (slot_requests_connection_of_bodies) {
				motor.activated = true;  
				motor.target_bodies[0] = slot.get_container();
				motor.target_bodies[1] = descendant;
				motor.linear_offset = force_joint_offset.pos;
				motor.angular_offset = force_joint_offset.rotation;
				motor.collide_connected = false;
			}
			else {
				motor.activated = false;  
			}

			motor_handle = motor;
		}

		messages::interpolation_correction_request request;
		request.subject = descendant;
		request.set_previous_transform_value = target_transform;
		output.interpolation_corrected.push_back(request);

		return recursive_callback_result::CONTINUE_AND_RECURSE;
	};

	physics_updater(grabbed_item_part_handle);
	grabbed_item_part_handle.for_each_contained_item_recursive(physics_updater);

	if (is_pickup) {
		const auto target_capability = target_slot_container.get_owning_transfer_capability();

		output.picked.emplace();
		output.picked->subject = target_capability;
		output.picked->item = grabbed_item_part_handle;
	}

	auto& grabbed_item = grabbed_item_part_handle.get<components::item>();

	if (target_slot_exists) {
		if (target_slot->items_need_mounting) {
			grabbed_item.intended_mounting = components::item::MOUNTED;

			if (r.force_immediate_mount) {
				grabbed_item.current_mounting = components::item::MOUNTED;
			}
		}
	}

	if (is_drop_request) {
		ensure(previous_slot_container.alive());

		const auto rigid_body = grabbed_item_part_handle.get<components::rigid_body>();

		// LOG_NVPS(rigid_body.velocity());
		// ensure(rigid_body.velocity().is_epsilon());

		rigid_body.set_velocity({ 0.f, 0.f });
		rigid_body.set_angular_velocity(0.f);

		if (r.impulse_applied_on_drop > 0.f) {
			const auto impulse = vec2::from_degrees(previous_container_transform.rotation) * r.impulse_applied_on_drop;
			rigid_body.apply_impulse(impulse * rigid_body.get_mass());
		}

		rigid_body.apply_angular_impulse(1.5f * rigid_body.get_mass());

		auto& special_physics = grabbed_item_part_handle.get<components::special_physics>();
		special_physics.dropped_or_created_cooldown.set(300, cosmos.get_timestamp());
		special_physics.during_cooldown_ignore_collision_with = previous_slot_container;

		output.dropped.emplace();
		auto& dropped = *output.dropped;

		auto& sound = dropped.sound;
		sound.delete_entity_after_effect_lifetime = true;
		sound.direct_listener = previous_slot_container.get_owning_transfer_capability();
		sound.effect = cosmos.get_common_assets().item_throw_sound;

		dropped.sound_transform = initial_transform_of_transferred;
		dropped.sound_subject = grabbed_item_part_handle;
	}

	return output;
}