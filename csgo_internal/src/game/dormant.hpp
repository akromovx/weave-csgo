#pragma once
#include "players.hpp"
#include "../../sdk/engine_sound.hpp"

namespace esp {
	// improper
	struct dormant_t {
		void on_round_start();
		void start();

		bool is_sound_expired(sdk::cs_player_t* player, float&);
		void start_adjust(sdk::cs_player_t* player, sdk::sound_info_t& sound);
		bool is_valid_sound(sdk::sound_info_t& sound);

		struct sound_player_t {
			void set(bool store_data = false, const vec3d& origin = vec3d{}, int flags = 0);
			void rewrite(sdk::sound_info_t& sound);

			float m_recieve_time{};
			vec3d m_origin{};
			vec3d m_observer_origin{};
			int m_flags{};

			float m_last_seen_time{};
		} m_sound_players[64];

		utils::utl_vector_t<sdk::sound_info_t> m_sound_buffer;
		utils::utl_vector_t<sdk::sound_info_t> m_cur_sound_list;
	};

	GLOBAL_DYNPTR(dormant_t, dormant);
}