#pragma once
#include "../src/utils/matrix.hpp"

namespace sdk {
	struct base_entity_t;
	struct cs_player_t;

	enum class e_trace_type {
		everything = 0,
		world_only,
		entities_only,
		everything_filter_props,
	};

	struct i_trace_filter {
		virtual bool should_hit_entity(base_entity_t* entity, int mask) = 0;
		virtual e_trace_type get_trace_type() const = 0;
	};

	struct trace_filter_t : i_trace_filter {
		bool should_hit_entity(base_entity_t* entity, int /*contentsMask*/);

		virtual e_trace_type get_trace_type() const {
			return e_trace_type::everything;
		}

		__forceinline void set_ignore_class(char* c) {
			m_ignore = c;
		}

		void* m_skip;
		char* m_ignore = new char[1];
	};

	struct trace_filter_friendly_t : i_trace_filter {
		trace_filter_friendly_t(base_entity_t* entity) : m_entity(entity) {}

		bool should_hit_entity(base_entity_t* entity, int) override;

		__forceinline e_trace_type get_trace_type() const { return e_trace_type::everything; }
		void* m_entity;
	};

	struct trace_filter_one_entity_t : i_trace_filter {
		__forceinline bool should_hit_entity(base_entity_t* entity, int /*contentsMask*/) {
			return (entity == m_entity);
		}

		__forceinline e_trace_type get_trace_type() const { return e_trace_type::everything; }
		void* m_entity;
	};

	struct trace_filter_skip_entity_t : i_trace_filter {
		__forceinline trace_filter_skip_entity_t(base_entity_t* entity) { m_skip = entity; }

		__forceinline bool should_hit_entity(base_entity_t* entity, int /*contentsMask*/) {
			return !(entity == m_skip);
		}

		virtual e_trace_type get_trace_type() const {
			return e_trace_type::everything;
		}

		void* m_skip;
	};

	struct trace_filter_entities_only_t : i_trace_filter {
		bool should_hit_entity(base_entity_t* entity, int /*contentsMask*/) {
			return true;
		}
		virtual e_trace_type get_trace_type() const {
			return e_trace_type::entities_only;
		}
	};

	struct trace_filter_world_only_t : i_trace_filter {
		__forceinline bool should_hit_entity(base_entity_t* /*pServerEntity*/, int /*contentsMask*/) {
			return false;
		}
		virtual e_trace_type get_trace_type() const {
			return e_trace_type::world_only;
		}
	};

	struct trace_filter_world_and_props_only_t : i_trace_filter {
		__forceinline bool should_hit_entity(base_entity_t* /*pServerEntity*/, int /*contentsMask*/) {
			return false;
		}
		virtual e_trace_type get_trace_type() const {
			return e_trace_type::everything;
		}
	};

	struct trace_filter_players_only_skip_one_t : i_trace_filter {
		__forceinline trace_filter_players_only_skip_one_t(base_entity_t* ent) {
			e = ent;
		}

		bool should_hit_entity(base_entity_t* entity, int /*contentsMask*/);

		virtual e_trace_type get_trace_type() const {
			return e_trace_type::entities_only;
		}

		base_entity_t* e;
	};

	struct trace_filter_skip_two_entities_t : i_trace_filter {
		__forceinline trace_filter_skip_two_entities_t(base_entity_t* ent1, base_entity_t* ent2) {
			e1 = ent1;
			e2 = ent2;
		}

		__forceinline bool should_hit_entity(base_entity_t* entity, int /*contentsMask*/) {
			return !(entity == e1 || entity == e2);
		}

		virtual e_trace_type get_trace_type() const {
			return e_trace_type::everything;
		}

	private:
		base_entity_t* e1;
		base_entity_t* e2;
	};

	struct trace_filter_hit_all_t : trace_filter_t {
		virtual bool should_hit_entity(base_entity_t* /*pServerEntity*/, int /*contentsMask*/) {
			return true;
		}
	};

	struct brush_side_info_t {
		vec4d m_plane;
		uint16_t m_bevel;
		uint16_t m_thin;
	};

	struct phys_collide_t;

	struct cplane_t {
		vec3d m_normal;
		float m_dist;
		uint8_t m_type;
		uint8_t m_signbits;
		uint8_t m_pad[2];
	};

	struct vcollide_t {
		uint16_t m_solid_count : 15;
		uint16_t m_is_packed : 1;
		uint16_t m_desc_size;
		phys_collide_t** m_solids;
		char* m_key_values;
		void* m_user_data;
	};

	struct cmodel_t {
		vec3d m_mins, m_maxs;
		vec3d m_origin;
		int32_t m_headnode;
		vcollide_t m_vcollision_data;
	};

	struct csurface_t {
		const char* m_name;
		int16_t m_surface_props;
		uint16_t m_flags;
	};

	struct ray_t {
		vec3d_aligned m_start;
		vec3d_aligned m_delta;
		vec3d_aligned m_start_offset;
		vec3d_aligned m_extents;
		const matrix3x4_t* m_world_axis_transform;
		bool m_is_ray;
		bool m_is_swept;

		__forceinline ray_t() : m_world_axis_transform(NULL) {}

		__forceinline ray_t(vec3d const& start, vec3d const& end) {
			m_delta = end - start;

			m_is_swept = (m_delta.length_sqr() != 0);

			m_extents.set();

			m_world_axis_transform = NULL;
			m_is_ray = true;

			m_start_offset.set();
			m_start = start;
		}

		__forceinline ray_t(vec3d const& start, vec3d const& end, vec3d const& mins, vec3d const& maxs) {
			m_delta = { end - start };
			m_world_axis_transform = nullptr;
			m_is_swept = m_delta.length_sqr() != 0.f;
			m_extents = { maxs - mins };
			m_extents *= 0.5f;
			m_is_ray = m_extents.length_sqr() < 1e-6;
			m_start_offset = { mins + maxs };
			m_start_offset *= 0.5f;
			m_start = { start + m_start_offset };
			m_start_offset *= -1.f;
		}

		__forceinline void init(vec3d const& start, vec3d const& end) {
			m_delta = end - start;

			m_is_swept = (m_delta.length_sqr() != 0);

			m_extents.set();

			m_world_axis_transform = NULL;
			m_is_ray = true;

			// Offset m_Start to be in the center of the box...
			m_start_offset.set();
			m_start = start;
		}

		__forceinline void init(vec3d const& start, vec3d const& end, vec3d const& mins, vec3d const& maxs) {
			m_delta = end - start;

			m_world_axis_transform = NULL;
			m_is_swept = (m_delta.length_sqr() != 0);

			m_extents = maxs - mins;
			m_extents *= 0.5f;
			m_is_ray = (m_extents.length_sqr() < 1e-6);

			// Offset m_Start to be in the center of the box...
			m_start_offset = maxs + mins;
			m_start_offset *= 0.5f;
			m_start = start + m_start_offset;
			m_start_offset *= -1.0f;
		}

		__forceinline vec3d inv_delta() const {
			vec3d inv_delta{};
			for (int i = 0; i < 3; ++i) {
				if (m_delta[i] != 0.0f)
					inv_delta[i] = 1.0f / m_delta[i];
				else
					inv_delta[i] = FLT_MAX;
			}

			return inv_delta;
		}
	};
} // namespace sdk