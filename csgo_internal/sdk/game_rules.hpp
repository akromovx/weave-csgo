#pragma once
#include "../src/utils/displacement.hpp"

namespace sdk {
	struct view_vectors_t {
		vec3d m_view{};

		vec3d m_hull_min{};
		vec3d m_hull_max{};

		vec3d m_duck_hull_min{};
		vec3d m_duck_hull_max{};
		vec3d m_duck_view{};

		vec3d m_obs_hull_min{};
		vec3d m_obs_hull_max{};

		vec3d m_dead_view_height{};
	};

	struct cs_game_rules_t {
		OFFSET(is_freeze_time, bool, 0x20);
		OFFSET(is_valve_ds, bool, 0x7C);

		VFUNC(get_view_vectors(), view_vectors_t*(__thiscall*)(void*), 30);
	};
} // namespace sdk