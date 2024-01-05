#include "override_entity_list.hpp"
#include "../../sdk/class_id.hpp"
#include "../interfaces.hpp"
#include "grenade_prediction.hpp"
#include "../features/misc.hpp"
#include "../globals.hpp"
#include "../interfaces.hpp"

using namespace sdk;

enum e_act {
	ACT_NONE,
	ACT_THROW,
	ACT_LOB,
	ACT_DROP,
};

STFI void trace_hull(vec3d& src, vec3d& end, trace_t& tr) {
	ray_t ray;
	ray.init(src, end, vec3d{ -2.0f, -2.0f, -2.0f }, vec3d{ 2.0f, 2.0f, 2.0f });

	trace_filter_world_and_props_only_t filter;
	interfaces::traces->trace_ray(ray, 0x200400B, &filter, &tr);
}

STFI void setup(int act, vec3d& vecSrc, vec3d& vecThrow, vec3d viewangles) {
	vec3d angThrow = viewangles;
	float pitch = angThrow.x;

	if (pitch <= 90.0f) {
		if (pitch < -90.0f) {
			pitch += 360.0f;
		}
	} else {
		pitch -= 360.0f;
	}
	float a = pitch - (90.0f - fabs(pitch)) * 10.0f / 90.0f;
	angThrow.x = a;

	// Gets ThrowVelocity from weapon files
	// Clamped to [15,750]
	float flVel = 750.0f * 0.9f;

	// Do magic on member of grenade object [esi+9E4h]
	// m1=1  m1+m2=0.5  m2=0
	static const float power[] = { 1.0f, 1.0f, 0.5f, 0.0f };
	float b = power[act];
	// Clamped to [0,1]
	b = b * 0.7f;
	b = b + 0.3f;
	flVel *= b;

	vec3d vForward, vRight, vUp;
	math::angle_vectors(angThrow, vForward, vRight, vUp); //angThrow.ToVector(vForward, vRight, vUp);

	vecSrc = globals->m_nades_eye_position;
	float off = (power[act] * 12.0f) - 12.0f;
	vecSrc.z += off;

	// Game calls UTIL_TraceHull here with hull and assigns vecSrc tr.endpos
	trace_t tr;
	vec3d vecDest = vecSrc;
	vecDest += vForward * 22.0f; //vecDest.MultAdd(vForward, 22.0f);

	trace_hull(vecSrc, vecDest, tr);

	// After the hull trace it moves 6 units back along vForward
	// vecSrc = tr.endpos - vForward * 6
	vec3d vecBack = vForward;
	vecBack *= 6.0f;
	vecSrc = tr.m_end;
	vecSrc -= vecBack;

	// Finally calculate velocity
	vec3d vel = globals->m_local->velocity();
	if (vel.length() < 5.f)
		vel = vec3d{};

	vecThrow = vel;
	vecThrow *= 1.25f;
	vecThrow += vForward * flVel; //	vecThrow.MultAdd(vForward, flVel);
}

STFI void simulate(std::vector<vec3d>& path, int act) {
	vec3d str, vec_throw;
	vec3d angles = interfaces::engine->get_view_angles();
	setup(act, str, vec_throw, angles);

	float interval = interfaces::global_vars->m_interval_per_tick;

	// Log positions 20 times per sec
	int logstep = static_cast<int>(0.05f / interval);
	int logtimer = 0;

	{
		if (projectiles->m_mutex.try_lock()) {
			path.clear();
			grenade_prediction->set_type(
					[&]() {
						const auto projectile = globals->m_local->active_weapon();
						switch (globals->m_local->active_weapon()->item_definition_index()) {
							case e_weapon_type::weapon_flashbang: return projectile_entry_t::e_type::flashbang;
							case e_weapon_type::weapon_hegrenade: return projectile_entry_t::e_type::hegrenade;
							case e_weapon_type::weapon_decoy: return projectile_entry_t::e_type::decoy;
							case e_weapon_type::weapon_inc:
							case e_weapon_type::weapon_molotov: return projectile_entry_t::e_type::fire;
							case e_weapon_type::weapon_smokegrenade: return projectile_entry_t::e_type::smokegrenade;
							case e_weapon_type::weapon_snowball: return projectile_entry_t::e_type::snowball;
							default: return projectile_entry_t::e_type::unknown;
						}
					}());

			for (unsigned int i = 0; i < path.max_size() - 1; ++i) {
				if (!logtimer)
					path.push_back(str);

				int s = grenade_prediction->step(str, vec_throw, i, interval);
				if ((s & 1)) break;

				// Reset the log timer every logstep OR we bounced
				if ((s & 2) || logtimer >= logstep) logtimer = 0;
				else
					++logtimer;
			}
			path.push_back(str);
			projectiles->m_mutex.unlock();
		}
	}
}

constexpr float csgo_armor(float damage, int armor_value) {
	constexpr auto armor_ratio = 0.5f;
	constexpr auto armor_bonus = 0.5f;
	if (armor_value > 0) {
		auto new_damage = damage * armor_ratio;
		auto armor = (damage - new_damage) * armor_bonus;

		if (armor > (float)(armor_value)) {
			armor = (float)(armor_value) * (1.f / armor_bonus);
			new_damage = damage - armor;
		}

		return new_damage;
	}

	return damage;
}

STFI int get_grenade_damage(base_entity_t* projectile, cs_player_t* entity, vec3d position) {
	if (entity == nullptr || !entity->is_alive())
		return 0;

	// some magic values by VaLvO
	static constexpr float a = 105.0f;
	static constexpr float b = 25.0f;
	static constexpr float c = 140.0f;

	auto origin = entity->origin();
	auto collideable = entity->collideable();

	auto min = collideable->mins() + origin;
	auto max = collideable->maxs() + origin;

	auto center = min + (max - min) * 0.5f;

	// get delta between center of mass and final nade pos.
	auto delta = center - position;
	if (delta.length() > 350.f)
		return 0;

	ray_t ray;
	trace_t trace;
	trace_filter_t filter;
	filter.m_skip = projectile;
	ray.init(position, center);

	interfaces::traces->trace_ray(ray, MASK_SHOT_BRUSHONLY, &filter, &trace);

	if (trace.m_entity != entity)
		return 0;

	float d = ((delta.length() - b) / c);
	float damage = a * exp(-d * d);
	int entity_armor = entity->armor();
	return min(max(static_cast<int>(ceilf(csgo_armor(damage, entity_armor))), 0), entity_armor > 0 ? 57 : 98);
}

void projectiles_t::on_create_move(user_cmd_t* cmd) {
	//if (!settings->grenade_esp.prediction)
	//	return;

	bool attack = cmd->m_buttons.has(in_attack);
	bool attack2 = cmd->m_buttons.has(in_attack2);

	m_act = (attack && attack2) ? ACT_LOB : (attack2) ? ACT_DROP
									: (attack)		  ? ACT_THROW
													  : ACT_NONE;
}

void projectiles_t::on_override_view() {
	//if (!settings->grenade_esp.prediction)
	//	return;

	auto weapon = globals->m_local->active_weapon();

	if (weapon && weapon->is_projectile() && m_act != ACT_NONE) {
		m_type = weapon->item_definition_index();
		simulate(m_predicted_path, m_act);
	} else {
		m_type = 0;
		{
			THREAD_SAFE(m_mutex);
			m_predicted_path.clear();
		}
	}
}

STFI std::vector<vec3d> convex_hull(std::vector<vec3d> points) {
	static auto orientation = [](vec3d p, vec3d q, vec3d r) -> int {
		int val = (q.y - p.y) * (r.x - q.x) -
				  (q.x - p.x) * (r.y - q.y);

		if (val == 0) return 0;
		return (val > 0) ? 1 : 2;
	};

	std::vector<vec3d> hull;
	size_t n = points.size();

	if (n < 3) return hull;

	int l = 0;
	for (int i = 1; i < n; i++)
		if (points[i].x < points[l].x)
			l = i;
	int p = l, q;

	do {
		hull.push_back(points[p]);

		q = (p + 1) % n;
		for (int i = 0; i < n; i++) {
			if (orientation(points[p], points[i], points[q]) == 2)
				q = i;
		}
		p = q;

	} while (p != l);

	return hull;
}

STFI void update_infernos() {
	THREAD_SAFE(entities->m_mutex);

	for (auto& [inferno, entry]: entities->m_infernos) {
		const auto& inferno_origin = inferno->origin();
		entry.m_time_to_die = (((*(float*)(uintptr_t(inferno) + 0x20)) + 7.03125f) - interfaces::global_vars->m_curtime);

		const auto m_bFireIsBurning = inferno->fire_is_burning();
		const auto m_fireXDelta = inferno->fire_delta_x();
		const auto m_fireYDelta = inferno->fire_delta_y();
		const auto m_fireZDelta = inferno->fire_delta_z();
		const auto m_fireCount = inferno->fire_count();

		entry.m_entity_origin = inferno_origin;
		entry.m_range = 0.f;

		vec3d average_vector{};
		std::vector<vec3d> points;
		for (int i = 0; i <= m_fireCount; i++) {
			if (!m_bFireIsBurning[i])
				continue;

			vec3d fire_origin = vec3d(m_fireXDelta[i], m_fireYDelta[i], m_fireZDelta[i]);
			float delta = fire_origin.length() + 14.4f;
			if (delta > entry.m_range)
				entry.m_range = delta;

			average_vector += fire_origin;
			if (fire_origin == vec3d(0, 0, 0))
				continue;

			//fire_origin.x += 60.f * cos(fire_origin.x);
			//fire_origin.y += 60.f * sin(fire_origin.y);

			points.emplace_back(fire_origin);
		}
		entry.m_points = convex_hull(points);

		for (auto& p: entry.m_points) {
			//p.x += inferno_origin.x + 60.f * sin(p.x);
			//p.y += inferno_origin.y + 60.f * cos(p.y);
			//p.z += inferno_origin.z;
			p += inferno_origin;
		}

		if (m_fireCount <= 1)
			entry.m_origin = inferno_origin;
		else
			entry.m_origin = (average_vector / m_fireCount) + inferno_origin;
	}
}

STFI void update_projectiles() {
	THREAD_SAFE(entities->m_mutex);

	for (auto& [projectile, entry]: entities->m_projectiles) {
		entry.m_entity = projectile;
		if (projectile->dormant())
			continue;

		// do it once
		if (entry.m_type == projectile_entry_t::e_type::unitialized) {
			entry.m_type = [&]() {
				switch (projectile->client_class()->m_class_id) {
					case e_class_id::CBaseCSGrenadeProjectile: {
						const auto model = projectile->get_model();
						if (model != nullptr && strstr(model->m_name, STRC("flashbang")) != nullptr)
							return projectile_entry_t::e_type::flashbang;
						else
							return projectile_entry_t::e_type::hegrenade;
					}
					case e_class_id::CDecoyProjectile: return projectile_entry_t::e_type::decoy;
					case e_class_id::CMolotovProjectile: return projectile_entry_t::e_type::fire;
					case e_class_id::CSmokeGrenadeProjectile: return projectile_entry_t::e_type::smokegrenade;
					case e_class_id::CSnowballProjectile: return projectile_entry_t::e_type::snowball;
					default: return projectile_entry_t::e_type::unknown;
				}
			}();

			switch (entry.m_type) {
				case projectile_entry_t::e_type::flashbang:
					entry.m_icon = (char*)misc->get_weapon_icon(e_weapon_type::weapon_flashbang);
					break;
				case projectile_entry_t::e_type::hegrenade:
					entry.m_icon = (char*)misc->get_weapon_icon(e_weapon_type::weapon_hegrenade);
					break;
				case projectile_entry_t::e_type::fire:
					entry.m_icon = (char*)misc->get_weapon_icon(e_weapon_type::weapon_molotov);
					break;
				case projectile_entry_t::e_type::smokegrenade:
					entry.m_icon = (char*)misc->get_weapon_icon(e_weapon_type::weapon_smokegrenade);
					break;
				case projectile_entry_t::e_type::decoy:
					entry.m_icon = (char*)misc->get_weapon_icon(e_weapon_type::weapon_decoy);
					break;
			}

			entry.m_spawn_time = interfaces::global_vars->m_curtime;

			if (entry.m_type != projectile_entry_t::e_type::unitialized)
				entry.m_predicted_path = grenade_prediction->predict_projectile_path(projectile, entry.m_type, entry.m_spawn_time, entry.m_detonate_time);
		} else {
			if (entry.m_type == projectile_entry_t::e_type::hegrenade)
				entry.m_exploded = projectile->hegrenade_exploded();
			else if (entry.m_type == projectile_entry_t::e_type::smokegrenade)
				entry.m_exploded = projectile->smoke_effect_tick_begin() > 0;

			vec3d source = projectile->origin(), velocity = projectile->velocity();
			if (entry.m_type != projectile_entry_t::e_type::unitialized && entry.m_old_velocity != projectile->velocity()) {
				float temp = 0.f;
				entry.m_predicted_path = grenade_prediction->predict_projectile_path(projectile, entry.m_type, entry.m_spawn_time, temp);
			}
		}

		if (globals->m_local_alive && entry.m_type == projectile_entry_t::e_type::hegrenade && !entry.m_predicted_path.empty())
			entry.m_predicted_damage = get_grenade_damage(projectile, globals->m_local, entry.m_predicted_path.back());
		else
			entry.m_predicted_damage = 0;

		entry.m_origin = projectile->origin();
		entry.m_distance_to_local = globals->m_local_alive ? entry.m_predicted_path.back().dist(globals->m_local->origin()) : 0.f;
		entry.m_old_velocity = projectile->velocity();
		entry.m_thrown_by_local = false;
		if (const auto thrower = (cs_player_t*)projectile->thrower().get(); thrower != nullptr) {
			if (thrower == globals->m_local)
				entry.m_thrown_by_local = true;
		}
	}
}

void projectiles_t::on_frame_render_start() {
	update_infernos();
	update_projectiles();
}