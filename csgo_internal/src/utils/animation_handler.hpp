#pragma once
#include "../base_includes.hpp"

namespace utils {
	class animation_helper_t {
	private:
		float m_animation_speed{};
		float m_last_time{};

	public:
		void update();

		__forceinline float get_speed(float scale = 4.f) { return m_animation_speed * scale; }

		__forceinline void lerp(float& value, float target, float scale = 4.f) {
			if (value > target)
				value = std::max<float>(value - get_speed(scale), target);
			else if (value < target)
				value = std::min<float>(value + get_speed(scale), target);
		}

		__forceinline void ease_lerp(float& value, float target, float scale = 4.f) {
			return lerp(value, target, std::abs(value - target) * scale);
		}
	};
} // namespace utils

GLOBAL_DYNPTR(utils::animation_helper_t, animations_pt);
GLOBAL_DYNPTR(utils::animation_helper_t, animations_fsn);
GLOBAL_DYNPTR(utils::animation_helper_t, animations_direct);