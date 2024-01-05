#pragma once
#include "esp_utils.hpp"
#include "../../utils/vector.hpp"
#include "../../utils/matrix.hpp"

namespace esp {
	class players_t {
	public:
		void render();

		__forceinline std::pair <color_t, color_t> get_health_color(int health, float alpha) {
			float health_multiplier = 12.f / 360.f * (std::ceil(std::clamp(health, 0, 100) / 10.f) - 1.f);
			if (settings->player_esp.override_health_color)
				return { settings->player_esp.health.colors[0].get().modify_alpha(alpha), settings->player_esp.health.colors[1].get().modify_alpha(alpha) };

			auto clr = color_t::hsb(health_multiplier, 1.f, 1.f).modify_alpha(alpha);
			return { clr, clr.decrease(10) };
		}
	};

	GLOBAL_DYNPTR(esp::players_t, players);
}