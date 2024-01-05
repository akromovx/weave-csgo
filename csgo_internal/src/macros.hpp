#pragma once
#include <functional>

namespace macros {
	template<typename type_t>
	class set_and_restore {
		type_t* m_stored{};
		type_t m_backup{};

	public:
		template<typename _type_t>
		__forceinline set_and_restore(type_t* original, _type_t value) {
			m_stored = original;
			m_backup = *m_stored;
			*m_stored = value;
		}

		__forceinline ~set_and_restore() {
			*m_stored = std::move(m_backup);
		}
	};

	class defer_t {
		std::function<void()> m_callback{};

	public:
		__forceinline defer_t(std::function<void()> callback) : m_callback(callback){};
		__forceinline ~defer_t() { m_callback(); }
	};
} // namespace macros

#define CONCAT_INNER(a, b) a##b
#define CONCAT(a, b) CONCAT_INNER(a, b)
#define GEN_UNIQUE_NAME(base) CONCAT(base##_, __COUNTER__)

#define SET_AND_RESTORE(original, value) \
	auto GEN_UNIQUE_NAME(backup) = macros::set_and_restore{ &original, value }

#define DEFER(callback) \
	auto GEN_UNIQUE_NAME(defer) = macros::defer_t{ callback }

#define THREAD_SAFE(mtx) std::lock_guard GEN_UNIQUE_NAME(lock)(mtx)

#define CLEAR_THIS() std::memset(this, 0, sizeof(*this))

#define _PAD(count) \
	uint8_t GEN_UNIQUE_NAME(pad)[count] {}

#define MSVC_CONSTEXPR(v)         \
	([]() constexpr -> auto {     \
		constexpr auto value = v; \
		return value;             \
	}())

#define CONSTS(...) static constexpr auto __VA_ARGS__