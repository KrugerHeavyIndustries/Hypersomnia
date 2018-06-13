#pragma once
#include "application/setups/editor/property_editor/general_edit_properties.h"
#include "application/setups/editor/detail/read_write_defaults_buttons.h"
#include "application/setups/editor/property_editor/special_widgets.h"
#include "application/setups/editor/detail/pathed_asset_entry.h"

template <class asset_id_type, class A, class B, class C>
void do_pathed_asset_properties(
	const path_tree_settings& tree_settings,
	const property_editor_input& prop_in,
	const editor_command_input cmd_in,
	const pathed_asset_entry<asset_id_type>& path_entry,
	const A& ticked_in_range,
	const B& ticked_ids,
	const bool is_current_ticked,
	const C& game_atlas,
	editor_image_preview& preview,
	const int num_cols,
	const bool nodeize_image_widgets = false
) {
	using namespace augs::imgui;

	constexpr bool is_image_type = std::is_same_v<asset_id_type, assets::image_id>;

	auto& viewables = cmd_in.get_viewable_defs();
	auto& definitions = get_viewable_pool<asset_id_type>(viewables);

	const auto id = path_entry.id;
	const auto displayed_name = tree_settings.get_prettified(path_entry.get_filename());

	{
		::read_write_defaults_buttons(
			prop_in.settings,
			cmd_in,
			definitions,
			id,
			is_current_ticked,
			ticked_in_range
		);

		next_columns(num_cols);
	}

	const auto property_location = typesafe_sprintf(" (in %x)", displayed_name);

	using command_type = change_asset_property_command<asset_id_type>;

	auto post_new_change = [&](
		const auto& description,
		const auto field_id,
		const auto& new_content
	) {
		command_type cmd;

		if constexpr(source_path_widget::handles<remove_cref<decltype(new_content)>>) {
			/* 
				If we are changing the source path inside an asset that is ticked,
				don't propagate the change throughout the rest of ticks
				because we uniquely identify exactly by the source path.
			*/

			cmd.affected_assets = { id };
		}
		else {
			if (is_current_ticked) {
				cmd.affected_assets = ticked_ids;
			}
			else {
				cmd.affected_assets = { id };
			}
		}

		cmd.property_id.field = field_id;
		cmd.value_after_change = augs::to_bytes(new_content);
		cmd.built_description = description + property_location;

		post_editor_command(cmd_in, std::move(cmd));
	};

	auto rewrite_last_change = [&](
		const auto& description,
		const auto& new_content
	) {
		auto& last = cmd_in.get_history().last_command();

		if (auto* const cmd = std::get_if<command_type>(std::addressof(last))) {
			cmd->built_description = description + property_location;
			cmd->rewrite_change(augs::to_bytes(new_content), cmd_in);
		}
		else {
			LOG("WARNING! There was some problem with tracking activity of editor controls.");
		}
	};

	const bool disable_path_chooser = is_current_ticked && ticked_ids.size() > 1;

	/* 
		Don't construct this widget for sounds and other pathed widgets. 
		Image offsets apply only to... images.
	*/

	using offset_widget = 
		std::conditional_t<
			is_image_type,
			image_offset_widget,
			default_widget_provider
		>
	;

	using color_widget =
		std::conditional_t<
			is_image_type,
			image_color_picker_widget,
			default_widget_provider
		>
	;

	const auto& project_path = cmd_in.folder.current_path;

	general_edit_properties(
		prop_in,
		definitions[id],
		post_new_change,
		rewrite_last_change,
		[&](const auto& first, const field_address field_id) {
			if (!is_current_ticked) {
				return true;
			}

			return compare_all_fields_to(
				first,
				asset_property_id<asset_id_type> { field_id }, 
				cmd_in, 
				ticked_ids
			);
		},
		special_widgets(
			source_path_widget { viewables, project_path, prop_in.settings, disable_path_chooser },
			offset_widget { id, game_atlas, nodeize_image_widgets },
			color_widget { id, game_atlas, viewables.image_definitions, preview, project_path, nodeize_image_widgets }
		),
		default_sane_default_provider(),
		num_cols - 2
	);
}
