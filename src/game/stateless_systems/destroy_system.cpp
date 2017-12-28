#include "augs/templates/introspect.h"
#include "destroy_system.h"
#include "game/transcendental/cosmos.h"
#include "game/transcendental/entity_id.h"

#include "game/messages/queue_destruction.h"
#include "game/messages/will_soon_be_deleted.h"

#include "game/transcendental/entity_handle.h"
#include "game/transcendental/logic_step.h"
#include "game/transcendental/data_living_one_step.h"

#include "augs/ensure.h"

#include "game/organization/all_component_includes.h"

#include "game/detail/inventory/perform_transfer.h"

void destroy_system::mark_queued_entities_and_their_children_for_deletion(
	const destruction_queue& queued, 
	deletion_queue& deletions, 
	const cosmos& cosmos
) {
	for (const auto& it : queued) {
		deletions.push_back(it.subject);

		cosmos[it.subject].for_each_child_entity_recursive([&deletions](const child_entity_id descendant) {
			deletions.push_back(descendant);
			return true;
		});
	}
}

void destroy_system::perform_deletions(const deletion_queue& deletions, cosmos& cosmos) {
	// destroy in reverse order; children first

	for (auto it = deletions.rbegin(); it != deletions.rend(); ++it) {
		const auto subject = cosmos[(*it).subject];

		if (subject.dead()) {
			continue;
		}

		cosmos.delete_entity((*it).subject);
	}
}

void destroy_system::mark_queued_entities_and_their_children_for_deletion(const logic_step step) {
	auto& cosmos = step.get_cosmos();
	const auto& queued = step.get_queue<messages::queue_destruction>();
	auto& deletions = step.get_queue<messages::will_soon_be_deleted>();
	mark_queued_entities_and_their_children_for_deletion(queued, deletions, cosmos);
}

void destroy_system::perform_deletions(const logic_step step) {
	auto& cosmos = step.get_cosmos();
	const auto& deletions = step.get_queue<messages::will_soon_be_deleted>();
	perform_deletions(deletions, cosmos);
}
