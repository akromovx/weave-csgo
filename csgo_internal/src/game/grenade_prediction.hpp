#pragma once
#include "../../sdk/entities.hpp"
#include "../base_includes.hpp"
#include "../utils/vector.hpp"
#include "grenades.hpp"

struct grenade_prediction_t {
	std::vector<vec3d> predict_projectile_path(sdk::base_grenade_t* projectile, projectile_entry_t::e_type type, float time_threw, float& detonate_time);
	int step(vec3d& source, vec3d& velocity, int tick, float interval);
	void set_type(projectile_entry_t::e_type type) { m_type = type; }

private:
	projectile_entry_t::e_type m_type{ projectile_entry_t::e_type::unknown };
	bool check_detonate(const vec3d& vecThrow, const sdk::trace_t& tr, int tick, float interval);
	void trace_hull(vec3d& src, vec3d& end, sdk::trace_t& tr);
	void add_gravity_move(vec3d& move, vec3d& vel, float frametime, bool onground);
	void push_entity(vec3d& src, const vec3d& move, sdk::trace_t& tr);
	void resolve_fly_collision(sdk::trace_t& tr, vec3d& vecVelocity, float interval);
	int physics_clip_velocity(const vec3d& in, const vec3d& normal, vec3d& out, float overbounce);
};

GLOBAL_DYNPTR(grenade_prediction_t, grenade_prediction);