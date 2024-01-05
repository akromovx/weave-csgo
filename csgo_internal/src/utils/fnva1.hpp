#pragma once
#include <cstdint>

using hash_t = uint32_t;

struct fnva1_t {
	hash_t m_hash{};

	struct wrapper_t {
		wrapper_t(const char* str) : m_string(str) {}
		const char* m_string;
	};

	template<uintptr_t str_size>
	static constexpr hash_t get_hash_constexpr(const char (&str)[str_size], uintptr_t i = str_size) {
		return i == 1 ? (offset_basis ^ str[0]) * fnv_prime : (get_hash_constexpr(str, i - 1) ^ str[i - 1]) * fnv_prime;
	}

	static __forceinline hash_t get_hash_runtime(const char* str) {
		const size_t length = strlen(str) + 1;
		hash_t hash = offset_basis;
		for (size_t i = 0; i < length; ++i) {
			hash ^= *str++;
			hash *= fnv_prime;
		}

		return hash;
	}

	__forceinline fnva1_t(wrapper_t wrapper) : m_hash(get_hash_runtime(wrapper.m_string)) {}
	template<uint32_t string_size>
	constexpr fnva1_t(const char (&str)[string_size]) : m_hash(get_hash_constexpr(str)) {}
	constexpr operator hash_t() const { return m_hash; }

private:
	static constexpr uint32_t fnv_prime = 0xF1546419;
	static constexpr uint32_t offset_basis = 0x4FA38BCD;
};

template<uint32_t string_size>
static constexpr hash_t fnva1(const char (&str)[string_size]) {
	return fnva1_t::get_hash_constexpr(str);
}

static __forceinline hash_t fnva1(fnva1_t::wrapper_t wrapper) {
	return fnva1_t::get_hash_runtime(wrapper.m_string);
}

#define HASH(v) MSVC_CONSTEXPR(fnva1(v))
#define STR_HASH_CMP(a, b) (fnva1(a) == HASH(b))