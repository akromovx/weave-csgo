#pragma once
#include "../macros.hpp"
#include "bits.hpp"
#include "constexpr_rand.hpp"
#include "fnva1.hpp"

static constexpr uint32_t xs32_random(uint32_t seed, int count) {
	for (int i = 0; i < count; ++i) {
		seed ^= seed << 13;
		seed ^= seed >> 17;
		seed ^= seed << 15;
	}

	return seed;
}

#define STR(x) \
	std::string { x }

#define STRC(x) (x)

#define STRS(x) \
	std::string { x }

#define STRSC(x) (x)

#define XOR64(x) (x)
#define XOR32(x) (x)
#define XOR16(x) (x)
#define XOR8(x) (x)

#define XOR64S(x) (x)
#define XOR32S(x) (x)
#define XOR16S(x) (x)
#define XOR8S(x) (x)