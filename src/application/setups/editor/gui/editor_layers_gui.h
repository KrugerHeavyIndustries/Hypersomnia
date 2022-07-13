#pragma once
#include "augs/misc/imgui/standard_window_mixin.h"
#include "view/necessary_resources.h"
#include "view/viewables/ad_hoc_in_atlas_map.h"

struct editor_filesystem_node;
class editor_setup;
struct editor_layers;

struct editor_layers_input {
	editor_setup& setup;
	editor_layers& layers;
	const editor_filesystem_node*& dragged_resource;
	const ad_hoc_in_atlas_map& ad_hoc_atlas;
	const necessary_images_in_atlas_map& necessary_images;
};

struct editor_layers_gui : standard_window_mixin<editor_layers_gui> {
	using base = standard_window_mixin<editor_layers_gui>;
	using base::base;
	using introspect_base = base;

	editor_node_id dragged_node;

	void perform(editor_layers_input);
};
