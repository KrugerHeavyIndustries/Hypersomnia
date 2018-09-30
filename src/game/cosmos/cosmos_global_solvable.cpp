#include "game/cosmos/cosmos_global_solvable.h"
#include "augs/templates/container_templates.h"
#include "game/cosmos/cosmos.h"
#include "game/cosmos/entity_handle.h"
#include "game/cosmos/logic_step.h"
#include "game/components/item_sync.h"
#include "game/detail/inventory/perform_transfer.h"
#include "game/detail/inventory/inventory_slot_handle.h"
#include "game/detail/entity_handle_mixins/get_owning_transfer_capability.hpp"
#include "game/detail/entity_handle_mixins/inventory_mixin.hpp"
#include "game/detail/inventory/item_mounting.hpp"
#include "game/messages/start_sound_effect.h"
#include "game/cosmos/data_living_one_step.h"

bool pending_item_mount::is_unmounting(const const_entity_handle& handle) const {
	if (const auto current_slot = handle.get_current_slot()) {
		if (const bool source_mounted = current_slot->is_mounted_slot()) {
			return true;
		}
	}

	return false;
}

real32 pending_item_mount::get_mounting_duration_ms(const const_entity_handle& handle) const {
	if (const auto current_slot = handle.get_current_slot()) {
		const auto& cosm = handle.get_cosmos();

		const auto target_slot = cosm[target];

		if (const bool source_mounted = current_slot->is_mounted_slot()) {
			/* We're unmounting */
			auto d = current_slot->mounting_duration_ms;

			if (target_slot.dead()) {
				/* We're dropping, so faster */
				d *= 0.35f;
			}

			return d;
		}
		else if (target_slot) {
			/* We're mounting */
			return target_slot->mounting_duration_ms;
		}
	}

	return -1.f;
}

void cosmos_global_solvable::solve_item_mounting(const logic_step step) {
	const auto delta = step.get_delta();
	auto& cosm = step.get_cosmos();

	erase_if(pending_item_mounts, [&](auto& m) {
		const auto& e_id = m.first;

		const auto item_handle = cosm[e_id];

		if (item_handle.dead()) {
			return true;
		}

		auto& request = m.second;
		auto& progress = request.progress_ms;

		const auto target_slot = cosm[request.target];

		auto& specified_charges = request.params.specified_quantity;

		if (specified_charges == 0) {
			return true;
		}

		bool should_be_erased = true;

		item_handle.template dispatch_on_having_all<components::item>([&](const auto& transferred_item) {
			const auto conditions = calc_mounting_conditions(transferred_item, target_slot);

			enum class sound_type {
				START,
				FINISH
			};

			auto get_sound_effect_input = [&](const sound_type t) {
				if (const auto current_slot = transferred_item.get_current_slot()) {
					if (const bool source_mounted = current_slot->is_mounted_slot()) {
						if (t == sound_type::START) {
							return current_slot->start_unmounting_sound;
						}
						else if (t == sound_type::FINISH) {
							return current_slot->finish_unmounting_sound;
						}
					}
					else {
						if (t == sound_type::START) {
							return target_slot->start_mounting_sound;
						}
						else if (t == sound_type::FINISH) {
							return target_slot->finish_mounting_sound;
						}
					}
				}

				return sound_effect_input();
			};

			auto stop_the_start_sound = [&]() {
				const auto input = get_sound_effect_input(sound_type::START);

				messages::stop_sound_effect s;
				s.match_chased_subject = transferred_item;
				s.match_effect_id = input.id;
				step.post_message(s);
			};

			if (conditions == mounting_conditions_type::PROGRESS) {
				auto play_sound = [&](const sound_type t) {
					/* Play start sound */
					packaged_sound_effect sound;
					sound.input = get_sound_effect_input(t);

					{
						const auto capability = transferred_item.get_owning_transfer_capability();
						sound.start = sound_effect_start_input::at_entity(transferred_item).set_listener(capability);
					}

					sound.post(step);
				};

				if (progress == 0.f) {
					play_sound(sound_type::START);
				}

				should_be_erased = false;
				progress += delta.in_milliseconds();

				const auto considered_max = request.get_mounting_duration_ms(transferred_item); 

				if (progress >= considered_max) {
					play_sound(sound_type::FINISH);
					stop_the_start_sound();

					item_slot_transfer_request transfer;
					transfer.item = transferred_item;
					transfer.target_slot = target_slot;
					transfer.params = request.params;
					transfer.params.bypass_mounting_requirements = true;
					transfer.params.specified_quantity = 1;

					const auto previous_charges = transferred_item.template get<components::item>().get_charges();

					::perform_transfer(transfer, step);

					if (previous_charges == 1) {
						should_be_erased = true;
						return;
					}

					if (specified_charges != -1) {
						--specified_charges;
						progress = 0.f;

						if (specified_charges == 0) {
							should_be_erased = true;
							return;
						}
					}
				}
			}
			else {
				/* Failure. Stop the sound that was started. */
				stop_the_start_sound();
			}
		});

		return should_be_erased;
	});
}

void cosmos_global_solvable::clear() {
	pending_item_mounts.clear();
}
