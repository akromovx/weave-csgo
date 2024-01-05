#pragma once
#include "../src/utils/utils.hpp"
#include "../src/utils/vector.hpp"
#include "entities.hpp"

namespace sdk {
	inline constexpr int multiplayer_backup = 150;

	struct user_cmd_t;
	struct prediction_t {
		VFUNC(update(int start_frame, bool valid_frame, int inc_ack, int out_cmd),
			  void(__thiscall*)(decltype(this), int, bool, int, int), 3, start_frame, valid_frame, inc_ack, out_cmd);

		VFUNC(set_local_viewangles(const vec3d& angle), void(__thiscall*)(decltype(this), const vec3d&), 13, angle);
		VFUNC(is_active(), bool(__thiscall*)(decltype(this)), 14);

		VFUNC(check_moving_ground(cs_player_t* player, double frametime), void(__thiscall*)(decltype(this), cs_player_t*, double), 18, player, frametime);

		VFUNC(setup_move(base_entity_t* player, user_cmd_t* cmd, void* helper, void* move),
			  void(__thiscall*)(decltype(this), base_entity_t*, user_cmd_t*, void*, void*), 20, player, cmd, helper, move);

		VFUNC(finish_move(base_entity_t* player, user_cmd_t* cmd, void* move_data),
			  void(__thiscall*)(decltype(this), base_entity_t*, user_cmd_t*, void*), 21, player, cmd, move_data);

		char pad_0[4];
		int m_last_ground;				// 4
		bool m_in_prediction;			// 8
		bool m_is_first_time_predicted; // 9
		bool m_engine_paused;			// 10
		bool m_old_cl_predict_value;	// 11
		int m_previous_startframe;		// 12
		int m_incoming_packet_number;	// 16
		char pad_1[0x4];
		int m_commands_predicted;
		int m_server_commands_acknowledged;
		bool m_prev_ack_had_erroes;
	};
} // namespace sdk