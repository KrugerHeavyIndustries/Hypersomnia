#pragma once
#include "game/cosmos/entity_handle.h"

template <class E, class F>
auto for_each_fixture(const E& handle, F callback) -> decltype(callback(std::declval<b2Fixture&>())) {
	const auto& physics = handle.get_cosmos().get_solvable_inferred().physics;

	if (const auto ch = physics.find_colliders_cache(handle.get_id())) {
		for (const auto& f : ch->constructed_fixtures) {
			decltype(auto) result = callback(*f);

			if (result != std::nullopt) {
				return result;
			}
		}
	}

	return std::nullopt;
}

inline auto shape_overlaps_fixture(
	const b2Shape* const shape,
	const si_scaling si,
	const vec2 shape_pos,
	const b2Fixture& fixture
) -> std::optional<b2TestOverlapOutput> {
	const auto queried_shape_transform = b2Transform(b2Vec2(si.get_meters(shape_pos)), b2Rot(0.f));

	constexpr auto index_a = 0;
	constexpr auto index_b = 0;

	if (const auto result = b2TestOverlapInfo(
		shape,
		index_a,
		fixture.GetShape(),
		index_b,
		queried_shape_transform,
		fixture.GetBody()->GetTransform()
	); result.overlap) {
		return result;
	}

	return std::nullopt;
}

template <class E>
auto shape_overlaps_entity(
	const b2Shape* const shape,
	const vec2 shape_pos,
	const E& handle
) {
	const auto si = handle.get_cosmos().get_si();

	return for_each_fixture(handle, [&](const b2Fixture& fixture) -> std::optional<b2TestOverlapOutput> {
		if (const auto result = shape_overlaps_fixture(
			shape,
			si,
			shape_pos,
			fixture
		); result->overlap) {
			return result;
		}

		return std::nullopt;
	});
}

template <class E>
auto circular_sector_overlaps_entity(
	const vec2 shape_pos,
	const real32 radius,
	const real32 sector_angle,
	const real32 sector_spread_degrees,
	const E& handle
) -> std::optional<b2TestOverlapOutput> {
	const auto si = handle.get_cosmos().get_si();

	{
		b2CircleShape shape;
		shape.m_radius = si.get_meters(radius);

		if (const auto result = shape_overlaps_entity(
			&shape,
			shape_pos,
			handle
		); result == std::nullopt) {
			return std::nullopt;
		}
	}

	const auto& rot = sector_angle;
	const auto& a = sector_spread_degrees;
	const auto& r = radius;

	b2Vec2 v[4];
	v[0] = b2Vec2();
	v[1] = b2Vec2(r * vec2::from_degrees(rot - a / 2));
	v[2] = b2Vec2(r * vec2::from_degrees(rot));
	v[3] = b2Vec2(r * vec2::from_degrees(rot + a / 2));

	for (auto& vv : v) {
		vv = si.get_meters(vv);
	}

	b2PolygonShape shape;
	shape.Set(v, 4);

	if (const auto result = shape_overlaps_entity(
		&shape,
		shape_pos,
		handle
	); result && result->overlap) {
		return result;
	}

	return std::nullopt;
}

template <class A, class B>
auto use_button_overlaps(
	const A& handle,
	const B& other
) -> std::optional<b2TestOverlapOutput> {
	const auto& sentience_def = handle.template get<invariants::sentience>();
	const auto tr = handle.get_logic_transform();

	if (const auto result = circular_sector_overlaps_entity(
		tr.pos,
		sentience_def.use_button_radius,
		tr.rotation,
		sentience_def.use_button_angle,
		other
	)) {
		LOG("OVERLAPS");
		return result;
	}
	LOG("NOT OVERLAPS");

	return std::nullopt;
}
