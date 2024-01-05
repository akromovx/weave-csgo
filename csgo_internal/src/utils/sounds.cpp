#include "sounds.hpp"
#include "../base_includes.hpp"

namespace utils {
	STFI void modify_volume_sound(char* bytes, ptrdiff_t file_size, float volume) {
		int offset = 0;
		for (int i = 0; i < file_size / 2; i++) {
			if (bytes[i] == 'd' && bytes[i + 1] == 'a' && bytes[i + 2] == 't' && bytes[i + 3] == 'a') {
				offset = i;
				break;
			}
		}

		if (offset == 0)
			return;

		char* data_offset = (bytes + offset);
		DWORD sample_bytes = *(DWORD*)(data_offset + 4);
		DWORD samples = sample_bytes / 2;

		SHORT* sample = (SHORT*)(data_offset + 8);

		for (DWORD i = 0; i < samples; i++) {
			SHORT sh_sample = *sample;
			sh_sample = (SHORT)(sh_sample * volume);
			*sample = sh_sample;
			sample++;
			if (((char*)sample) >= (bytes + file_size - 1))
				break;
		}
	}

	__forceinline void play_sound_from_memory(uint8_t* bytes, size_t size, float volume) {
		if (GetForegroundWindow() != ctx->hwnd)
			return;

		static std::unordered_map<uint8_t*, std::pair<std::vector<uint8_t>, float>> cache{};
		if (cache.count(bytes) == 0)
			cache[bytes].first.resize(size);

		auto& current = cache[bytes];
		uint8_t* stored_bytes = current.first.data();

		// only modify sound when changed volume
		if (current.second != volume) {
			std::memcpy(stored_bytes, bytes, size);
			current.second = volume;
			modify_volume_sound((char*)stored_bytes, size, volume);
		}

		PlaySoundA((char*)stored_bytes, NULL, SND_ASYNC | SND_MEMORY);
	}
} // namespace utils