#include "../../globals.hpp"
#include "../../interfaces.hpp"
#include "exploits.hpp"
#include "hvh.hpp"
#include "peek_state.hpp"

namespace hvh {
	using namespace sdk;

	STFI bool determine_fakelag(int target_choke) {
		return interfaces::client_state->m_choked_commands >= target_choke;
	}

	STFI bool fakelag_trigger(user_cmd_t* cmd) {
		// we must reset current lag while peeking
		for (auto& p: peek_state->m_players) {
			if (p.m_need_reset && determine_fakelag(1)) {
				p.m_need_reset = false;
				return false;
			}

			if (p.m_has_damage && !p.m_has_damage_old) {
				if (!determine_fakelag(1)) {
					p.m_need_reset = true;
					return false;
				}

				return false;
			}

			if (p.m_has_damage)
				return true;
		}

		static float last_speed{};
		static vec3d last_velocity{};
		static float last_trigger_time{};
		static int last_freestand_side{};

		constexpr auto angle = DEG2RAD(2);

		vec3d velocity = globals->m_local->velocity();
		if (std::abs(velocity.x) < 1.f) velocity.x = 0.f;
		if (std::abs(velocity.y) < 1.f) velocity.y = 0.f;
		if (std::abs(velocity.z) < 1.f) velocity.z = 0.f;

		auto speed = velocity.length();
		float time_delta = std::abs(last_trigger_time - interfaces::global_vars->m_curtime);

		bool ret = false;

		float angle_diff = std::abs(std::atan2(velocity.y, velocity.x) - std::atan2(last_velocity.y, last_velocity.x));
		angle_diff = std::remainderf(angle_diff, fPI);

		if (angle_diff > angle)
			ret = true;

		if (std::abs(speed - last_speed) > 2.f)
			ret = true;

		if (time_delta > 0.2f && time_delta < 0.4f)
			ret = true;

		if (ret && time_delta > 0.4f)
			last_trigger_time = interfaces::global_vars->m_curtime;

		if (last_freestand_side != antiaim->m_freestand_side)
			ret = false;

		last_freestand_side = antiaim->m_freestand_side;
		last_velocity = velocity;
		last_speed = speed;

		return ret;
	}

	void fakelag_t::on_create_move(user_cmd_t* cmd) {
		if (exploits->m_in_shift)
			return;

		if (interfaces::game_rules->is_freeze_time() || globals->m_local->flags().has(fl_frozen))
			return;

		if (settings->antiaim.fakelag == 0) {
			if (antiaim->m_settings.has_value() && antiaim->m_settings->desync_amount > 0)
				*globals->m_send_packet = determine_fakelag(1);
			return;
		}

		if (exploits->m_last_shift_tick == interfaces::client_state->get_current_tick()) {
			*globals->m_send_packet = determine_fakelag(1);
			return;
		}

		auto max_fakelag = determine_fakelag(MAX_FAKELAG);
		auto semi_fakelag = determine_fakelag(1);
		auto min_fakelag = determine_fakelag(1);

		if (exploits->m_recharge_ticks > 0 || exploits->m_charged) {
			max_fakelag = min_fakelag;

			if (exploits->m_charged && exploits->m_type == exploits_t::e_type::hideshot) {
				max_fakelag = semi_fakelag;
			}
		} else {
			if (m_need_reset_shot || m_last_tick_fired) {
				max_fakelag = min_fakelag;
				if (max_fakelag) {
					if (m_last_tick_fired)
						m_last_tick_fired = false;
					else
						m_need_reset_shot = false;
				}
			} else {
				auto shot = is_shooting(cmd);

				if (settings->ragebot.enable && globals->m_weapon->item_definition_index() == e_weapon_type::weapon_revolver)
					shot = cmd->m_buttons.has(in_attack) && hvh::aimbot->m_revolver_fire;

				if (shot) {
					max_fakelag = min_fakelag;
					if (!max_fakelag)
						m_last_tick_fired = true;
				}
			}
		}

		switch (settings->antiaim.fakelag) {
			case 1: *globals->m_send_packet = max_fakelag; break;
			case 2: *globals->m_send_packet = fakelag_trigger(cmd) ? max_fakelag : min_fakelag; break;
		}
	}
} // namespace hvh