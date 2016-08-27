#pragma once

enum class network_command {
	COMPLETE_STATE,
	ENTROPY_FOR_NEXT_STEP,
	ENTROPY_WITH_HEARTBEAT_FOR_NEXT_STEP,

	COUNT
};

static_assert(static_cast<int>(network_command::COUNT) < 256, "network commands do not fit in one byte!");

namespace augs {
	template<class A>
	auto read_object(A& ar, network_command& storage) {
		unsigned char compressed;
		auto result = read_object(ar, compressed);

		storage = static_cast<network_command>(compressed);

		return result;
	}

	template<class A, class T, class...>
	void write_object(A& ar, const network_command& storage) {
		unsigned char compressed = static_cast<network_command>(storage);

		write_object(ar, compressed);
	}
}