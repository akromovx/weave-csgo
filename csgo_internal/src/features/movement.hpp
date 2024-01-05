#pragma once
#include "../base_includes.hpp"
#include "../utils/vector.hpp"
#include "../../sdk/input.hpp"

struct movement_t {
	struct {
		vec3d m_source{};
		bool m_going_back{};
		float m_animation{};

		int m_last_ticks_going_back{};
		float m_last_shot_time{};
		bool m_can_recharge{};

		bool m_working{};
		bool m_should_stop{};

		__forceinline void reset() {
			m_source = vec3d{};
			m_going_back = false;
			m_last_ticks_going_back = -1;
			m_last_shot_time = -1.f;
			m_can_recharge = true;
			m_should_stop = false;
		}
	} m_peek_state;

	bool m_is_any_move_pressed{};

	bool is_pressing_movement_keys(sdk::user_cmd_t* cmd);
	void slow_walk(sdk::user_cmd_t* cmd, vec3d original_angle, float min_speed_mult = 3.f, bool in_air = false);
	bool fast_stop(sdk::user_cmd_t* cmd, vec3d original_angle, float min_speed = -1.f, bool in_air = false);
	void on_pre_move(sdk::user_cmd_t* cmd, vec3d& original_angle);
	void on_create_move(sdk::user_cmd_t* cmd);
	void on_post_move(sdk::user_cmd_t* cmd, vec3d& original_angle);
	void fix(sdk::user_cmd_t* cmd, vec3d& original_angle);

private:
	void autojump(sdk::user_cmd_t* cmd);
	void autostrafe(sdk::user_cmd_t* cmd);
	void leg_movement(sdk::user_cmd_t* cmd);
	void peek_assist(sdk::user_cmd_t* cmd, vec3d& original_angle);
};

GLOBAL_DYNPTR(movement_t, movement);