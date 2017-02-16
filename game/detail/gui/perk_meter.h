#pragma once
#include "augs/gui/rect.h"
#include "augs/gui/material.h"
#include "augs/gui/appearance_detector.h"

#include "game/detail/gui/game_gui_context.h"

struct perk_meter : game_gui_rect_node {
	typedef dereferenced_location<perk_meter_in_character_gui> this_pointer;
	typedef const_dereferenced_location<perk_meter_in_character_gui> const_this_pointer;
	typedef typename game_gui_rect_node::gui_entropy gui_entropy;

	augs::gui::appearance_detector detector;

	struct effect_particle {
		vec2i relative_pos;
		augs::gui::material mat;
	};

	int border_width = 1;
	int border_spacing = 1;

	float seconds_accumulated = 0.f;

	int get_total_spacing() const {
		return border_width + border_spacing;
	}

	std::vector<effect_particle> particles;

	perk_meter();

	augs::gui::material get_icon_mat(const const_this_pointer this_id) const;
	augs::gui::material get_bar_mat(const const_this_pointer this_id) const;

	ltrb get_full_value_bar_rect(
		const const_game_gui_context context,
		const const_this_pointer this_id,
		const ltrb absolute
	) const;

	ltrb get_full_value_bar_rect_bordered(
		const const_game_gui_context context,
		const const_this_pointer this_id,
		const ltrbi absolute
	) const;

	static void draw(
		const viewing_game_gui_context context,
		const const_this_pointer this_id,
		augs::gui::draw_info info
	);

	static void advance_elements(
		const game_gui_context,
		const this_pointer this_id,
		const augs::delta
	);

	static void respond_to_events(
		const game_gui_context,
		const this_pointer this_id,
		const gui_entropy& entropies
	);

	static void rebuild_layouts(
		const game_gui_context,
		const this_pointer this_id
	);
};