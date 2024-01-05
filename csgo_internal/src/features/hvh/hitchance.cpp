#include "hvh.hpp"
#include "../../game/players.hpp"
#include "../../interfaces.hpp"
#include "../../globals.hpp"
#include "../../utils/easings.hpp"
#include "../../utils/threading.hpp"
#include "../../game/engine_prediction.hpp"
#include "autowall.hpp"

namespace hvh {
	using namespace sdk;

	float can_hit_hitbox(mstudiobbox_t* bbox, const vec3d& start, const vec3d& end, cs_player_t* player, e_hitbox hitbox, matrix3x4_t* bones) {
		vec3d min{}, max{};
		if (bbox->m_radius != -1.f) {
			//if (hitbox == e_hitbox::head) {
			//	trace_t tr;
			//	ray_t ray;
			//	ray.init(start, end);
			//	interfaces::traces->clip_ray_to_entity(ray, MASK_SHOT | CONTENTS_GRATE, player, &tr);
			//	if (tr.m_entity == player && tr.m_hitgroup == e_hitgroup::head)
			//		return 1.f;

			//	return 0.f;
			//}

			math::vector_transform(bbox->m_min, bones[bbox->m_bone], min);
			math::vector_transform(bbox->m_max, bones[bbox->m_bone], max);

			const auto dist = math::segment_to_segment(start, end, min, max);
			constexpr float hitbox_corner_smooth = 1 / 0.075f;
			float dist_ratio = 1.f - dist / bbox->m_radius;
			const float hitchance = std::clamp(dist_ratio * hitbox_corner_smooth, 0.f, 1.f);

			if (dist <= bbox->m_radius)
				return 1.f;
		} else {
			trace_t tr{};
			ray_t ray{};
			ray.init(start, end);
			interfaces::traces->clip_ray_to_entity(ray, MASK_SHOT | CONTENTS_GRATE, player, &tr);

			if (tr.m_entity == player) {
				if (hitbox_to_hitgroup(hitbox) == tr.m_hitgroup)
					return 1.f;
			}
		}

		return 0.f;
	}

	STFI void push_trace(mstudiobbox_t* bbox, int current, float* hits, bool optimization, int its, int j, float oang, float dist,
						 float range, cs_player_t* player, e_hitbox hitbox, matrix3x4_t* bones, vec3d start, vec3d forward, vec3d right, vec3d up, bool check_wallbang = false) {
		const auto& current_hitgroup = get_hitboxes_in_hitgroup(hitbox);
		float ang = fPI * 2.f / its * j + oang,
			  spread_x = cos(ang) * dist,
			  spread_y = sin(ang) * dist;

		vec3d total_spread = (forward + right * spread_x + up * spread_y).normalize_in_place(), spread_angle, end;

		math::vector_angles(total_spread, spread_angle);
		math::angle_vectors(spread_angle, end);

		end = start + end.normalize_in_place() * range;

		/*{
			vec3d a = start - end, b = start - position;
			float ratio = a.x * b.x + a.y * b.y + a.z * b.z;

			float spread_angle_f = acos(ratio / (a.Length() * b.Length()));
			if (spread_angle_f > Ragebot::arrMaxSpreadAngle[idx])
				Ragebot::arrMaxSpreadAngle[idx] = spread_angle_f;
		}*/

		if (optimization) {
			hits[current] += can_hit_hitbox(bbox, start, end, player, hitbox, bones);
		} else {
			for (auto current_hitbox: current_hitgroup) {
				auto hit = can_hit_hitbox(bbox, start, end, player, current_hitbox, bones);
				if (hit > 0) {
					hits[current] += hit;
					break;
				}
			}
		}
	}

	inline bool calculate_hitchance(float* out_chance, float weapon_inaccuracy, lag_record_t* record, const vec3d& eye_position, const vec3d& position,
									float chance, e_hitbox hitbox, matrix3x4_t* bones, bool optimization, bool use_autowall) {
		int idx = record->m_player->index();

		*out_chance = 0.f;
		if (CVAR_BOOL("weapon_accuracy_nospread")) {
			// Ragebot::arrMaxSpreadAngle[idx] = FLT_MAX;
			*out_chance = 1.f;
			return true;
		}

		if (chance == 0.f) {
			// Ragebot::m_arrMaxSpreadAngle[idx] = FLT_MAX;
			*out_chance = 1.f;
			return true;
		}

		const auto weapon = globals->m_weapon;

		if (weapon == nullptr)
			return false;

		const auto info = weapon->get_cs_weapon_info();

		if (info == nullptr)
			return false;

		//const auto weapon_inaccuracy = engine_prediction->m_unpredicted_data.m_spread + engine_prediction->m_unpredicted_data.m_inaccuracy/*weapon->get_inaccuracy() + weapon->get_spread()*/;

		vec3d start = eye_position;
		const auto aim_angle = math::calculate_angle(start, position);
		vec3d forward, right, up;
		math::angle_vectors(aim_angle, forward, right, up);

		static float its_mul = 0.15f;
		static int rings = 6, total_points = []() -> int {
			int _total_points = 0;
			for (int i = 0; i < rings; ++i) {
				int its_base = (i + 12),
					its = (int)std::floor(its_base * its_base * its_mul) + 6 /* min points count */;
				for (int j = 0; j < its; ++j, ++_total_points)
					;
			}
			return _total_points;
		}();

		float hc_mult = 1.f;
		int weap_def_idx = weapon->item_definition_index();
		switch (weap_def_idx) {
			case e_weapon_type::weapon_xm1014: hc_mult = 6.f; break;
			case e_weapon_type::weapon_nova: hc_mult = 9.f; break;
			case e_weapon_type::weapon_sawedoff: hc_mult = 8.f; break;
			case e_weapon_type::weapon_mag7: hc_mult = 8.f; break;
		}

		chance /= hc_mult;

		int curr_point = 0;

		studiohdr_t* hdr = interfaces::model_info->get_studio_model(record->m_player->get_model());
		mstudiohitboxset_t* set = hdr->hitbox_set(0);

		if (set == nullptr)
			return false;

		mstudiobbox_t* bbox = set->hitbox((int)hitbox);
		if (bbox == nullptr)
			return false;

		float curr_dist_base = 1.f, curr_dist_step = -1.f / (float)rings, oang = 0.f;

		threading_t::callbacks_t traces{};
		int current = 0;

		float* hits = new float[total_points]{};
		float* hits_mindmg = new float[total_points]{};

		for (int i = 0; i < rings; ++i, curr_dist_base += curr_dist_step) {
			float dist = curr_dist_base * weapon_inaccuracy;
			int its_base = (i + 12),
				its = (int)std::floor(its_base * its_base * its_mul) + 6 /* min points count */;

			for (int j = 0; j < its; ++j, ++curr_point) {
				traces.emplace_back([=]() {
					if (use_autowall) {
						const auto ang = fPI * 2.f / its * j + oang,
								   spread_x = cos(ang) * dist,
								   spread_y = sin(ang) * dist;

						vec3d total_spread = (forward + right * spread_x + up * spread_y).normalize_in_place(), spread_angle, end;

						math::vector_angles(total_spread, spread_angle);
						math::angle_vectors(spread_angle, end);

						end = start + end.normalize_in_place() * info->m_range;
						auto pen_info = autowall->run(start, end, globals->m_weapon, record, record->m_player);
						if (pen_info.m_did_hit && pen_info.m_hitgroup == hitbox_to_hitgroup(hitbox)) {
							hits[current] = pen_info.m_damage;
							hits_mindmg[current] = pen_info.m_min_damage;
						}
					} else
						push_trace(bbox, current, hits, optimization, its, j, oang, dist, info->m_range, record->m_player, hitbox, bones, start, forward, right, up);
				});

				++current;
			}

			oang += (((i & 1) == 0) ? fPI : -fPI) / 2.f;
		}

		threading->run(traces);

		int hited{};
		float hitbox_chance = 0.f;
		const auto mindamage = (float)aimbot->get_minimum_damage(record->m_player->health()) - 1.f;
		for (int i = 0; i < total_points; ++i) {
			if (hits[i] <= 0.f)
				continue;

			if (use_autowall) {
				if (hits[i] >= mindamage /*&& hits_mindmg[i] >= mindamage*/)
					++hited;
			} else {
				hitbox_chance += hits[i];
				++hited;
			}
		}

		delete[] hits;
		delete[] hits_mindmg;

		*out_chance = std::clamp((float)hited / (float)total_points * hc_mult, 0.f, 1.f);
		return *out_chance >= chance;
	}

	inline float calculate_hitchance_fast(lag_record_t* record, const vec3d& eye_position, const vec3d& position, e_hitbox hitbox, matrix3x4_t* bones) {
		if (CVAR_BOOL("weapon_accuracy_nospread"))
			return 1.f;

		const auto weapon = globals->m_weapon;

		if (!weapon)
			return false;

		const auto info = weapon->get_cs_weapon_info();

		if (!info)
			return false;

		const auto weapon_inaccuracy = engine_prediction->m_unpredicted_data.m_spread + engine_prediction->m_unpredicted_data.m_inaccuracy /*weapon->get_inaccuracy() + weapon->get_spread()*/;

		// calculate start and angle.
		vec3d start = eye_position;
		const auto aim_angle = math::calculate_angle(start, position);
		vec3d forward, right, up;
		math::angle_vectors(aim_angle, forward, right, up);

		static float its_mul = 0.02f;
		static int rings = 6, total_points = []() -> int {
			int _total_points = 0;
			for (int i = 0; i < rings; ++i) {
				int its_base = (i + 12),
					its = (int)std::floor(its_base * its_base * its_mul) + 6 /* min points count */;
				for (int j = 0; j < its; ++j, ++_total_points)
					;
			}
			return _total_points;
		}();

		float hc_mult = 1.f;
		int weap_def_idx = weapon->item_definition_index();

		switch (weap_def_idx) {
			case e_weapon_type::weapon_xm1014: hc_mult = 6.f; break;
			case e_weapon_type::weapon_nova: hc_mult = 9.f; break;
			case e_weapon_type::weapon_sawedoff: hc_mult = 8.f; break;
			case e_weapon_type::weapon_mag7: hc_mult = 8.f; break;
		}

		studiohdr_t* hdr = interfaces::model_info->get_studio_model(record->m_player->get_model());
		mstudiohitboxset_t* set = hdr->hitbox_set(0);

		if (set == nullptr)
			return false;

		mstudiobbox_t* bbox = set->hitbox((int)hitbox);
		if (bbox == nullptr)
			return false;

		int curr_point = 0;
		float hits = 0.f;
		float curr_dist_base = 1.f, curr_dist_step = -1.f / (float)rings, oang = 0.f;
		for (int i = 0; i < rings; ++i, curr_dist_base += curr_dist_step) {
			float dist = curr_dist_base * weapon_inaccuracy;
			int its_base = (i + 12),
				its = (int)std::floor(its_base * its_base * its_mul) + 6 /* min points count */;

			for (int j = 0; j < its; ++j, ++curr_point) {
				float ang = fPI * 2.f / its * j + oang,
					  spread_x = cos(ang) * dist,
					  spread_y = sin(ang) * dist;

				vec3d total_spread = (forward + right * spread_x + up * spread_y).normalize_in_place(), spread_angle, end;

				math::vector_angles(total_spread, spread_angle);
				math::angle_vectors(spread_angle, end);
				end = start + end.normalize_in_place() * info->m_range;

				hits += (can_hit_hitbox(bbox, start, end, record->m_player, hitbox, bones) > 0.0f) ? 1.f : 0.f;

				// interfaces::debug_overlay->add_line_overlay(start, end, 255, 255, 255, false, interfaces::global_vars->m_interval_per_tick * 2.f);
			}

			oang += (((i & 1) == 0) ? fPI : -fPI) / 2.f;
		}

		return std::min<float>((hits / (float)total_points) * hc_mult, 1.f);
	}
} // namespace hvh