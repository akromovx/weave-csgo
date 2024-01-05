#include "dormant.hpp"
#include "../globals.hpp"
#include "../interfaces.hpp"

using namespace sdk;

namespace esp {
	void dormant_t::sound_player_t::set(bool store_data, const vec3d& origin, int flags) {
		if (store_data) {
			m_recieve_time = interfaces::global_vars->m_realtime;
			m_origin = origin;
			m_flags = flags;
		} else {
			m_recieve_time = 0.0f;
			m_origin.set();
			m_flags = 0;
		}
	}

	void dormant_t::sound_player_t::rewrite(sound_info_t& sound) {
		m_recieve_time = interfaces::global_vars->m_realtime;
		m_origin = *sound.m_origin;
	}

	void dormant_t::on_round_start() {
	}

	void dormant_t::start() {
		m_cur_sound_list.remove_all();
		interfaces::engine_sound->get_active_sounds(m_cur_sound_list);

		if (!m_cur_sound_list.count())
			return;

		if (globals->m_local == nullptr)
			return;

		for (auto i = 0; i < m_cur_sound_list.count(); ++i) {
			auto& sound = m_cur_sound_list[i];

			if (sound.m_sound_source < 1 || sound.m_sound_source > 64)
				continue;

			if (sound.m_origin->is_zero())
				continue;

			if (!is_valid_sound(sound))
				continue;

			auto player = (cs_player_t*)interfaces::entity_list->get_client_entity(sound.m_sound_source);
			if (player == nullptr || !player->is_alive() || !player->is_player() || player == globals->m_local || player->is_teammate()) {
				m_sound_players[sound.m_sound_source].set();
				continue;
			}

			start_adjust(player, sound);
			m_sound_players[sound.m_sound_source].rewrite(sound);
		}

		m_sound_buffer = m_cur_sound_list;
	}

	void dormant_t::start_adjust(cs_player_t* player, sound_info_t& sound) {
		vec3d src3D, dst3D;
		trace_t tr;
		ray_t ray;
		trace_filter_t filter;

		src3D = *sound.m_origin + vec3d{ 0.f, 0.f, 1.f };
		dst3D = src3D - vec3d{ 0.f, 0.f, 100.f };

		filter.m_skip = player;
		ray.init(src3D, dst3D);

		interfaces::traces->trace_ray(ray, MASK_PLAYERSOLID, &filter, &tr);

		if (tr.m_all_solid)
			m_sound_players[sound.m_sound_source].m_recieve_time = -1;

		*sound.m_origin = tr.m_fraction <= 0.97f ? tr.m_end : *sound.m_origin;

		m_sound_players[sound.m_sound_source].m_flags = player->flags();
		m_sound_players[sound.m_sound_source].m_flags |= (tr.m_fraction < 0.50f ? fl_ducking : 0) | (tr.m_fraction < 1.0f ? fl_onground : 0);
		m_sound_players[sound.m_sound_source].m_flags &= (tr.m_fraction >= 0.50f ? ~fl_ducking : 0) | (tr.m_fraction >= 1.0f ? ~fl_onground : 0);
	}

	bool dormant_t::is_sound_expired(cs_player_t* entity, float& alpha) {
		auto i = entity->index();
		auto& sound_player = m_sound_players[i];

		auto expired = false;

		if (std::abs(interfaces::global_vars->m_realtime - sound_player.m_recieve_time) > 10.0f)
			expired = true;

		if (std::abs(interfaces::global_vars->m_realtime - sound_player.m_recieve_time) < 0.5f)
			alpha = 0.8f;

		entity->spotted() = true;
		entity->flags() = sound_player.m_flags;
		entity->set_abs_origin(sound_player.m_origin);
		entity->render_origin() = sound_player.m_origin;

		m_sound_players[i].m_last_seen_time = sound_player.m_recieve_time;
		return expired;
	}

	bool dormant_t::is_valid_sound(sound_info_t& sound) {
		for (auto i = 0; i < m_sound_buffer.count(); i++)
			if (m_sound_buffer[i].m_guid == sound.m_guid)
				return false;

		return true;
	}
} // namespace esp