#pragma once
#include <cstdint>

static consteval uint32_t __consteval_rand32(uint32_t seed, uint32_t count) {
	for (auto i = 0u; i < count; ++i) {
		seed ^= seed << 13;
		seed ^= seed >> 17;
		seed ^= seed << 15;
	}

	return seed;
}

static consteval uint64_t __consteval_rand64(uint64_t seed, uint32_t count) {
	for (auto i = 0u; i < count; ++i) {
		seed ^= seed << 13;
		seed ^= seed >> 7;
		seed ^= seed << 17;
	}

	return seed;
}

#define CONSTEXPR_RAND32_SEED (uint32_t)(__TIME__[0] * __TIME__[1] * (__COUNTER__ + 1) + __LINE__ * __LINE__)
#define CONSTEXPR_RAND64_SEED (uint64_t)(CONSTEXPR_RAND32_SEED | ((uint64_t)CONSTEXPR_RAND32) << 32)

#define CONSTEXPR_RAND32 CONSTEXPR_RAND32_SEED //__consteval_rand32(CONSTEXPR_RAND32_SEED, __COUNTER__ + 1)
#define CONSTEXPR_RAND64 CONSTEXPR_RAND64_SEED //__consteval_rand64(CONSTEXPR_RAND64_SEED, __COUNTER__ + 1)