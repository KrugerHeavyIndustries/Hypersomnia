#pragma once
#include "augs/templates/maybe_const.h"

#include "game/transcendental/entity_handle_declaration.h"
#include "game/transcendental/entity_id.h"
#include "game/enums/slot_function.h"

#include "game/detail/inventory/inventory_slot_handle_declaration.h"
#include "game/detail/inventory/inventory_slot_id.h"

#include "game/components/transform_component.h"

struct inventory_slot;

class cosmos;

template <bool is_const>
class basic_inventory_slot_handle {
	using owner_reference = maybe_const_ref_t<is_const, cosmos>;
	using slot_reference = maybe_const_ref_t<is_const, inventory_slot>;
	using slot_pointer = maybe_const_ptr_t<is_const, inventory_slot>;

	using entity_handle_type = basic_entity_handle<is_const>;

public:
	basic_inventory_slot_handle(owner_reference owner, const inventory_slot_id raw_id);
	
	owner_reference get_cosmos() const;

	owner_reference owner;
	inventory_slot_id raw_id;

	void unset();

	slot_reference get() const;
	slot_reference operator*() const;
	slot_pointer operator->() const;

	bool alive() const;
	bool dead() const;

	bool can_contain(const entity_id) const;

	entity_handle_type get_item_if_any() const;
	entity_handle_type get_container() const;

	entity_handle_type get_first_ancestor_with_body_connection() const;

	entity_handle_type get_root_container() const;
	entity_handle_type get_root_container_until(const entity_id container_entity) const;

	bool is_child_of(const entity_id container_entity) const;

	const std::vector<entity_id>& get_items_inside() const;

	bool has_items() const;
	bool is_empty_slot() const;

	bool is_hand_slot() const;
	size_t get_hand_index() const;

	float calculate_density_multiplier_due_to_being_attached() const;

	unsigned calculate_local_space_available() const;
	unsigned calculate_real_space_available() const;

	bool is_physically_connected_until(const entity_id until_parent = entity_id()) const;

	inventory_slot_id get_id() const;
	operator inventory_slot_id() const;
	operator basic_inventory_slot_handle<true>() const;

	operator bool() const {
		return alive();
	}
};

const std::vector<entity_id>& get_items_inside(const const_entity_handle h, const slot_function s);

template <bool C>
inline basic_inventory_slot_handle<C>::basic_inventory_slot_handle(owner_reference owner, const inventory_slot_id raw_id) : owner(owner), raw_id(raw_id) {}

template <bool C>
inline typename basic_inventory_slot_handle<C>::owner_reference basic_inventory_slot_handle<C>::get_cosmos() const {
	return owner;
}

template <bool C>
inline typename basic_inventory_slot_handle<C>::slot_pointer basic_inventory_slot_handle<C>::operator->() const {
	return &get_container().template get<components::container>().slots.at(raw_id.type);
}

template <bool C>
inline typename basic_inventory_slot_handle<C>::slot_reference basic_inventory_slot_handle<C>::operator*() const {
	return *operator->();
}

template <bool C>
inline typename basic_inventory_slot_handle<C>::slot_reference basic_inventory_slot_handle<C>::get() const {
	return *operator->();
}

template <bool C>
inline bool basic_inventory_slot_handle<C>::alive() const {
	if (get_container().dead()) {
		return false;
	}

	const auto* const container = get_container().template find<components::container>();

	return container && container->slots.find(raw_id.type) != container->slots.end();
}

template <bool C>
inline bool basic_inventory_slot_handle<C>::dead() const {
	return !alive();
}

template <bool C>
inline inventory_slot_id basic_inventory_slot_handle<C>::get_id() const {
	return raw_id;
}

template <bool C>
inline basic_inventory_slot_handle<C>::operator inventory_slot_id() const {
	return get_id();
}

template <bool C>
inline basic_inventory_slot_handle<C>::operator basic_inventory_slot_handle<true>() const {
	return basic_inventory_slot_handle<true>(owner, raw_id);
}

template <class C>
auto subscript_handle_getter(C& cosm, const inventory_slot_id id) {
	return basic_inventory_slot_handle<std::is_const_v<C>>{ cosm, id };
}