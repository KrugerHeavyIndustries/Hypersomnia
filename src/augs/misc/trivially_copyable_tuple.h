#pragma once
#include "augs/ensure.h"

#include "augs/templates/type_list.h"
#include "augs/templates/constexpr_arithmetic.h"
#include "augs/templates/type_matching_and_indexing.h"

namespace augs {
	template <class... Types>
	class trivially_copyable_tuple {
		alignas(constexpr_max_v<std::size_t, alignof(Types)...>) char buf[sum_sizes_of_types_in_list_v<type_list<Types...>>];
		
		template <class type>
		void init() {
			static constexpr std::size_t offset = sum_sizes_until_nth_v<index_in_v<type, Types...>, type_list<Types...>>;
			
			auto* p = data() + offset;
			auto* field = reinterpret_cast<const type*>(p);

			ensure(p >= reinterpret_cast<char*>(buf));
			ensure(p + sizeof(type) <= buf + sum_sizes_of_types_in_list_v<type_list<Types...>>);

			new (p) type;
		}

	public:

		static_assert(std::conjunction_v<std::is_trivially_copyable<Types>...>, "One of the tuple types is not trivially copyable!");

		trivially_copyable_tuple() {
			(init<Types>(), ...);
		}

		char* data() {
			return buf;
		}

		const char* data() const {
			return buf;
		}
	};
}

namespace std {
	template <std::size_t I, class... Types>
	auto& get(augs::trivially_copyable_tuple<Types...>& t) {
		static constexpr std::size_t offset = sum_sizes_until_nth_v<I, type_list<Types...>>;
		using type = nth_type_in_t<I, Types...>;

		return *reinterpret_cast<type*>(t.data() + offset);
	}

	template <std::size_t I, class... Types>
	const auto& get(const augs::trivially_copyable_tuple<Types...>& t) {
		static constexpr std::size_t offset = sum_sizes_until_nth_v<I, type_list<Types...>>;
		using type = nth_type_in_t<I, Types...>;

		return *reinterpret_cast<const type*>(t.data() + offset);
	}
	
	template<class T, class... Types>
	auto& get(augs::trivially_copyable_tuple<Types...>& t) {
		return get<index_in_v<T, Types...>>(t);
	}
	
	template<class T, class... Types>
	const auto& get(const augs::trivially_copyable_tuple<Types...>& t) {
		return get<index_in_v<T, Types...>>(t);
	}
}