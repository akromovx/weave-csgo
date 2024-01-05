#pragma once
#include <cstdint>

namespace utils {
	extern void play_sound_from_memory(uint8_t* bytes, size_t size, float volume = 1.f);
}