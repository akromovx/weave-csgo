#include "../../game/engine_prediction.hpp"
#include "../../game/override_entity_list.hpp"
#include "../../globals.hpp"
#include "../../render.hpp"
#include "../../utils/hotkeys.hpp"
#include "../movement.hpp"
#include "exploits.hpp"
#include "hvh.hpp"
#include "peek_state.hpp"

namespace hvh {
	using namespace sdk;

	enum antiaim_type {
		_def,
		stand,
		move,
		jump,
		slowwalk,
		crouch,
		defensive,
		antiaim_trigger_max
	};

	struct {
		int last_update{};
		bool jitter_switch{};
		float spin_angle{};
	} antiaim_temp_info[antiaim_trigger_max]{};

	STFI std::optional<incheat_vars::antiaim_settings_t> get_current_settings(user_cmd_t* cmd) {
		auto& set = settings->antiaim.triggers;

		if (set[defensive].override_default && exploits->m_breaking_lc && !peek_state->m_defensive_lag) {
			antiaim->m_trigger_id = defensive;
			return set[antiaim->m_trigger_id];
		}

		if (globals->m_groundtick > 1) {
			if (set[crouch].override_default && (cmd->m_buttons.has(in_duck) || globals->m_fake_duck)) {
				antiaim->m_trigger_id = crouch;
				return set[antiaim->m_trigger_id];
			}

			if (set[slowwalk].override_default && hotkeys->slow_walk.m_active) {
				antiaim->m_trigger_id = slowwalk;
				return set[antiaim->m_trigger_id];
			}

			if (globals->m_local->velocity().length_sqr() > 25.f) {
				if (set[move].override_default) {
					antiaim->m_trigger_id = move;
					return set[antiaim->m_trigger_id];
				}
			} else {
				if (set[stand].override_default) {
					antiaim->m_trigger_id = stand;
					return set[antiaim->m_trigger_id];
				}
			}
		} else {
			if (set[jump].override_default) {
				antiaim->m_trigger_id = jump;
				return set[antiaim->m_trigger_id];
			}
		}

		antiaim->m_trigger_id = _def;
		return set[antiaim->m_trigger_id];
	}

	void antiaim_t::setup_settings(incheat_vars::antiaim_settings_t& set, int& side) {
	
	}

	__forceinline cs_player_t* get_best_player(bool only_on_screen) {
		cs_player_t* best_player = nullptr;
		float best_distance = FLT_MAX;
		vec2d screen_center = vec2d{ render::screen_width / 2.f, render::screen_height / 2.f };

		for (auto& [player, entry]: entities->m_players) {
			if (!player->is_alive() || player->is_teammate() || player == globals->m_local || player->dormant())
				continue;

			vec2d origin_2d{};
			if (!math::world_to_screen(player->origin(), origin_2d) && only_on_screen)
				continue;

			if (only_on_screen && (origin_2d.x < 0.f || origin_2d.x > render::screen_width || origin_2d.y < 0.f || origin_2d.y > render::screen_height))
				continue;

			const float dist = origin_2d.dist_to_sqr(screen_center);
			if (dist < best_distance) {
				best_player = player;
				best_distance = dist;
			}
		}

		return best_player;
	}

	std::optional<float> antiaim_t::get_at_target_yaw(bool only_on_screen) {
		auto best_player = get_best_player(only_on_screen);
		if (best_player != nullptr)
			return math::calculate_angle(globals->m_local->origin(), best_player->origin()).y;

		return 0.0f;
	}

	bool antiaim_t::can_work(sdk::user_cmd_t* cmd) {
		switch (globals->m_local->movetype()) {
			case type_ladder:
			case type_noclip:
			case type_observer:
				return false;
		}

		if (interfaces::game_rules->is_freeze_time() || globals->m_local->flags().has(fl_frozen))
			return false;

		if (exploits->m_in_shift)
			return false;

		if (globals->m_weapon->item_definition_index() == e_weapon_type::weapon_revolver) {
			if ((aimbot->m_revolver_fire && cmd->m_buttons.has(in_attack)) || cmd->m_buttons.has(in_use))
				return false;
		} else {
			if (is_shooting(cmd) || cmd->m_buttons.has(in_use))
				return false;
		}

		return true;
	}

	void antiaim_t::apply(user_cmd_t* cmd) {
		if (!settings->antiaim.enable)
			return;

		if (!can_work(cmd))
			return;

		m_settings = get_current_settings(cmd);
		if (!m_settings.has_value())
			return;

		int side = hotkeys->desync_inverter.m_active ? 1 : -1;
		setup_settings(*m_settings, side);

		cmd->m_viewangles = engine_prediction->m_unpredicted_data.m_cmd.m_viewangles;

		if (antiaim_temp_info[m_trigger_id].last_update != cmd->m_command_number && *globals->m_send_packet) {
			antiaim_temp_info[m_trigger_id].jitter_switch = !antiaim_temp_info[m_trigger_id].jitter_switch;
			antiaim_temp_info[m_trigger_id].last_update = cmd->m_command_number;
		}

		if (m_settings->force_off)
			return;

		switch (m_settings->pitch) {
			case 0: cmd->m_viewangles.x = 89.f; break;
			case 1: cmd->m_viewangles.x = math::random_float(-89.f, 89.f); break;
			case 2: cmd->m_viewangles.x = antiaim_temp_info[m_trigger_id].jitter_switch ? -89.f : 89.f; break;
			default: break;
		}

		if (hotkeys->manual_right.m_active)
			cmd->m_viewangles.y += 90.f;
		else if (hotkeys->manual_left.m_active)
			cmd->m_viewangles.y -= 90.f;
		else if (hotkeys->manual_forward.m_active)
			cmd->m_viewangles.y += 180.f;

		auto manuals_work = hotkeys->manual_right.m_active || hotkeys->manual_left.m_active ||
							hotkeys->manual_back.m_active || hotkeys->manual_forward.m_active;

		auto best_player = get_best_player(m_settings->align_by_target == 1);
		if (best_player != nullptr) {
			auto angle = math::calculate_angle(globals->m_local->origin(), best_player->origin());
			if (m_settings->align_by_target && !manuals_work)
				cmd->m_viewangles.y = angle.y;
		}

		const auto freestand = m_freestand_side != 0 && hotkeys->freestand.m_active;

		if (m_settings->ignore_manuals)
			manuals_work = false;

		if (!manuals_work && freestand)
			cmd->m_viewangles.y += m_freestand_side * 90.f;

		manuals_work = manuals_work || (!m_settings->ignore_freestand && freestand);

		const auto abs_jitter_angle = std::abs(m_settings->jitter_angle);

		if (!manuals_work) {
			cmd->m_viewangles.y += math::normalize_yaw(180.f + m_settings->yaw_add);

			if (m_settings->desync_jitter)
				side *= antiaim_temp_info[m_trigger_id].jitter_switch ? 1 : -1;

			if (m_settings->randomize_jitter && abs_jitter_angle > 1) {
				cmd->m_viewangles.y += abs_jitter_angle - (rand() % abs_jitter_angle) * 2.f;
			} else
				cmd->m_viewangles.y += m_settings->jitter_angle * (antiaim_temp_info[m_trigger_id].jitter_switch ? 1.f : -1.f);

			if (m_settings->spin) {
				if (antiaim_temp_info[m_trigger_id].spin_angle > m_settings->spin_range)
					antiaim_temp_info[m_trigger_id].spin_angle = std::clamp(
							antiaim_temp_info[m_trigger_id].spin_angle - m_settings->spin_range + m_settings->spin_speed / 10.f, 0.f, (float)m_settings->spin_range);
				else
					antiaim_temp_info[m_trigger_id].spin_angle += m_settings->spin_speed / 10.f;

				cmd->m_viewangles.y += math::normalize_yaw(m_settings->spin_range - antiaim_temp_info[m_trigger_id].spin_angle * 2.f);
			}
		} else
			cmd->m_viewangles.y += 180.f;

		if (m_settings->desync_amount > 0) {
			const float max_body_rotation = globals->m_local->max_desync();
			float current_body_rotation = m_settings->desync_amount / 100.f * max_body_rotation;

			float last_angle = math::normalize_yaw(cmd->m_viewangles.y + current_body_rotation * side);
			float eye_feet_delta = math::normalize_yaw(players->m_local_player.m_last_goal_feet_yaw - last_angle);

			float fake_angle = cmd->m_viewangles.y;

			if (current_body_rotation < max_body_rotation && std::abs(eye_feet_delta) < max_body_rotation) {
				if (eye_feet_delta > 0.f)
					fake_angle = math::normalize_yaw(last_angle - max_body_rotation);
				else
					fake_angle = math::normalize_yaw(last_angle + max_body_rotation);
			} else
				fake_angle = math::normalize_yaw(last_angle + max_body_rotation * side);

			if (!(*globals->m_send_packet)) {
				//cmd->m_viewangles.y += 180.f;
				cmd->m_viewangles.y = fake_angle;
			}
			else {
				if (m_settings->align_desync)
					cmd->m_viewangles.y += players->m_local_player.m_desync * 0.5f;
			}
		}

		// normalize yaw, we don't want to get untrusted
		cmd->m_viewangles.y = math::normalize_yaw(cmd->m_viewangles.y);
		m_last_applied_tick = cmd->m_command_number;
	}
} // namespace hvh