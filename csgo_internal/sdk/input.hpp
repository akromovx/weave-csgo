#pragma once
#include <cstddef>
#include "checksum_crc.h"
#include "../src/utils/vector.hpp"

namespace sdk {
	enum e_buttons {
		in_attack = (1 << 0),
		in_jump = (1 << 1),
		in_duck = (1 << 2),
		in_forward = (1 << 3),
		in_back = (1 << 4),
		in_use = (1 << 5),
		in_cancel = (1 << 6),
		in_left = (1 << 7),
		in_right = (1 << 8),
		in_moveleft = (1 << 9),
		in_moveright = (1 << 10),
		in_attack2 = (1 << 11),
		in_run = (1 << 12),
		in_reload = (1 << 13),
		in_alt1 = (1 << 14),
		in_alt2 = (1 << 15),
		in_score = (1 << 16),
		in_speed = (1 << 17),
		in_walk = (1 << 18),
		in_zoom = (1 << 19),
		in_weapon1 = (1 << 20),
		in_weapon2 = (1 << 21),
		in_bullrush = (1 << 22),
		in_grenade1 = (1 << 23),
		in_grenade2 = (1 << 24),
		in_attack3 = (1 << 25),
		in_any_move = in_forward | in_back | in_moveright | in_moveleft
	};

	constexpr int max_input = 150;

	struct bf_read;
	struct bf_write;

	struct user_cmd_t {
		__forceinline user_cmd_t() { std::memset(this, 0, sizeof(*this)); };
		virtual ~user_cmd_t(){};

		__forceinline void copy(user_cmd_t* cmd) { std::memcpy(this, cmd, sizeof(user_cmd_t)); }

		int m_command_number;
		int m_tickcount;
		vec3d m_viewangles;
		vec3d m_aim_direction;
		float m_forwardmove;
		float m_sidemove;
		float m_upmove;
		flags_t m_buttons;
		int m_impulse;
		int m_weapon_select;
		int m_weapon_sub_type;
		int m_random_seed;
		short m_mouse_dx;
		short m_mouse_dy;
		bool m_has_been_predicted;
		vec3d m_headangles;
		vec3d m_headoffset;

		__forceinline CRC32_t get_checksum() const {
			CRC32_t crc;
			CRC32_Init(&crc);

			CRC32_ProcessBuffer(&crc, &m_command_number, sizeof(m_command_number));
			CRC32_ProcessBuffer(&crc, &m_tickcount, sizeof(m_tickcount));
			CRC32_ProcessBuffer(&crc, &m_viewangles, sizeof(m_viewangles));
			CRC32_ProcessBuffer(&crc, &m_aim_direction, sizeof(m_aim_direction));
			CRC32_ProcessBuffer(&crc, &m_forwardmove, sizeof(m_forwardmove));
			CRC32_ProcessBuffer(&crc, &m_sidemove, sizeof(m_sidemove));
			CRC32_ProcessBuffer(&crc, &m_upmove, sizeof(m_upmove));
			CRC32_ProcessBuffer(&crc, &m_buttons, sizeof(m_buttons));
			CRC32_ProcessBuffer(&crc, &m_impulse, sizeof(m_impulse));
			CRC32_ProcessBuffer(&crc, &m_weapon_select, sizeof(m_weapon_select));
			CRC32_ProcessBuffer(&crc, &m_weapon_sub_type, sizeof(m_weapon_sub_type));
			CRC32_ProcessBuffer(&crc, &m_random_seed, sizeof(m_random_seed));
			CRC32_ProcessBuffer(&crc, &m_mouse_dx, sizeof(m_mouse_dx));
			CRC32_ProcessBuffer(&crc, &m_mouse_dy, sizeof(m_mouse_dy));
			CRC32_Final(&crc);
			return crc;
		}
	};

	struct cmd_ctx_t {
		bool m_needs_processing;
		user_cmd_t m_cmd;
		int m_command_number;
	};

	struct verified_usercmd_t {
		user_cmd_t m_cmd;
		uint32_t m_crc;
	};

	struct input_t {
		uint8_t pad0[0xC];
		bool m_track_ir_available;
		bool m_mouse_initialized;
		bool m_mouse_active;
		uint8_t pad1[0x9A];
		bool m_camera_in_third_person;
		uint8_t pad2[0x2];
		vec3d m_camera_offset;
		uint8_t pad3[0x38];
		user_cmd_t* m_commands;
		verified_usercmd_t* m_verified_commands;

		__forceinline user_cmd_t* get_user_cmd(int sequence) { return &m_commands[sequence % max_input]; }
		__forceinline verified_usercmd_t* get_verified_cmd(int sequence) { return &m_verified_commands[sequence % max_input]; }
	};
} // namespace sdk