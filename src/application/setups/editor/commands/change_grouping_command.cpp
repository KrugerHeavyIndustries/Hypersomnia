#include "application/setups/editor/commands/change_grouping_command.h"
#include "application/setups/editor/editor_folder.h"

std::size_t change_grouping_command::size() const {
	return affected_entities.size();
}

bool change_grouping_command::empty() const {
	return affected_entities.empty();
}

void change_grouping_command::push_entry(const entity_id id) {
	affected_entities.push_back(id);
}

void change_grouping_command::redo(const editor_command_input in) {
	ensure(group_indices_before.empty());

	/* First, ungroup the affected entities */
	auto& groups = in.folder.view.selection_groups;

	const auto pusher = [this](const auto index, auto& group, const auto it) {
		group_indices_before.push_back(static_cast<unsigned>(index));
		group.erase(it);
	};

	for (const auto& e : affected_entities) {
		if (!groups.on_group_entry_of(e, pusher)) {
			group_indices_before.push_back(-1);	
		}
	}

	ensure_eq(group_indices_before.size(), affected_entities.size());

	if (create_new_group) {
		assign_begin_end(groups.new_group(), affected_entities);
	}
}

void change_grouping_command::undo(const editor_command_input in) {
	ensure_eq(group_indices_before.size(), affected_entities.size());

	auto& groups = in.folder.view.selection_groups;

	const auto eraser = [&](const auto, auto& group, const auto it) {
		group.erase(it);
	};

	for (const auto& e : affected_entities) {
		const auto found_group = groups.on_group_entry_of(e, eraser);

		if (create_new_group) {
			/* 
				We're undoing creation of new group, 
				so the group MUST have been found on undoing. 
			*/

			ensure(found_group);
		}
		else {
			/* 
				We're undoing ungrouping some collection of entities,
				so NO GROUP must have been found on undoing.
			*/

			ensure(!found_group);
		}
	}

	for (std::size_t i = 0; i < affected_entities.size(); ++i) {
		if (const auto previous_group_index = group_indices_before[i];
			previous_group_index != -1	
		) {
			const auto id = affected_entities[i];
			groups.set_group(previous_group_index, id); 
		}
	}

	group_indices_before.clear();
}

std::string change_grouping_command::describe() const {
	return built_description;
}

