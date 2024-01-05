#pragma once
#include "../../base_includes.hpp"
#include "../../../sdk/input.hpp"

struct peek_state_t {
	struct {
		bool m_has_damage{};
		bool m_has_damage_old{};
		bool m_need_reset{};
		bool m_has_lethal_point{};

		int m_freestand_side{};
	} m_players[64];

	bool m_defensive_lag{};

	void on_create_move();
	void on_post_move();

private:
	__forceinline void reset() {
		for (auto& p : m_players) {
			p.m_has_damage = false;
			p.m_has_lethal_point = false;
			p.m_freestand_side = 0;
		}
	}
};

GLOBAL_DYNPTR(peek_state_t, peek_state);