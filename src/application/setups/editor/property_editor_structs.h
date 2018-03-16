#pragma once
#include "3rdparty/imgui/imgui.h"
#include "augs/readwrite/memory_stream.h"

#include "game/components/shape_polygon_component.h"
#include "game/components/sentience_component.h"

using edited_field_type_id = type_in_list_id<
	type_list<
		augs::trivial_type_marker,
		std::string,
		convex_partitioned_shape::poly_vector_type
	>
>;

struct description_pair {
	std::string of_old;
	std::string of_new;
};

struct editor_properties_gui {
	std::optional<ImGuiID> last_active;
	std::string old_description = "";
};

struct field_address {
	unsigned offset = static_cast<unsigned>(-1);
	edited_field_type_id type_id;
};
