#pragma once
#include "application/setups/editor/commands/editor_command_meta.h"
#include "application/setups/editor/resources/editor_typed_resource_id.h"
#include "application/setups/editor/gui/inspected_variant.h"

template <class T>
struct edit_resource_command {
	using editable_type = decltype(T::editable);

	editor_command_meta meta;

	editor_typed_resource_id<T> resource_id;

	editable_type before;
	editable_type after;

	std::string built_description;

	std::optional<std::vector<inspected_variant>> override_inspector_state;

	void undo(editor_command_input in);
	void redo(editor_command_input in);

	const auto& describe() const {
		return built_description;
	}
};
