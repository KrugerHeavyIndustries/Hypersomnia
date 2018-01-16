#include "augs/templates/algorithm_templates.h"
#include "augs/templates/container_templates.h"
#include "game/detail/physics/physics_queries.h"
#include "game/detail/visible_entities.h"

#include "game/transcendental/cosmos.h"
#include "game/components/render_component.h"

#include "game/enums/filters.h"
#include "game/detail/physics/physics_scripts.h"

#include "game/inferred_caches/tree_of_npo_cache.h"
#include "game/inferred_caches/physics_world_cache.h"

static void get_visible_per_layer(
	const cosmos& cosmos,
	const visible_entities::all_type& entities,
	visible_entities::per_layer_type& output_layers
) {
	if (entities.empty()) {
		return;
	}

	for (const auto it_id : entities) {
		const auto it = cosmos[it_id];

#if ENABLE_ENSURE
		if (!it.find_def<definitions::render>()) {
			LOG("WARNING! %x has no render definition.", it.get_type_id());
		}
#endif
		const auto layer = it.get_def<definitions::render>().layer;
		// ensure(layer < static_cast<render_layer>(output_layers.size()));
		output_layers[layer].push_back(it);
	}

	auto& car_interior_layer = output_layers[render_layer::CAR_INTERIOR];

	if (car_interior_layer.size() > 1) {
		sort_range(
			car_interior_layer, 
			[&cosmos](const auto b, const auto a) {
				return are_connected_by_friction(cosmos[a], cosmos[b]);
			}
		);
	}
}

visible_entities::visible_entities(const visible_entities_query input) {
	reacquire_all_and_sort(input);
}

void visible_entities::clear() {
	all.clear();

	for (auto& layer : per_layer) {
		layer.clear();
	}
}

void visible_entities::reacquire_all_and_sort(const visible_entities_query input) {
	clear();
	acquire_non_physical(input);
	acquire_physical(input);
	sort_per_layer(input.cosm);
}

void visible_entities::acquire_physical(const visible_entities_query input) {
	const auto& cosmos = input.cosm;
	const auto camera = input.cone;

	const auto& physics = cosmos.get_solvable_inferred().physics;

	thread_local std::unordered_set<entity_id> unique_from_physics;
	unique_from_physics.clear();

	if (input.exact) {
		const auto camera_aabb = camera.get_visible_world_rect_aabb(input.screen_size);
		
		physics.for_each_intersection_with_polygon(
			cosmos.get_si(),
			camera_aabb.get_vertices<real32>(),
			filters::renderable_query(),
			[&](const b2Fixture* const fix, auto, auto) {
				unique_from_physics.insert(cosmos.make_versioned(get_entity_that_owns(fix)));
				return callback_result::CONTINUE;
			}
		);
	}
	else {
		physics.for_each_in_camera(
			cosmos.get_si(),
			camera,
			input.screen_size,
			[&](const b2Fixture* const fix) {
				unique_from_physics.insert(cosmos.make_versioned(get_entity_that_owns(fix)));
				return callback_result::CONTINUE;
			}
		);
	}

	concatenate(all, unique_from_physics);
}

void visible_entities::acquire_non_physical(const visible_entities_query input) {
	const auto& cosmos = input.cosm;
	const auto camera = input.cone;
	const auto camera_aabb = camera.get_visible_world_rect_aabb(input.screen_size);

	const auto& tree_of_npo = cosmos.get_solvable_inferred().tree_of_npo;
	
	tree_of_npo.for_each_in_camera(
		[&](const unversioned_entity_id id) {
			const auto versioned_id = cosmos.make_versioned(id);
			
			if (input.exact) {
				if (!camera_aabb.hover(cosmos[versioned_id].get_aabb())) {
					return;
				}
			}

			all.push_back(cosmos.make_versioned(id));
		},
		camera,
		input.screen_size,
		tree_of_npo_type::RENDERABLES
	);
}

void visible_entities::clear_dead_entities(const cosmos& cosm) {
	auto dead_deleter = [&cosm](const entity_id e) {
		return cosm[e].dead();
	};

	erase_if(all, dead_deleter);

	for (auto& layer : per_layer) {
		erase_if(layer, dead_deleter);
	}
}

void visible_entities::sort_per_layer(const cosmos& cosm) {
	get_visible_per_layer(cosm, all, per_layer);
}