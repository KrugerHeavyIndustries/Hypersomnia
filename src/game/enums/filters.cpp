#include "game/enums/filters.h"
#include "augs/misc/enum/enum_bitset.h"

using C = filter_category;

const predefined_filters filters;

template <class... Args>
static auto make_flags(Args... enums) {
	return static_cast<uint16>(augs::enum_bitset<C>(enums...).to_ulong());
}

static auto standard_participation_bitset() {
	return augs::enum_bitset<C>(C::QUERY, C::BULLET, C::WALL, C::CHARACTER, C::LYING_ITEM, C::GROUND, C::FLYING, C::TRIGGER, C::SHELL, C::GLASS_OBSTACLE);
}

static auto standard_participation() {
	return static_cast<uint16>(standard_participation_bitset().to_ulong());
}

template <class... C>
static auto standard_participation_except(C... c) {
	auto r = standard_participation_bitset();
	(r.reset(c), ...);
	return static_cast<uint16>(r.to_ulong());
}

namespace predefined_queries {
	b2Filter line_of_sight() {
		b2Filter out;
		out.categoryBits = make_flags(C::QUERY);
		out.maskBits = make_flags(C::WALL);
		return out;
	}

	b2Filter crosshair_laser() {
		b2Filter out;
		out.categoryBits = make_flags(C::QUERY);
		out.maskBits = make_flags(C::WALL, C::GLASS_OBSTACLE, C::CHARACTER);
		return out;
	}

	b2Filter crosshair_laser_except_characters() {
		b2Filter out;
		out.categoryBits = make_flags(C::QUERY);
		out.maskBits = make_flags(C::WALL, C::GLASS_OBSTACLE);
		return out;
	}

	b2Filter melee_query() {
		b2Filter out;
		out.categoryBits = make_flags(C::QUERY);
		out.maskBits = make_flags(C::WALL, C::GLASS_OBSTACLE, C::CHARACTER, C::LYING_ITEM, C::FLYING);
		return out;
	}

	b2Filter melee_solid_obstacle_query() {
		b2Filter out;
		out.categoryBits = make_flags(C::QUERY);
		out.maskBits = make_flags(C::WALL, C::GLASS_OBSTACLE);
		return out;
	}

	b2Filter pathfinding() {
		b2Filter out;
		out.categoryBits = make_flags(C::QUERY);
		out.maskBits = make_flags(C::WALL, C::GLASS_OBSTACLE);
		return out;
	}
	
	b2Filter force_explosion() {
		b2Filter out;
		out.categoryBits = make_flags(C::QUERY);
		out.maskBits = standard_participation_except(C::BULLET);
		return out;
	}

	b2Filter renderable() {
		b2Filter out;
		out.categoryBits = make_flags(C::QUERY);
		out.maskBits = make_flags(C::BULLET, C::CHARACTER, C::LYING_ITEM, C::WALL, C::GROUND, C::SHELL, C::GLASS_OBSTACLE, C::FLYING);
		return out;
	}
}

predefined_filters::predefined_filters() {
	{
		auto& out = filters[predefined_filter_type::WALL];
		out.categoryBits = make_flags(C::WALL);
		out.maskBits = standard_participation();
	}
	{

		auto& out = filters[predefined_filter_type::CHARACTER];
		out.categoryBits = make_flags(C::CHARACTER);
		out.maskBits = standard_participation_except();
	}
	{

		auto& out = filters[predefined_filter_type::GROUND];
		out.categoryBits = make_flags(C::GROUND);
		out.maskBits = standard_participation_except(C::FLYING);
	}
	{

		auto& out = filters[predefined_filter_type::LYING_ITEM];
		out.categoryBits = make_flags(C::LYING_ITEM);
		out.maskBits = standard_participation_except(C::CHARACTER);
	}
	{

		auto& out = filters[predefined_filter_type::FLYING_ITEM];
		out.categoryBits = make_flags(C::FLYING);
		out.maskBits = standard_participation_except(C::LYING_ITEM);
	}
	{

		auto& out = filters[predefined_filter_type::FLYING_BULLET];
		out.categoryBits = make_flags(C::BULLET);
		out.maskBits = standard_participation_except(C::BULLET);
	}
	{

		auto& out = filters[predefined_filter_type::SHELL];
		out.categoryBits = make_flags(C::SHELL);
		out.maskBits = standard_participation_except(C::BULLET, C::FLYING, C::TRIGGER);
	}
	{

		auto& out = filters[predefined_filter_type::PLANTED_EXPLOSIVE];
		out.categoryBits = make_flags(C::LYING_ITEM);
		out.maskBits = make_flags(C::QUERY, C::SHELL);
	}
	{

		auto& out = filters[predefined_filter_type::GLASS_OBSTACLE];
		out.categoryBits = make_flags(C::GLASS_OBSTACLE);
		out.maskBits = standard_participation();
	}
}

