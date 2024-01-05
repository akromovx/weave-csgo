#include "movement.hpp"
#include "../game/engine_prediction.hpp"
#include "../globals.hpp"
#include "../interfaces.hpp"
#include "../utils/hotkeys.hpp"
#include "hvh/exploits.hpp"
#include "hvh/hvh.hpp"

using namespace sdk;

void movement_t::peek_assist(user_cmd_t* cmd, vec3d& original_angle) {
	const auto origin = globals->m_local->origin(), velocity = globals->m_local->velocity();
	const auto distance = (origin - m_peek_state.m_source).length2d();

	if (!(settings->movement.peek_assist_retreat_on_key && !m_peek_state.m_going_back && m_is_any_move_pressed)) {
		constexpr auto autism = 5; // ticks from max velocity to 0 with sv_accelerate is default (5.5)
		const auto predicted_origin = origin + velocity * TICKS_TO_TIME(XOR32S(autism));

		// distance to m_peek_state.m_source less than distance to our position after 'magic autism number' ticks
		// so we need to stop now if we wish to almost perfectly forward to our start position
		if (distance < (origin - predicted_origin).length2d()) {
			if (fast_stop(cmd, original_angle, 15.f)) {
				m_peek_state.m_can_recharge = true;
				cmd->m_forwardmove = cmd->m_sidemove = cmd->m_upmove = 0.f;
				//engine_prediction->repredict(cmd);
			}

		} else if (distance > 5.f) {
			original_angle.y = math::calculate_angle(origin, m_peek_state.m_source).y;

			cmd->m_forwardmove = CVAR_FLOAT("cl_forwardspeed");
			cmd->m_sidemove = cmd->m_upmove = 0.0f;

			// Compensate lateral velocity
			vec3d direction{}, forward{};
			math::vector_angles(velocity, direction);
			direction.y = original_angle.y - direction.y;
			math::angle_vectors(direction, forward);
			cmd->m_sidemove = (forward * -velocity.length2d()).y;

			// We don't want to make jitter move while going back so make it only compensate the huge lateral velocity
			if (std::abs(cmd->m_sidemove) < 10.f)
				cmd->m_sidemove = 0.f;
		}
	}

	if (distance <= 5.f) {
		m_peek_state.m_can_recharge = true;
		if (m_peek_state.m_last_shot_time != -1.f) {
			m_peek_state.m_last_ticks_going_back = TIME_TO_TICKS(
					std::max<float>(std::abs(interfaces::global_vars->m_curtime - m_peek_state.m_last_shot_time) - TICKS_TO_TIME(5), 0.f));

			m_peek_state.m_last_shot_time = -1.f;
		}

		// TODO: rebuild game movement to see how much time we need to back to our last shot position
		if (hvh::exploits->m_in_shift || hvh::get_ticks_to_shoot() > 3 || (hotkeys->doubletap.m_active && !hvh::exploits->m_charged)) {
			cmd->m_forwardmove = cmd->m_sidemove = cmd->m_upmove = 0.f;
			return;
		}

		m_peek_state.m_going_back = false;
	}
}

void movement_t::autojump(user_cmd_t* cmd) {
	if (CVAR_BOOL("sv_autobunnyhopping"))
		return;

	if (!settings->movement.bhop)
		return;

	switch (globals->m_local->movetype()) {
		case type_ladder:
		case type_noclip:
		case type_observer:
			return;
	}

	if (!globals->m_local->flags().has(fl_onground))
		cmd->m_buttons.remove(in_jump);
}

void movement_t::autostrafe(user_cmd_t* cmd) {
	vec3d velocity = globals->m_local->velocity();
	float speed = velocity.length2d();

	if (cmd->m_buttons.has(in_speed) || !settings->movement.autostrafe || speed < 10.f || globals->m_groundtick > 1)
		return; // doesn't allow strafe when you're holding shift or you're not moving

	// TODO:
	//if (stop)
	//	return;

	bool holding_w = cmd->m_buttons.has(in_forward);
	bool holding_a = cmd->m_buttons.has(in_moveleft);
	bool holding_s = cmd->m_buttons.has(in_back);
	bool holding_d = cmd->m_buttons.has(in_moveright);

	bool m_pressing_move = holding_w || holding_a || holding_s || holding_d;

	static auto switch_key = 1.f;
	static auto circle_yaw = 0.f;
	static auto old_yaw = 0.f;
	velocity.z = 0.f;

	auto ideal_strafe = (speed > 5.f) ? RAD2DEG(std::asin(15.f / speed)) : 90.f;
	ideal_strafe *= 1.f - (settings->movement.strafe_smooth * 0.01f);

	ideal_strafe = min(90.f, ideal_strafe);

	switch_key *= -1.f;

	if (m_pressing_move) {
		float wish_dir{};

		// move in the appropriate direction.
		if (holding_w) {
			//  forward left
			if (holding_a) {
				wish_dir += 45.f;
			}
			//  forward right
			else if (holding_d) {
				wish_dir -= 45.f;
			}
		} else if (holding_s) {
			//  back left
			if (holding_a) {
				wish_dir += 135.f;
			}
			//  back right
			else if (holding_d) {
				wish_dir -= 135.f;
			}
			//  back
			else {
				wish_dir += 180.f;
			}

			cmd->m_forwardmove = 0;
		} else if (holding_a) {
			//  left
			wish_dir += 90.f;
		} else if (holding_d) {
			//  right
			wish_dir -= 90.f;
		}

		globals->m_original_angle.y += math::normalize_yaw(wish_dir);
	}

	float smooth = (1.f - (0.15f * (1.f - settings->movement.strafe_smooth * 0.01f)));

	if (speed <= 0.5f) {
		cmd->m_forwardmove = 450.f;
		return;
	}

	const auto diff = math::normalize_yaw(globals->m_original_angle.y - RAD2DEG(std::atan2f(velocity.y, velocity.x)));

	cmd->m_forwardmove = std::clamp((5850.f / speed), -450.f, 450.f);
	cmd->m_sidemove = (diff > 0.f) ? -450.f : 450.f;

	globals->m_original_angle.y = math::normalize_yaw(globals->m_original_angle.y - diff * smooth);
}

bool movement_t::is_pressing_movement_keys(sdk::user_cmd_t* cmd) {
	return cmd->m_forwardmove != 0.f || cmd->m_sidemove != 0.f || cmd->m_buttons.has(in_speed);
}

bool movement_t::fast_stop(user_cmd_t* cmd, vec3d original_angle, float min_speed, bool in_air) {
	if (globals->m_local == nullptr || !globals->m_local->is_alive())
		return false;

	switch (globals->m_local->movetype()) {
		case type_noclip:
		case type_observer:
			return false;
	}

	if (!in_air) {
		if (globals->m_groundtick <= 1)
			return false;
	}

	auto weapon_info = globals->m_weapon->get_cs_weapon_info();
	if (weapon_info == nullptr)
		return false;

	cmd->m_buttons.remove(in_speed);

	if (min_speed <= 0.f)
		min_speed = (globals->m_weapon->zoom_level() > 0 ? weapon_info->m_max_speed_alt : weapon_info->m_max_speed) / 3.1f;

	auto speed = globals->m_local->velocity().length2d();
	if (speed < min_speed) {
		//engine_prediction->repredict(cmd);
		return true;
	}

	vec3d direction{}, forward{};
	math::vector_angles(globals->m_local->velocity(), direction);
	direction.y = original_angle.y - direction.y;
	math::angle_vectors(direction, forward);
	vec3d negated_direction = forward * -speed;
	cmd->m_forwardmove = negated_direction.x;
	cmd->m_sidemove = negated_direction.y;
	cmd->m_upmove = 0.f;
	//engine_prediction->repredict(cmd);
	return false;
}

void movement_t::slow_walk(sdk::user_cmd_t* cmd, vec3d original_angle, float min_speed_mult, bool in_air) {
	const auto info = globals->m_weapon->get_cs_weapon_info();
	if (info == nullptr)
		return;

	switch (globals->m_local->movetype()) {
		case type_noclip:
		case type_observer:
			return;
	}

	const float min_speed = (globals->m_local->scoped() ? info->m_max_speed_alt : info->m_max_speed) / min_speed_mult;
	const vec2d move = { cmd->m_forwardmove, cmd->m_sidemove };
	const float move_length = move.length();

	if (move_length > 0.f && movement->m_is_any_move_pressed && movement->fast_stop(cmd, original_angle, min_speed + 10.f, in_air)) {
		const float ratio = min_speed / move_length;
		cmd->m_forwardmove *= ratio;
		cmd->m_sidemove *= ratio;
		//engine_prediction->repredict(cmd);
	}
}

STFI void micro_movement(user_cmd_t* cmd) {
	if (!settings->antiaim.enable || movement->m_peek_state.m_going_back)
		return;

	if (globals->m_groundtick < 1 || movement->m_is_any_move_pressed)
		return;

	float move_amount = cmd->m_buttons.has(in_duck) || globals->m_fake_duck ? 3.25f : 1.01f;

	if (cmd->m_viewangles.z != 0.f)
		move_amount *= 1.75f;

	move_amount = (cmd->m_command_number & 1) ? move_amount : -move_amount;
	vec3d velocity = globals->m_local->velocity();
	if (velocity.length2d() < 10.f)
		cmd->m_forwardmove += move_amount;
}

STFI vec3d get_peek_assist_source() {
	vec3d src = globals->m_local->origin();
	vec3d dst = src + vec3d{ 0.f, 0.f, -1000.f };

	ray_t ray{ src, dst, globals->m_local->mins(), globals->m_local->maxs() };
	trace_filter_t filter{};
	filter.m_skip = globals->m_local;

	trace_t trace{};
	interfaces::traces->trace_ray(ray, MASK_PLAYERSOLID, &filter, &trace);
	return trace.m_end;
}

void movement_t::on_pre_move(user_cmd_t* cmd, vec3d& original_angle) {
	switch (globals->m_local->movetype()) {
		case type_noclip:
		case type_ladder:
		case type_observer:
			return;
	}

	m_is_any_move_pressed = is_pressing_movement_keys(cmd);
	if (globals->m_local == nullptr || !globals->m_local->is_alive())
		return;

	if (globals->m_local->flags().has(fl_onground)) {
		globals->m_airtick = 0;
		++globals->m_groundtick;
	} else {
		globals->m_groundtick = 0;
		++globals->m_airtick;
	}

	autojump(cmd);
	micro_movement(cmd);

	m_peek_state.m_can_recharge = false;
	m_peek_state.m_working = false;

	if (interfaces::game_rules != nullptr) {
		if (interfaces::game_rules->is_freeze_time() || globals->m_local->flags().has(fl_frozen))
			return;
	}

	if (hotkeys->slow_walk.m_active)
		movement->slow_walk(cmd, globals->m_original_angle, 3.f, true);

	if (hotkeys->peek_assist.m_active) {
		if (m_peek_state.m_going_back || (settings->movement.peek_assist_retreat_on_key && !m_is_any_move_pressed)) {
			peek_assist(cmd, original_angle);
			m_peek_state.m_working = true;
		}
	} else
		m_peek_state.reset();
}

void movement_t::on_create_move(sdk::user_cmd_t* cmd) {
	switch (globals->m_local->movetype()) {
		case type_noclip:
		case type_observer:
			return;
	}

	if (globals->m_local == nullptr || !globals->m_local->is_alive())
		return;

	autostrafe(cmd);
}

void movement_t::on_post_move(sdk::user_cmd_t* cmd, vec3d& original_angle) {
	if (globals->m_local == nullptr || !globals->m_local->is_alive())
		return;

	if (hotkeys->peek_assist.m_active) {
		if (m_peek_state.m_source == vec3d{ 0.f, 0.f, 0.f })
			m_peek_state.m_source = get_peek_assist_source();

		if (hvh::is_shooting(cmd)) {
			if (globals->m_weapon->item_definition_index() == e_weapon_type::weapon_revolver) {
				if (cmd->m_buttons.has(in_attack) && hvh::aimbot->m_revolver_fire) {
					m_peek_state.m_going_back = true;
					m_peek_state.m_last_shot_time = interfaces::global_vars->m_curtime;
					//return;
				}
			} else {
				m_peek_state.m_going_back = true;
				m_peek_state.m_last_shot_time = interfaces::global_vars->m_curtime;
				//return;
			}
		}
	}

	if (!m_peek_state.m_working && settings->movement.fast_stop && globals->m_groundtick > 1) {
		if (!m_is_any_move_pressed)
			fast_stop(cmd, original_angle, 15.f);
	}
}

void movement_t::fix(user_cmd_t* cmd, vec3d& original_angle) {
	switch (globals->m_local->movetype()) {
		case type_noclip:
		case type_observer:
			return;
	}

	vec3d move = { cmd->m_forwardmove, cmd->m_sidemove, 0.f };

	auto len = move.normalize_movement();

	if (len == 0.f)
		return;

	vec3d move_angle{};
	math::vector_angles(move, move_angle);

	auto delta = cmd->m_viewangles.y - original_angle.y;
	move_angle.y += delta;

	vec3d dir{};
	math::angle_vectors(move_angle, dir);

	dir *= len;

	if (globals->m_local->movetype() == e_move_type::type_ladder) {
		if (cmd->m_viewangles.x >= 45.f && original_angle.x < 45.f && std::abs(delta) <= 65.f)
			dir.x = -dir.x;

		cmd->m_forwardmove = dir.x;
		cmd->m_sidemove = dir.y;

		if (cmd->m_forwardmove > 200.f)
			cmd->m_buttons.add(in_forward);
		else if (cmd->m_forwardmove < -200.f)
			cmd->m_buttons.add(in_back);

		if (cmd->m_sidemove > 200.f)
			cmd->m_buttons.add(in_moveright);
		else if (cmd->m_sidemove < -200.f)
			cmd->m_buttons.add(in_moveleft);
	} else {
		if (cmd->m_viewangles.x < -90 || cmd->m_viewangles.x > 90)
			dir.x = -dir.x;

		cmd->m_forwardmove = dir.x;
		cmd->m_sidemove = dir.y;
	}

	cmd->m_forwardmove = std::clamp(cmd->m_forwardmove, -450.f, 450.f);
	cmd->m_sidemove = std::clamp(cmd->m_sidemove, -450.f, 450.f);
	cmd->m_upmove = std::clamp(cmd->m_upmove, -320.f, 320.f);

	leg_movement(cmd);
	//engine_prediction->repredict(cmd);
}

void movement_t::leg_movement(user_cmd_t* cmd) {
	switch (globals->m_local->movetype()) {
		case type_ladder:
		case type_noclip:
		case type_observer:
			return;
	}

	if (!globals->m_local->flags().has(fl_onground))
		return;

	cmd->m_buttons.remove(in_any_move);

	if (settings->movement.leg_movement == 1) {
		vec2d move = { cmd->m_forwardmove, cmd->m_sidemove };

		if (!*globals->m_send_packet) {
			if (move.y < -5.f)
				cmd->m_buttons.add(in_moveright);
			else if (move.y > 5.f)
				cmd->m_buttons.add(in_moveleft);

			if (move.x < -5.f)
				cmd->m_buttons.add(in_forward);
			else if (move.x > 5.f)
				cmd->m_buttons.add(in_back);
		} else {
			if (move.y > 5.f)
				cmd->m_buttons.add(in_moveright);
			else if (move.y < -5.f)
				cmd->m_buttons.add(in_moveleft);

			if (move.x > 5.f)
				cmd->m_buttons.add(in_forward);
			else if (move.x < -5.f)
				cmd->m_buttons.add(in_back);
		}

		/*	cmd->m_buttons.add(globals->m_local->button_forced());
		cmd->m_buttons &= ~(globals->m_local->button_disabled());

		const auto buttons = cmd->m_buttons;
		const auto local_buttons = globals->m_local->buttons();
		const auto buttons_changed = buttons ^ local_buttons;

		globals->m_local->button_last() = local_buttons;
		globals->m_local->buttons() = buttons;
		globals->m_local->button_pressed() = buttons_changed & buttons;
		globals->m_local->button_released() = buttons_changed & (~buttons);*/

		//engine_prediction->repredict(cmd);
	}
}