#pragma once
#if STATICALLY_ALLOCATE_ASSETS

#include "augs/misc/enum/enum_map.h"
#include "game/assets/ids/asset_id_declarations.h"
#include "game/container_sizes.h"
#include "augs/misc/pool/pool.h"
#include "augs/misc/declare_containers.h"

template <class enum_key, class mapped>
using asset_map = augs::enum_map<enum_key, mapped>;

template <class pooled_type>
using image_id_pool = augs::pool<
	pooled_type, 
	of_size<MAX_IMAGE_COUNT>::make_constant_vector,
   	asset_pool_id_size_type
>;

#else

template <class enum_key, class mapped>
using asset_map = std::unordered_map<enum_key, mapped>;

template <class pooled_type>
using image_id_pool = augs::pool<
	pooled_type, 
	make_vector,
   	asset_pool_id::used_size_type
>;

#endif