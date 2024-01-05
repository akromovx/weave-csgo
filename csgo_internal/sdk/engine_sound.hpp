#pragma once
#include "../src/utils/displacement.hpp"
#include "../src/utils/vector.hpp"

namespace sdk {
	struct sound_info_t {
		int m_guid;
		void* m_filename_handle;
		int m_sound_source;
		int m_channel;
		int m_speaker_entity;
		float m_volume;
		float m_last_spatialized_volume;
		float m_radius;
		int m_pitch;
		vec3d* m_origin;
		vec3d* m_direction;
		bool m_update_positions;
		bool m_is_sentence;
		bool m_dry_mix;
		bool m_speaker;
		bool m_from_server;
	};

	struct engine_sound_t {
		VFUNC(get_active_sounds(utils::utl_vector_t<sound_info_t>& sound_list),
			  void(__thiscall*)(void*, utils::utl_vector_t<sound_info_t>&), 19, sound_list);
	};
} // namespace sdk