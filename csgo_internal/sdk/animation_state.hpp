#pragma once
#include "animation_layer.hpp"
#include "../src/utils/vector.hpp"
#include "entities.hpp"

namespace sdk {
	enum animstate_pose_param_idx_t {
		PLAYER_POSE_PARAM_FIRST = 0,
		PLAYER_POSE_PARAM_LEAN_YAW = PLAYER_POSE_PARAM_FIRST,
		PLAYER_POSE_PARAM_SPEED,
		PLAYER_POSE_PARAM_LADDER_SPEED,
		PLAYER_POSE_PARAM_LADDER_YAW,
		PLAYER_POSE_PARAM_MOVE_YAW,
		PLAYER_POSE_PARAM_RUN,
		PLAYER_POSE_PARAM_BODY_YAW,
		PLAYER_POSE_PARAM_BODY_PITCH,
		PLAYER_POSE_PARAM_DEATH_YAW,
		PLAYER_POSE_PARAM_STAND,
		PLAYER_POSE_PARAM_JUMP_FALL,
		PLAYER_POSE_PARAM_AIM_BLEND_STAND_IDLE,
		PLAYER_POSE_PARAM_AIM_BLEND_CROUCH_IDLE,
		PLAYER_POSE_PARAM_STRAFE_DIR,
		PLAYER_POSE_PARAM_AIM_BLEND_STAND_WALK,
		PLAYER_POSE_PARAM_AIM_BLEND_STAND_RUN,
		PLAYER_POSE_PARAM_AIM_BLEND_CROUCH_WALK,
		PLAYER_POSE_PARAM_MOVE_BLEND_WALK,
		PLAYER_POSE_PARAM_MOVE_BLEND_RUN,
		PLAYER_POSE_PARAM_MOVE_BLEND_CROUCH_WALK,
		PLAYER_POSE_PARAM_COUNT,
	};

	struct pose_param_cache_t {
		bool m_initialized;
		int m_index;
		const char* m_name;
	};

	struct base_entity_t;
	struct base_weapon_t;

	struct animation_state_t {
		animation_state_t(){};
		animation_state_t(const animation_state_t& animstate) { memcpy(this, &animstate, sizeof(animation_state_t)); };

		void release() { delete this; }

		int* m_layer_order_preset = nullptr;
		bool m_first_run_since_init = false;

		bool m_first_foot_plant_since_init = false;
		int m_last_update_tick = 0;
		float m_eye_position_smooth_lerp = 0.0f;

		float m_strafe_change_weight_smooth_fall_off = 0.0f;

		float m_stand_walk_duration_state_has_been_valid = 0.0f;
		float m_stand_walk_duration_state_has_been_invalid = 0.0f;
		float m_stand_walk_how_long_to_wait_until_transition_can_blend_in = 0.0f;
		float m_stand_walk_how_long_to_wait_until_transition_can_blend_out = 0.0f;
		float m_stand_walk_blend_value = 0.0f;

		float m_stand_run_duration_state_has_been_valid = 0.0f;
		float m_stand_run_duration_state_has_been_invalid = 0.0f;
		float m_stand_run_how_long_to_wait_until_transition_can_blend_in = 0.0f;
		float m_stand_run_how_long_to_wait_until_transition_can_blend_out = 0.0f;
		float m_stand_run_blend_value = 0.0f;

		float m_crouch_walk_duration_state_has_been_valid = 0.0f;
		float m_crouch_walk_duration_state_has_been_invalid = 0.0f;
		float m_crouch_walk_how_long_to_wait_until_transition_can_blend_in = 0.0f;
		float m_crouch_walk_how_long_to_wait_until_transition_can_blend_out = 0.0f;
		float m_crouch_walk_blend_value = 0.0f;

		int m_cached_model_index = 0;

		float m_step_height_left = 0.0f;
		float m_step_height_right = 0.0f;

		void* m_weapon_last_bone_setup = nullptr;

		base_entity_t* m_player = nullptr;
		base_weapon_t* m_weapon = nullptr;
		base_weapon_t* m_weapon_last = nullptr;

		float m_last_update_time = 0.0f;
		int m_last_update_frame = 0;
		float m_last_update_increment = 0.0f;

		float m_eye_yaw = 0.0f;
		float m_eye_pitch = 0.0f;
		float m_goal_feet_yaw = 0.0f;
		float m_last_goal_feet_yaw = 0.0f;
		float m_move_yaw = 0.0f;
		float m_move_yaw_ideal = 0.0f;
		float m_move_yaw_current_to_ideal = 0.0f;
		float m_time_to_align_lower_body;

		float m_primary_cycle = 0.0f;
		float m_move_weight = 0.0f;

		float m_move_weight_smoothed = 0.0f;
		float m_anim_duck_amount = 0.0f;
		float m_duck_additional = 0.0f;
		float m_recrouch_weight = 0.0f;

		vec3d m_position_current = vec3d();
		vec3d m_position_last = vec3d();

		vec3d m_velocity = vec3d();
		vec3d m_velocity_normalized = vec3d();
		vec3d m_velocity_normalized_non_zero = vec3d();
		float m_velocity_length_xy = 0.0f;
		float m_velocity_length_z = 0.0f;

		float m_speed_as_portion_of_run_top_speed = 0.0f;
		float m_speed_as_portion_of_walk_top_speed = 0.0f;
		float m_speed_as_portion_of_crouch_top_speed = 0.0f;

		float m_duration_moving = 0.0f;
		float m_duration_still = 0.0f;

		bool m_on_ground = false;

		bool m_landing = false;
		float m_jump_to_fall = 0.0f;
		float m_duration_in_air = 0.0f;
		float m_left_ground_height = 0.0f;
		float m_land_anim_multiplier = 0.0f;

		float m_walk_run_transition = 0.0f;

		bool m_landed_on_ground_this_frame = false;
		bool m_left_the_ground_this_frame = false;
		float m_in_air_smooth_value = 0.0f;

		bool m_on_ladder = false;
		float m_ladder_weight = 0.0f;
		float m_ladder_speed = 0.0f;

		bool m_walk_to_run_transition_state = false;

		bool m_defuse_started = false;
		bool m_plant_anim_started = false;
		bool m_twitch_anim_started = false;
		bool m_adjust_started = false;

		char m_activity_modifiers_server[20] = {};

		float m_next_twitch_time = 0.0f;

		float m_time_of_last_known_injury = 0.0f;

		float m_last_velocity_test_time = 0.0f;
		vec3d m_velocity_last = vec3d();
		vec3d m_target_acceleration = vec3d();
		vec3d m_acceleration = vec3d();
		float m_acceleration_weight = 0.0f;

		float m_aim_matrix_transition = 0.0f;
		float m_aim_matrix_transition_delay = 0.0f;

		bool m_flashed = false;

		float m_strafe_change_weight = 0.0f;
		float m_strafe_change_target_weight = 0.0f;
		float m_strafe_change_cycle = 0.0f;
		int m_strafe_sequence = 0;
		bool m_strafe_changing = false;
		float m_duration_strafing = 0.0f;

		float m_foot_lerp = 0.0f;

		bool m_feet_crossed = false;

		bool m_player_is_accelerating = false;

		pose_param_cache_t m_pose_param_mappings[PLAYER_POSE_PARAM_COUNT] = {};

		float m_duration_move_weight_is_too_high = 0.0f;
		float m_static_approach_speed = 0.0f;

		int m_previous_move_state = 0;
		float m_stutter_step = 0.0f;

		float m_action_weight_bias_remainder = 0.0f;

		vec3d m_foot_left_pos_anim = vec3d();
		vec3d m_foot_left_pos_anim_last = vec3d();
		vec3d m_foot_left_pos_plant = vec3d();
		vec3d m_foot_left_plant_vel = vec3d();
		float m_foot_left_lock_amount = 0.0f;
		float m_foot_left_last_plant_time = 0.0f;

		vec3d m_foot_right_pos_anim = vec3d();
		vec3d m_foot_right_pos_anim_last = vec3d();
		vec3d m_foot_right_pos_plant = vec3d();
		vec3d m_foot_right_plant_vel = vec3d();
		float m_foot_right_lock_amount = 0.0f;
		float m_foot_right_last_plant_time = 0.0f;

		float m_camera_smooth_height = 0.0f;
		bool m_smooth_height_valid = false;
		float m_last_time_velocity_over_ten = 0.0f;

		float m_anko1337;

		float m_aim_yaw_min = 0.0f;
		float m_aim_yaw_max = 0.0f;
		float m_aim_pitch_min = 0.0f;
		float m_aim_pitch_max = 0.0f;

		int m_animstate_model_version = 0;

		PATTERN(reset(), void(__thiscall*)(decltype(this)), patterns::animstate::reset);
		PATTERN(create(base_entity_t* entity), void(__thiscall*)(decltype(this), base_entity_t*), patterns::animstate::create, entity);
		PATTERN(update(float y, float x),
				void(__vectorcall*)(decltype(this), void*, float, float, float, void*), patterns::animstate::update, nullptr, 0.f, y, x, nullptr);

		PATTERN(setup_velocity(), void(__thiscall*)(decltype(this)), patterns::animstate::setup_velocity);
		PATTERN(setup_aim_matrix(), void(__thiscall*)(decltype(this)), patterns::animstate::setup_aim_matrix);
		PATTERN(setup_movement(), void(__thiscall*)(decltype(this)), patterns::animstate::setup_movement);

		void increment_layer_cycle(animation_layer_t* layer, bool loop);
		bool is_layer_sequence_finished(animation_layer_t* layer, float time);
		void set_layer_cycle(animation_layer_t* layer, float_t cycle);
		void set_layer_rate(animation_layer_t* layer, float rate);

		void set_layer_weight(animation_layer_t* layer, float weight);
		void set_layer_weight_rate(animation_layer_t* layer, float prev);
		void set_layer_sequence(animation_layer_t* layer, int activity);
		int select_sequence_from_activity_modifier(int iActivity);
	};
} // namespace sdk