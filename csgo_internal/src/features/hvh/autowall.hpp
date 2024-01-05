#pragma once
#include "../../base_includes.hpp"
#include "../../utils/vector.hpp"
#include "../../../sdk/trace_define.hpp"
#include "../../../sdk/engine_trace.hpp"
#include "../../game/players.hpp"

namespace hvh {
	struct autowall_t {
		struct penetration_t {
			float m_damage{}, m_min_damage{};
			sdk::e_hitbox m_hitbox{};
			sdk::e_hitgroup m_hitgroup{};
			vec3d m_impacts[10]{};
			uint8_t m_impact_count{};
			vec3d m_direction{}, m_end{};
			bool m_did_hit{};
		};

		penetration_t run(const vec3d& src, const vec3d& end, sdk::base_combat_weapon_t* weapon, lag_record_t* target, sdk::cs_player_t* override_player = nullptr, bool no_opt = false);

	private:
		penetration_t fire_bullet(sdk::cs_weapon_info_t* data, vec3d src, const vec3d& pos, sdk::i_trace_filter* filter, lag_record_t* target, bool is_zeus, bool no_opt);
	};

	GLOBAL_DYNPTR(autowall_t, autowall);
} // namespace hvh