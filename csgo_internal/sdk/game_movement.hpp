#pragma once
#include "../src/utils/utils.hpp"
#include "../src/utils/vector.hpp"
#include "entities.hpp"

namespace sdk {
	struct move_data_t {
		bool m_first_run_of_functions : 1;
		bool m_game_code_moved_player : 1;
		int m_player_handle;	 // edict index on server, client entity handle on client=
		int m_impulse_command;	 // Impulse command issued.
		vec3d m_view_angles;	 // Command view angles (local space)
		vec3d m_abs_view_angles; // Command view angles (world space)
		int m_buttons;			 // Attack buttons.
		int m_old_buttons;		 // From host_client->oldbuttons;
		float m_forwardmove;
		float m_sidemove;
		float m_upmove;
		float m_max_speed;
		float m_client_max_speed;
		vec3d m_velocity; // edict::velocity        // Current movement direction.
		vec3d m_angles;	  // edict::angles
		vec3d m_old_angles;
		float m_out_step_height; // how much you climbed this move
		vec3d m_out_wish_vel;	 // This is where you tried
		vec3d m_out_jump_vel;	 // This is your jump velocity
		vec3d m_constraint_center;
		float m_constraint_radius;
		float m_constraint_width;
		float m_constraint_speed_factor;
		float unknown[5];
		vec3d m_abs_origin;
	};

	struct game_movement_t {
		virtual ~game_movement_t() {}
		virtual void process_movement(base_entity_t* player, move_data_t* move) = 0;
		virtual void reset(void) = 0;
		virtual void start_track_prediction_errors(base_entity_t* player) = 0;
		virtual void finish_track_prediction_errors(base_entity_t* player) = 0;
		virtual void diff_print(char const* fmt, ...) = 0;
		virtual vec3d const& get_mins(bool ducked) const = 0;
		virtual vec3d const& get_maxs(bool ducked) const = 0;
		virtual vec3d const& get_player_view_offset(bool ducked) const = 0;
		virtual bool is_moving_player_stuck(void) const = 0;
		virtual base_entity_t* get_moving_player(void) const = 0;
		virtual void unblock_pusher(base_entity_t* player, base_entity_t* pusher) = 0;
		virtual void setup_movement_bounds(move_data_t* move) = 0;
	};
} // namespace sdk