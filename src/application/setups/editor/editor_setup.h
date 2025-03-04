#pragma once
#include <optional>
#include "augs/misc/timing/fixed_delta_timer.h"
#include "augs/math/camera_cone.h"

#include "game/detail/render_layer_filter.h"
#include "game/cosmos/entity_handle.h"
#include "game/modes/test_mode.h"

#include "test_scenes/test_scene_settings.h"

#include "application/intercosm.h"
#include "application/setups/default_setup_settings.h"

#include "application/debug_settings.h"
#include "application/input/entropy_accumulator.h"
#include "application/setups/setup_common.h"
#include "view/mode_gui/arena/arena_player_meta.h"

#include "view/viewables/ad_hoc_atlas_subject.h"
#include "application/setups/editor/project/editor_project.h"

#include "application/setups/editor/gui/editor_inspector_gui.h"
#include "application/setups/editor/gui/editor_layers_gui.h"
#include "application/setups/editor/gui/editor_filesystem_gui.h"
#include "application/setups/editor/gui/editor_history_gui.h"

#include "application/setups/editor/editor_filesystem.h"
#include "application/setups/editor/editor_history.h"

#include "application/setups/editor/selector/editor_entity_selector.h"
#include "application/setups/editor/mover/editor_node_mover.h"

#include "application/setups/editor/project/editor_project_paths.h"

#include "application/setups/editor/editor_view.h"
#include "augs/misc/imgui/simple_popup.h"
#include "application/setups/editor/editor_settings.h"
#include "augs/graphics/imgui_payload.h"
#include "application/setups/editor/commands/node_transform_commands.h"

struct config_lua_table;
struct draw_setup_gui_input;

template <class E>
struct editor_typed_resource_id;

template <class E>
struct editor_typed_node_id;

namespace sol {
	class state;
}

struct editor_paths_changed_report {
	std::vector<std::pair<augs::path_type, augs::path_type>> redirects;
	std::vector<augs::path_type> missing;

	bool any() const {
		return redirects.size() > 0 || missing.size() > 0;
	}
};

struct editor_gui {
	// GEN INTROSPECTOR struct editor_gui
	editor_inspector_gui inspector = std::string("Inspector");
	editor_layers_gui layers = std::string("Layers");
	editor_filesystem_gui filesystem = std::string("Resources");
	editor_history_gui history = std::string("History");
	// END GEN INTROSPECTOR
};

struct editor_icon_info;
struct editor_icon_info_in;

class editor_setup : public default_setup_settings {
	intercosm scene;
	per_entity_type_array<std::vector<editor_node_id>> scene_entity_to_node;

	editor_resource_pools official_resources;

	editor_project project;
	editor_gui gui;
	editor_view view;

	editor_entity_selector selector;
	editor_node_mover mover;

	current_selections_type entity_selector_state;
	std::vector<entity_id> cached_selected_comparison;
	std::vector<entity_id> cached_selected_comparison_after;

	std::optional<simple_popup> ok_only_popup;

	editor_history history;
	editor_filesystem files;
	editor_filesystem_node official_files_root;

	const editor_project_paths paths;
	editor_settings settings;

	bool rebuild_ad_hoc_atlas = true;
	ad_hoc_atlas_subjects last_ad_hoc_subjects;

	double global_time_seconds = 0.0;

	void create_official();

	void on_window_activate();
	void rebuild_filesystem();
	editor_paths_changed_report rebuild_pathed_resources();

	void force_autosave_now();

	void load_gui_state();
	void save_gui_state();
	void save_last_project_location();

	template <class S, class F>
	static decltype(auto) on_resource_impl(S& self, const editor_resource_id& id, F&& callback);

	template <class S, class T>
	static decltype(auto) find_resource_impl(S& self, const editor_typed_resource_id<T>& id);

	friend create_layer_command;
	friend delete_layers_command;

	template <class T>
	friend struct create_node_command;

	friend reorder_layers_command;

	friend move_nodes_command;
	friend resize_nodes_command;
	friend flip_nodes_command;

	friend delete_nodes_command;
	friend duplicate_nodes_command;

	friend editor_node_mover;

	cosmos& get_cosmos() {
		return scene.world;
	}

public:
	static constexpr auto loading_strategy = viewables_loading_type::LOAD_ALL;
	static constexpr bool handles_window_input = true;
	static constexpr bool has_additional_highlights = true;

	editor_setup(const augs::path_type& project_path);
	
	~editor_setup();

	void open_default_windows();

	bool handle_input_before_imgui(
		handle_input_before_imgui_input
	);

	bool handle_input_before_game(
		handle_input_before_game_input
	);

	void customize_for_viewing(config_lua_table&) const;
	std::optional<ad_hoc_atlas_subjects> get_new_ad_hoc_images();

	struct parent_layer_info {
		editor_layer_id layer_id;
		const editor_layer* layer_ptr = nullptr;
		std::size_t layer_index;
		std::size_t index_in_layer;
	};

	std::optional<parent_layer_info> find_parent_layer(editor_node_id id) const;
	std::size_t find_layer_index(editor_layer_id id) const;

	void sort_inspected();

	const std::vector<editor_layer_id>& get_layers() const {
		return project.layers.order;
	}

	editor_layer* find_layer(const editor_layer_id& id);
	const editor_layer* find_layer(const editor_layer_id& id) const;

	editor_layer* find_layer(const std::string& name);
	const editor_layer* find_layer(const std::string& name) const;
	void create_new_layer(const std::string& name_pattern = "New layer");

	std::string get_free_layer_name() const;
	std::string get_free_layer_name_for(const std::string& name_pattern) const;

	const auto& get_project() const { return project; }
	const auto& get_official_resources() const { return official_resources; }

	template <class T>
	decltype(auto) find_node(const editor_typed_node_id<T>& id);

	template <class T>
	decltype(auto) find_node(const editor_typed_node_id<T>& id) const;

	template <class T>
	decltype(auto) find_resource(const editor_typed_resource_id<T>& id);

	template <class T>
	decltype(auto) find_resource(const editor_typed_resource_id<T>& id) const;

	template <class F>
	decltype(auto) on_node(const editor_node_id& id, F&& callback);

	template <class F>
	decltype(auto) on_node(const editor_node_id& id, F&& callback) const;

	template <class F>
	decltype(auto) on_resource(const editor_resource_id& id, F&& callback);

	template <class F>
	decltype(auto) on_resource(const editor_resource_id& id, F&& callback) const;

	template <class F>
	void for_each_dashed_line(F&&) const;

	template <class T, class F>
	void for_each_inspected(F&& callback) const {
		gui.inspector.for_each_inspected<T>(std::forward<F>(callback));
	}

	template <class F>
	void for_each_inspected_entity(F&& callback) const;

	std::unordered_set<entity_id> get_all_inspected_entities() const;
	void get_all_selected_by_selector(std::vector<entity_id>& into) const;
	void inspect_from_selector_state();

	void finish_rectangular_selection();
	void select_all_entities();

	void center_view_at_selection();
	void center_view_at(editor_node_id);

	template <class F>
	void for_each_highlight(F&& callback) const;

	editor_node_id to_node_id(entity_id) const;

	entity_id get_hovered_entity(const necessary_images_in_atlas_map& sizes_for_icons, std::optional<vec2> at = std::nullopt) const;
	editor_node_id get_hovered_node(const necessary_images_in_atlas_map& sizes_for_icons, std::optional<vec2> at = std::nullopt) const;
	editor_node_id get_hovered_node() const;

	template <class T>
	entity_id to_entity_id(const editor_typed_node_id<T> id) const {
		if (const auto node = find_node(id)) {
			return node->scene_entity_id;
		}

		return {};
	}

	entity_id to_entity_id(editor_node_id) const;

	void scroll_once_to(inspected_variant);

	std::unordered_map<std::string, editor_node_id> make_name_to_node_map() const;
	std::string get_free_node_name_for(const std::string& new_name) const;

	bool exists(const editor_resource_id&) const;

	void seek_to_revision(editor_history::index_type);
	bool wants_multiple_selection() const;

	template <class T>
	decltype(auto) post_new_command(T&&);

	template <class T>
	decltype(auto) rewrite_last_command(T&&);

	template <class T, class... Args>
	auto make_command_from_selected_entities(const std::string& preffix, Args&&...) const;

	template <class T, class... Args>
	auto make_command_from_selected_nodes(const std::string& preffix, Args&&...) const;

	template <class T, class... Args>
	auto make_command_from_selected_layers(const std::string& preffix, Args&&...) const;

	void clear_inspector();

	bool register_node_in_layer(editor_node_id node, editor_node_id over_node);
	bool register_node_in_layer(editor_node_id node, editor_layer_id layer, std::size_t index);

	void unregister_node_from_layer(editor_node_id);
	void unregister_node_from_layer(editor_node_id, editor_layer_id);

	void inspect(const current_selections_type&);
	void inspect(const std::vector<entity_id>&);

	void inspect(inspected_variant);
	void inspect_add_quiet(inspected_variant);

	template <class F>
	void inspect_erase_if(F&& callback) { 
		erase_if(gui.inspector.all_inspected, callback);
		after_quietly_adding_inspected();
	}

	void after_quietly_adding_inspected();

	void inspect_only(inspected_variant);
	void inspect_only(const std::vector<inspected_variant>&);
	void inspect_only(const std::vector<editor_node_id>&);
	void inspected_to_entity_selector_state();

	template <class T>
	bool inspects_any() const {
		return gui.inspector.template inspects_any<T>();
	}

	bool is_inspected(inspected_variant) const;
	std::vector<inspected_variant> get_all_inspected() const;

	template <class T>
	decltype(auto) get_all_inspected() const {
		return gui.inspector.template get_all_inspected<T>();
	}

	editor_node_id get_topmost_inspected_node() const;

	std::string get_name(inspected_variant) const;
	std::string get_name(entity_id) const;
	std::size_t get_node_count() const;

	editor_history::index_type get_last_command_index() const;

	editor_command_input make_command_input(); 
	entity_selector_input make_selector_input() const;
	node_mover_input make_mover_input();

	std::optional<rgba> find_highlight_color_of(const entity_id id) const;
	std::optional<ltrb> find_selection_aabb() const;

	template <class F>
	std::optional<ltrb> find_aabb_of_typed_nodes(F&& for_each_typed_node) const;

	template <class F>
	std::optional<ltrb> find_aabb_of_nodes(F&& for_each_node) const;

	std::optional<ltrb> find_screen_space_rect_selection(
		const vec2i screen_size,
		const vec2i mouse_pos
	) const;

	void undo();
	void undo_quiet();

	void redo();

	bool handle_doubleclick_in_layers_gui = false;

	void rebuild_scene();

	augs::path_type resolve_project_path(const augs::path_type& path_in_project) const;

	camera_eye get_camera_eye() const;

	vec2 get_world_cursor_pos() const;
	vec2 get_world_cursor_pos(const camera_eye eye) const;

	template <class T>
	editor_icon_info get_icon_for(
		const T& object, 
		editor_icon_info_in
	) const;

	void unhover();
	void clear_id_caches();
	bool confirm_modal_popup();

	const auto& get_view() const {
		return view;
	}

	void start_renaming_selection();

	bool start_moving_selection();
	void finish_moving_selection();
	void show_absolute_mover_pos_once();

	void make_last_command_a_child();

	bool is_next_command_child() const;
	bool is_last_command_child() const;

	void cut_selection();
	void delete_selection();

	bool is_node_visible(const editor_node_id) const;
	auto only_visible_nodes() const {
		return [this](const auto id) {
			return is_node_visible(id);
		};
	}

	void mirror_selection(vec2i direction, bool move_if_only_duplicate = true);
	void duplicate_selection(bool start_moving = true);

	void move_dragged_to_new_layer(editor_node_id dragged_node);
	void move_inspected_to_new_layer();

	const auto& get_history() const {
		return history;
	}

	/*********************************************************/
	/*************** DEFAULT SETUP BOILERPLATE ***************/
	/*********************************************************/

	auto get_audiovisual_speed() const {
		return 1.0;
	}

	const auto& get_viewed_cosmos() const {
		return scene.world;
	}

	auto get_interpolation_ratio() const {
		return 1.0;
	}

	auto get_viewed_character_id() const {
		return entity_id();
	}

	auto get_controlled_character_id() const {
		return get_viewed_character_id();
	}

	auto get_viewed_character() const {
		return get_viewed_cosmos()[get_viewed_character_id()];
	}

	const auto& get_viewable_defs() const {
		return scene.viewables;
	}

	void perform_main_menu_bar(perform_custom_imgui_input);
	custom_imgui_result perform_custom_imgui(perform_custom_imgui_input);

	void apply(const config_lua_table&);

	setup_escape_result escape();

	auto get_inv_tickrate() const {
		return get_viewed_cosmos().get_fixed_delta().in_seconds<double>();
	}

	template <class C>
	void advance(
		const setup_advance_input& in,
		const C& callbacks
	) {
		global_time_seconds += in.frame_delta.in_seconds();

		(void)in;
		(void)callbacks;
	}

	template <class T>
	void control(const T&) {}

	void accept_game_gui_events(const game_gui_entropy_type&) {}

	std::optional<camera_eye> find_current_camera_eye() const {
		return get_camera_eye();
	}

	augs::path_type get_unofficial_content_dir() const;

	auto get_render_layer_filter() const {
		return render_layer_filter::disabled();
	}

	void draw_custom_gui(const draw_setup_gui_input&);

	void ensure_handler() {}
	bool requires_cursor() const { return false; }

private:
	entropy_accumulator zero_entropy;
public:

	const entropy_accumulator& get_entropy_accumulator() const {
		return zero_entropy;
	}

	template <class F>
	void on_mode_with_input(F&&) const {}

	auto get_game_gui_subject_id() const {
		return get_viewed_character_id();
	}

	std::nullopt_t get_new_player_metas() {
		return std::nullopt;
	}

	const arena_player_metas* find_player_metas() const {
		return nullptr;
	}

	void after_all_drawcalls(game_frame_buffer&) {}
	void do_game_main_thread_synced_op(renderer_backend_result&) {}

	bool is_mover_active() const {
		return mover.is_active(history);
	}
};