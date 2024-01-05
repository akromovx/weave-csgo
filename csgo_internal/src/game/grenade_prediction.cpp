#include "grenade_prediction.hpp"
#include "../interfaces.hpp"

using namespace sdk;

std::vector<vec3d> grenade_prediction_t::predict_projectile_path(base_grenade_t* projectile, projectile_entry_t::e_type type, float time_threw, float& detonate_time) {
	// apply projectile type
	m_type = type;

	// log positions 20 times per sec
	int log_step = (int)(0.05f / interfaces::global_vars->m_interval_per_tick), log_timer = 0;

	vec3d velocity = projectile->velocity();
	vec3d source = projectile->origin();
	int delta_ticks = TIME_TO_TICKS(interfaces::global_vars->m_curtime - time_threw);

	std::vector<vec3d> path{};
	for (size_t i = 0; i < path.max_size() - 1; ++i) {
		if (log_timer == 0)
			path.emplace_back(source);

		int current_tick = i + delta_ticks + /* magic number */ 3;
		int s = this->step(source, velocity, current_tick, interfaces::global_vars->m_interval_per_tick);

		detonate_time = interfaces::global_vars->m_curtime + TICKS_TO_TIME(current_tick);
		if (s & 1) break;

		// reset the log timer every logstep or we bounced
		if (s & 2 || log_timer >= log_step)
			log_timer = 0;
		else
			++log_timer;
	}

	path.emplace_back(source);
	return path;
}

int grenade_prediction_t::step(vec3d& source, vec3d& velocity, int tick, float interval) {
	// apply gravity
	vec3d move;
	this->add_gravity_move(move, velocity, interval, false);

	// push entity
	trace_t tr;
	this->push_entity(source, move, tr);

	int result = 0;
	// check ending conditions
	if (this->check_detonate(velocity, tr, tick, interval))
		result |= 1;

	// resolve collisions
	if (tr.m_fraction != 1.0f) {
		result |= 2; // collision!
		this->resolve_fly_collision(tr, velocity, interval);
	}

	// set new position
	source = tr.m_end;
	return result;
}

bool grenade_prediction_t::check_detonate(const vec3d& velocity, const trace_t& trace, int tick, float interval) {
	switch (m_type) {
		case projectile_entry_t::e_type::decoy:
		case projectile_entry_t::e_type::smokegrenade:
			// velocity must be < 0.1f, this is only checked every 0.2s
			if (velocity.length2d() < 0.1f)
				return !(tick % (int)(0.2f / interval));
			return false;

		case projectile_entry_t::e_type::fire:
			// detonate when hitting the floor
			if (trace.m_fraction != 1.0f && trace.m_plane.m_normal.z > 0.7f)
				return true;

			// or we've been flying for too long
			return (float)tick * interval > 2.0f && tick % (int)(0.2f / interval) == 0;

		case projectile_entry_t::e_type::flashbang:
		case projectile_entry_t::e_type::hegrenade:
			// pure timer based, detonate at 1.5s, checked every 0.2s
			return (float)tick * interval > 1.5f && tick % (int)(0.2f / interval) == 0;
		default:
			return true;
	}
}

void grenade_prediction_t::trace_hull(vec3d& src, vec3d& end, trace_t& tr) {
	ray_t ray{};
	constexpr vec3d grenade_maxs = { 2.f, 2.f, 2.f };
	constexpr vec3d grenade_mins = grenade_maxs * -1.f;

	ray.init(src, end, grenade_mins, grenade_maxs);

	trace_filter_world_and_props_only_t filter{};
	interfaces::traces->trace_ray(ray, 0x200400B, &filter, &tr);
}

void grenade_prediction_t::add_gravity_move(vec3d& move, vec3d& vel, float frametime, bool onground) {
	move.x = vel.x * frametime;
	move.y = vel.y * frametime;

	if (!onground) {
		float newZ = vel.z - ((CVAR_FLOAT("sv_gravity") * 0.4f) * frametime);
		move.z = ((vel.z + newZ) / 2.0f) * frametime;
		vel.z = newZ;
	} else
		move.z = vel.z * frametime;
}

void grenade_prediction_t::push_entity(vec3d& source, const vec3d& move, trace_t& tr) {
	vec3d end = source + move;
	this->trace_hull(source, end, tr);
}

void grenade_prediction_t::resolve_fly_collision(trace_t& tr, vec3d& velocity, float interval) {
	constexpr float surface_elasticity = 1.0;
	constexpr float grenade_elasticity = 0.45f;
	float total_elasticity = std::clamp(grenade_elasticity * surface_elasticity, 0.f, 0.9f);

	// calculate bounce
	vec3d abs_velocity;
	this->physics_clip_velocity(velocity, tr.m_plane.m_normal, abs_velocity, 2.0f);
	abs_velocity *= total_elasticity;

	// stop completely once we move too slow
	constexpr float min_speed_sqr = 20.0f * 20.0f;
	if (abs_velocity.length_sqr() < min_speed_sqr)
		abs_velocity.set();

	// stop if on ground
	if (tr.m_plane.m_normal.z > 0.7f) {
		velocity = abs_velocity;
		abs_velocity *= ((1.0f - tr.m_fraction) * interval);
		this->push_entity(tr.m_end, abs_velocity, tr);
	} else
		velocity = abs_velocity;
}

int grenade_prediction_t::physics_clip_velocity(const vec3d& in, const vec3d& normal, vec3d& out, float overbounce) {
	int blocked{};

	if (normal[2] > 0)
		blocked |= 1; // floor

	if (normal[2] == 0)
		blocked |= 2; // step

	float backoff = in.dot(normal) * overbounce;
	for (int i = 0; i < 3; i++) {
		auto& it = out[i];
		it = in[i] - (normal[i] * backoff);
		if (it > -0.1f && it < 0.1f)
			it = 0.f;
	}

	return blocked;
}