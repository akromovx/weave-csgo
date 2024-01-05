#pragma once
#include "bspflags.h"
#include "../src/utils/vector.hpp"
#include "entities.hpp"
#include "client_class.hpp"
#include "trace_define.hpp"

namespace sdk {
	struct base_trace_t {
		__forceinline bool is_disp_surface() { return ((m_disp_flags & DISPSURF_FLAG_SURFACE) != 0); }
		__forceinline bool is_disp_surface_walkable() { return ((m_disp_flags & DISPSURF_FLAG_WALKABLE) != 0); }
		__forceinline bool is_disp_surface_buildable() { return ((m_disp_flags & DISPSURF_FLAG_BUILDABLE) != 0); }
		__forceinline bool is_disp_surface_prop1() { return ((m_disp_flags & DISPSURF_FLAG_SURFPROP1) != 0); }
		__forceinline bool is_disp_surface_prop2() { return ((m_disp_flags & DISPSURF_FLAG_SURFPROP2) != 0); }

		vec3d m_start{};
		vec3d m_end{};
		cplane_t m_plane{};

		float m_fraction{};

		int m_contents{};
		uint16_t m_disp_flags{};

		bool m_all_solid{};
		bool m_start_solid{};

		__forceinline base_trace_t() {}
	};

	struct game_trace_t : base_trace_t {
		bool did_hit_world() const;
		bool did_hit_non_world_entity() const;
		__forceinline bool did_hit() const { return m_fraction < 1 || m_all_solid || m_start_solid; }
		__forceinline bool is_visible() const { return m_fraction > 0.97f; }

		float m_fraction_left_solid{};
		csurface_t m_surface{};
		e_hitgroup m_hitgroup{};
		int16_t m_physics_bone{};
		uint16_t m_world_surface_index{};
		base_entity_t* m_entity{};
		int32_t m_hitbox{};

		__forceinline game_trace_t() {}

	private:
		__forceinline game_trace_t(const game_trace_t& other) : m_fraction_left_solid(other.m_fraction_left_solid),
																m_surface(other.m_surface),
																m_hitgroup(other.m_hitgroup),
																m_physics_bone(other.m_physics_bone),
																m_world_surface_index(other.m_world_surface_index),
																m_entity(other.m_entity),
																m_hitbox(other.m_hitbox) {
			m_start = other.m_start;
			m_end = other.m_end;
			m_plane = other.m_plane;
			m_fraction = other.m_fraction;
			m_contents = other.m_contents;
			m_disp_flags = other.m_disp_flags;
			m_all_solid = other.m_all_solid;
			m_start_solid = other.m_start_solid;
		}
	};

	struct engine_trace_t {
		virtual int get_point_contents(const vec3d& position, int contents_mask = MASK_ALL, base_entity_t** p_entity = nullptr) = 0;
		virtual int get_point_contents_world_only(const vec3d& position, int contents_mask = MASK_ALL) = 0;
		virtual int get_point_contents_collideable(collideable_t* collide, const vec3d& position) = 0;
		virtual void clip_ray_to_entity(const ray_t& ray, unsigned int mask, base_entity_t* e, game_trace_t* trace) = 0;
		virtual void clip_ray_to_collideable(const ray_t& ray, unsigned int mask, collideable_t* collide, game_trace_t* trace) = 0;
		virtual void trace_ray(const ray_t& ray, unsigned int mask, i_trace_filter* trace_filter, game_trace_t* trace) = 0;
	};
} // namespace sdk