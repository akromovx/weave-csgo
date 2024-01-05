#pragma once
#include <cstdint>

namespace sdk {
	struct base_handle_t {
	private:
		using handle_t = uint32_t;
		handle_t m_value{};

	public:
		__forceinline auto& value() { return m_value; }
		__forceinline base_handle_t(handle_t handle) : m_value(handle){};

		void* get();
	};
} // namespace sdk