#pragma once
#include "trace_define.hpp"

namespace sdk {
	struct beam_t;

	enum beam_types_t {
		beam_normal = 0,
		beam_disk = 2,
		beam_cylinder,
		beam_follow,
		beam_ring,
		beam_spline,
		beam_ring_point,
		beam_laser,
		beam_tesla,
	};

	enum beam_flags_t {
		beam_start_entity = 0x00000001,
		beam_end_entity = 0x00000002,
		beam_fade_in = 0x00000004,
		beam_fade_out = 0x00000008,
		beam_sine_no_ise = 0x00000010,
		beam_solid = 0x00000020,
		beam_shade_in = 0x00000040,
		beam_shade_out = 0x00000080,
		beam_only_no_is_once = 0x00000100,
		beam_notile = 0x00000200,
		beam_use_hitboxes = 0x00000400,
		beam_start_visible = 0x00000800,
		beam_end_visible = 0x00001000,
		beam_is_active = 0x00002000,
		beam_forever = 0x00004000,
		beam_halobeam = 0x00008000,
		beam_reversed = 0x00010000,
		num_beam_flags = 17
	};

	struct beam_info_t {
		beam_types_t m_type{};

		cs_player_t* m_start_ent{};
		int m_start_attachment{};

		cs_player_t* m_end_ent{};
		int m_end_attachment{};

		vec3d m_start{};
		vec3d m_end{};

		int m_model_index{};
		const char* m_model_name{};

		int m_halo_index{};
		const char* m_halo_name{};
		float m_halo_scale{};

		float m_life{};
		float m_width{};
		float m_end_width{};
		float m_fade_lenght{};
		float m_amplitude{};

		float m_brightness{};
		float m_speed{};

		int m_start_frame{};
		float m_frame_rate{};

		float m_red{};
		float m_green{};
		float m_blue{};

		bool m_renderable{};

		int m_segments{};

		int m_flags{};

		vec3d m_center{};
		float m_start_radius{};
		float m_end_radius{};

		__forceinline beam_info_t() {
			m_type = beam_normal;
			m_segments = -1;
			m_model_name = NULL;
			m_halo_name = NULL;
			m_model_index = -1;
			m_halo_index = -1;
			m_renderable = true;
			m_flags = 0;
		}
	};

	struct view_render_beams_t {
		//VFUNC(draw_beam(beam_t* beam), void(__thiscall*)(decltype(this), beam_t*), 4, beam);
		//VFUNC(create_beam_points(beam_info_t& info), beam_t* (__thiscall*)(decltype(this), beam_info_t&), 9, info);

		view_render_beams_t(void);
		virtual ~view_render_beams_t(void);

		virtual void init_beams(void);
		virtual void shut_down_beams(void);
		virtual void clear_beams(void);

		virtual void update_temp_ent_beams();

		virtual void draw_beam(beam_t* beam);
		virtual void draw_beam(void* beam, trace_filter_t* entity_beam_trace_filter = NULL);

		virtual void killed_dead_beams(base_entity_t* dead_entity);

		virtual void create_beam_ents(int start_ent, int end_ent, int model_index, int halo_index, float halo_scale,
									  float life, float width, float end_width, float fade_length, float amplitude,
									  float brightness, float speed, int start_frame,
									  float framerate, float r, float g, float b, int type = -1);
		virtual beam_t* create_beam_ents(beam_info_t& beam_info);

		virtual void create_beam_ent_point(int start_entity, const vec3d* start, int end_entity, const vec3d* end,
										   int model_index, int halo_index, float halo_scale,
										   float life, float width, float end_width, float fade_length, float amplitude,
										   float brightness, float speed, int start_frame,
										   float framerate, float r, float g, float b);
		virtual beam_t* create_beam_ent_point(beam_info_t& beam_info);

		virtual void create_beam_points(vec3d& start, vec3d& end, int model_index, int halo_index, float halo_scale,
										float life, float width, float end_width, float fade_length, float amplitude,
										float brightness, float speed, int start_frame,
										float framerate, float r, float g, float b);
		virtual beam_t* create_beam_points(beam_info_t& beam_info);

		virtual void create_beam_ring(int start_ent, int end_ent, int model_index, int halo_index, float halo_scale,
									  float life, float width, float end_width, float fade_length, float amplitude,
									  float brightness, float speed, int start_frame,
									  float framerate, float r, float g, float b, int flags);
		virtual beam_t* create_beam_ring(beam_info_t& beam_info);

		virtual void create_beam_ring_point(const vec3d& center, float start_radius, float end_radius, int model_index, int halo_index, float halo_scale,
											float life, float width, float end_width, float fade_length, float amplitude,
											float brightness, float speed, int start_frame,
											float framerate, float r, float g, float b, int flags);
		virtual beam_t* create_beam_ring_point(beam_info_t& beam_info);

		virtual void create_beam_circle_points(int type, vec3d& start, vec3d& end,
											   int model_index, int halo_index, float halo_scale, float life, float width,
											   float end_width, float fade_length, float amplitude, float brightness, float speed,
											   int start_frame, float framerate, float r, float g, float b);
		virtual beam_t* create_beam_circle_points(beam_info_t& beam_info);

		virtual void create_beam_follow(int start_ent, int model_index, int halo_index, float halo_scale,
										float life, float width, float end_width, float fade_length, float r, float g, float b,
										float brightness);
		virtual beam_t* create_beam_follow(beam_info_t& beam_info);
	};
} // namespace sdk