#pragma once
#include "net_channel.hpp"
#include "../src/utils/vector.hpp"

namespace sdk {
	struct client_state_t {
		char pad000[0x9C];
		net_channel_t* m_net_channel;
		int m_challenge_nr;
		char pad001[0x4];
		double m_connect_time;
		int m_retry_number;
		char pad002[0x54];
		int m_signon_state;
		char pad003[0x4];
		double m_next_cmd_time;
		int m_server_count;
		int m_current_sequence;
		char pad004[0x8];

		struct {
			float m_clock_offsets[0x10];
			uint32_t m_cur_clock_offset;
			uint32_t m_server_tick;
			uint32_t m_client_tick;
		} m_clock_drift_manager;

		int m_delta_tick;
		char pad005[0x4];
		int m_view_entity;
		int m_player_slot;
		bool m_paused;
		char pad006[0x3];
		char m_level_name[0x104];
		char m_level_name_short[0x28];
		char pad007[0xD4];
		int m_max_clients;
		char pad008[0x4994];
		int m_old_tickcount;
		float m_tick_remainder;
		float m_frametime;
		int m_last_outgoing_command;
		int m_choked_commands;
		int m_last_command_ack;
		int m_last_server_tick;
		int m_command_ack;
		int m_sound_sequence;
		int m_last_progress_percent;
		bool m_is_hltv;
		char pad009[0x4B];
		vec3d m_viewangles;
		char pad010[0xCC];
		void* m_events;

		__forceinline int get_current_tick() const {
			return m_last_outgoing_command + m_choked_commands + 1;
		}
	};
} // namespace sdk