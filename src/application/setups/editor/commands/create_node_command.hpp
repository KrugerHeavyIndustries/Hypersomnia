#pragma once
#include "application/setups/editor/commands/create_node_command.h"
#include "augs/misc/pool/pool_allocate.h"

template <class T>
inline void create_node_command<T>::undo(editor_command_input in) {
	in.setup.unregister_node_from_layer(get_node_id(), layer_id);

	{
		auto& node_pool = in.setup.project.nodes.template get_pool_for<T>();
		base::undo(node_pool);
	}

	if (create_layer != std::nullopt) {
		create_layer->undo(in);
	}

	in.setup.clear_inspector();
}

template <class T>
inline void create_node_command<T>::redo(editor_command_input in) {
	if (create_layer != std::nullopt) {
		create_layer->omit_inspector = true;
		create_layer->redo(in);

		layer_id = create_layer->get_allocated_id();
	}

	{
		auto& node_pool = in.setup.project.nodes.template get_pool_for<T>();
		base::redo(node_pool, created_node);
	}

	const auto node_id = get_node_id();

	if (in.setup.register_node_in_layer(node_id, layer_id, index_in_layer)) {
		in.setup.inspect_only(node_id);
	}
}

template <class T>
inline editor_node_id create_node_command<T>::get_node_id() const {
	return editor_typed_node_id<T> { base::get_allocated_id() }.operator editor_node_id();
}
