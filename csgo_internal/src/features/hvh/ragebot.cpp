#include "autowall.hpp"
#include "exploits.hpp"
#include "hvh.hpp"
#include "peek_state.hpp"

#include "../../globals.hpp"
#include "../../interfaces.hpp"

#include "../../utils/hotkeys.hpp"
#include "../../utils/threading.hpp"

#include "../../game/custom_animations.hpp"
#include "../../game/engine_prediction.hpp"
#include "../../game/override_entity_list.hpp"

#include "../visuals/chams.hpp"
#include "../visuals/local_esp.hpp"
#include "../visuals/logs.hpp"

#include "../bullets.hpp"
#include "../movement.hpp"

#include "../../utils/nt.hpp"

namespace hvh {
	using namespace sdk;

	STFI int32_t lookup_bone(base_entity_t* entity, const char* name) {
		return patterns::lookup_bone.as<int32_t(__thiscall*)(base_entity_t*, const char*)>()(entity, name);
	}

	STFI vec3d matrix_get_origin(const matrix3x4_t& src) {
		return { src[0][3], src[1][3], src[2][3] };
	}

	STFI std::optional<animation_state_t> predict_animation_state(cs_player_t* player) {
		auto& animstate = players->m_local_player.m_real_state;

		if (animstate == nullptr || !players->m_local_player.m_valid)
			return std::nullopt;

		animation_layers_t backup_layers{};
		float backup_poses[24]{};
		const animation_state_t backup_state = *animstate;
		const auto curtime_backup = interfaces::global_vars->m_curtime;

		player->store_layers(backup_layers);
		player->store_poses(backup_poses);

		if (animstate->m_last_update_frame >= interfaces::global_vars->m_framecount)
			animstate->m_last_update_frame = interfaces::global_vars->m_framecount - 1;

		animstate->update(player->eye_angles().y, player->eye_angles().x);
		animation_state_t pred = *animstate;

		*animstate = backup_state;
		player->apply_layers(backup_layers);
		player->apply_poses(backup_poses);
		interfaces::global_vars->m_curtime = curtime_backup;

		return pred;
	}

	STFI void modify_eye_position(cs_player_t* player, const matrix3x4_t* bones, vec3d& pos) {
		const auto state = predict_animation_state(globals->m_local);

		if (!state.has_value())
			return;

		//const auto state = players->m_local_player.m_real_state;
		//if (state == nullptr)
		//	return;

		if (state->m_landing || state->m_anim_duck_amount != 0.f || player->ground_entity_handle().get() == nullptr) {
			const auto head = lookup_bone(player, STRC("head_0"));
			auto bone_pos = matrix_get_origin(bones[head]);

			//interfaces::debug_overlay->add_line_overlay(bone_pos, pos, 255, 255, 255, false, 1.f);

			const auto bone_z = bone_pos.z + 1.7f;
			if (pos.z > bone_z) {
				const auto view_modifier = std::clamp((fabsf(pos.z - bone_z) - 4.f) * 0.16666667f, 0.f, 1.f);
				const auto view_modifier_sqr = view_modifier * view_modifier;
				pos.z += (bone_z - pos.z) * (3.f * view_modifier_sqr - 2.f * view_modifier_sqr * view_modifier);
			}
		}
	}

	struct mstudio_pose_param_desc_t {
		int m_name_index{};

		inline char* const get_name(void) const { return (char*)this + m_name_index; }

		int m_flags{};

		float m_start{};
		float m_end{};
		float m_loop{};
	};

	mstudio_pose_param_desc_t* get_pose_parameter(studio_hdr_t* hdr, int index) {
		static auto fn = patterns::get_pose_parameter.as<mstudio_pose_param_desc_t*(__thiscall*)(studio_hdr_t*, int)>();
		return fn(hdr, index);
	}

	STFI float studio_set_pose_parameter(studio_hdr_t* hdr, int index, float value, float& ctl_value) {
		if (index < 0 || index > 24 || hdr == nullptr)
			return 0.f;

		auto pose_param = get_pose_parameter(hdr, index);
		if (pose_param == nullptr)
			return 0.f;

		if (pose_param->m_loop) {
			float wrap = (pose_param->m_start + pose_param->m_end) / 2.0f + pose_param->m_loop / 2.0f;
			float shift = pose_param->m_loop - wrap;

			value = value - pose_param->m_loop * floor((value + shift) / pose_param->m_loop);
		}

		ctl_value = std::clamp((value - pose_param->m_start) / (pose_param->m_end - pose_param->m_start), 0.f, 1.f);

		return ctl_value * (pose_param->m_end - pose_param->m_start) + pose_param->m_start;
	}

	vec3d aimbot_t::get_shoot_position(std::optional<vec3d> override_angle) {
		if (!override_angle.has_value())
			return globals->m_local->eye_position();

		vec3d shoot_pos = globals->m_local->eye_position();

		auto poses = globals->m_local->poses();
		const auto old_poses = poses[12];

		SET_AND_RESTORE(interfaces::global_vars->m_curtime, interfaces::global_vars->m_curtime);

		const auto command_number = interfaces::client_state->get_current_tick() - 1;
		auto& entry = engine_prediction->m_entries[command_number % max_input];
		if (entry.m_shift > 0 && entry.m_sequence == command_number) {
			interfaces::global_vars->m_curtime += TICKS_TO_TIME(entry.m_shift);
		}

		static auto deadflag = netvars->get_offset(HASH("DT_BasePlayer"), HASH("deadflag"));
		vec3d old_ang = *(vec3d*)((DWORD)globals->m_local + deadflag + 4);

		const vec3d angle = *override_angle;
		*(vec3d*)((DWORD)globals->m_local + deadflag + 4) = angle;

		//	globals->m_local->invalidate_bone_cache(interfaces::global_vars->m_framecount);

		globals->m_local->set_abs_angles({ 0.f, angle.y, 0.f });

		float new_angle = angle.x - 90.f;

		if (new_angle > 180.f)
			new_angle = new_angle - 360.f;

		new_angle = std::clamp(new_angle, -90.f, 90.f);

		//float pose = 0.f;;
		//new_angle = studio_set_pose_parameter(globals->m_local->studio_hdr(), 12, new_angle, pose);
		poses[12] = (angle.x + 89.f) / 180.f;

		players->is_setuping(globals->m_local) = true;
		globals->m_local->setup_bones(nullptr, -1, 0x7FF00, interfaces::global_vars->m_curtime);
		players->is_setuping(globals->m_local) = false;

		players->is_clamping(globals->m_local) = true;
		clamp_bones_in_bbox(globals->m_local, globals->m_local->bone_cache().base(), 0x7FF00, interfaces::global_vars->m_curtime, angle);
		players->is_clamping(globals->m_local) = false;

		//globals->m_local->store_bone_cache(players->m_local_player.m_bones);
		//math::change_bones_position(players->m_local_player.m_bones, sdk::max_bones, globals->m_local->render_origin(), vec3d{ 0.f, 0.f, 0.f });

		modify_eye_position(globals->m_local, globals->m_local->bone_cache().base(), shoot_pos);
		*(vec3d*)((DWORD)globals->m_local + deadflag + 4) = old_ang;

		poses[12] = old_poses;
		return shoot_pos;
	}

	STFI std::string extract_resolver_base_mode(uint32_t resolver_mode) {
		switch (resolver_mode & 0xFF) {
			case e_resolver_mode::animation_layers:
				return STR("anim | ");
			case e_resolver_mode::edge:
				return STR("edge | ");
			case e_resolver_mode::off:
				return STR("off | ");
			case e_resolver_mode::jumping:
				return STR("in air | ");
			case e_resolver_mode::jitter:
				return STR("jitter | ");
		}

		return STR("unknown | ");
	}

	STFI std::string extract_resolver_info_short(uint32_t resolver_mode) {
		if ((resolver_mode & (0xF << e_resolver_mode_offset::roll)) & 1)
			return extract_resolver_base_mode(resolver_mode) + STR(" & roll");

		return extract_resolver_base_mode(resolver_mode) + std::to_string((resolver_mode & 0xFF00) >> e_resolver_mode_offset::base_mode_id);
	}

	STFI incheat_vars::ragebot_settings_t get_current_settings(int group_id, int weapon_id) {
		auto group = &settings->ragebot.weapons[group_id];
		auto weapon = &group->settings[weapon_id];

		if (weapon->override_default)
			return *weapon;

		if (group->override_default)
			return group->settings[0];

		return settings->ragebot.weapons[0].settings[0];
	}

	STFI std::optional<incheat_vars::ragebot_settings_t> get_current_settings() {
		using namespace incheat_vars;

		switch (globals->m_weapon->item_definition_index()) {
			case e_weapon_type::weapon_glock: return get_current_settings(rage_group_pistols, 1);
			case e_weapon_type::weapon_usp: return get_current_settings(rage_group_pistols, 2);
			case e_weapon_type::weapon_usps: return get_current_settings(rage_group_pistols, 3);
			case e_weapon_type::weapon_dualberetta: return get_current_settings(rage_group_pistols, 4);
			case e_weapon_type::weapon_p250: return get_current_settings(rage_group_pistols, 5);
			case e_weapon_type::weapon_tec9: return get_current_settings(rage_group_pistols, 6);
			case e_weapon_type::weapon_fiveseven: return get_current_settings(rage_group_pistols, 7);
			case e_weapon_type::weapon_cz75: return get_current_settings(rage_group_pistols, 8);

			case e_weapon_type::weapon_deagle: return get_current_settings(rage_group_heavy_pistols, 1);
			case e_weapon_type::weapon_revolver: return get_current_settings(rage_group_heavy_pistols, 2);

			case e_weapon_type::weapon_nova: return get_current_settings(rage_group_shotguns, 1);
			case e_weapon_type::weapon_xm1014: return get_current_settings(rage_group_shotguns, 2);
			case e_weapon_type::weapon_sawedoff: return get_current_settings(rage_group_shotguns, 3);
			case e_weapon_type::weapon_mag7: return get_current_settings(rage_group_shotguns, 4);

			case e_weapon_type::weapon_m249: return get_current_settings(rage_group_heavies, 1);
			case e_weapon_type::weapon_negev: return get_current_settings(rage_group_heavies, 2);

			case e_weapon_type::weapon_mac10: return get_current_settings(rage_group_smgs, 1);
			case e_weapon_type::weapon_mp9: return get_current_settings(rage_group_smgs, 2);
			case e_weapon_type::weapon_mp7: return get_current_settings(rage_group_smgs, 3);
			case e_weapon_type::weapon_mp5sd: return get_current_settings(rage_group_smgs, 4);
			case e_weapon_type::weapon_ump45: return get_current_settings(rage_group_smgs, 5);
			case e_weapon_type::weapon_p90: return get_current_settings(rage_group_smgs, 6);
			case e_weapon_type::weapon_bizon: return get_current_settings(rage_group_smgs, 7);

			case e_weapon_type::weapon_galil: return get_current_settings(rage_group_rifles, 1);
			case e_weapon_type::weapon_famas: return get_current_settings(rage_group_rifles, 2);
			case e_weapon_type::weapon_ak47: return get_current_settings(rage_group_rifles, 3);
			case e_weapon_type::weapon_m4a1: return get_current_settings(rage_group_rifles, 4);
			case e_weapon_type::weapon_m4a1s: return get_current_settings(rage_group_rifles, 5);
			case e_weapon_type::weapon_sg553: return get_current_settings(rage_group_rifles, 6);
			case e_weapon_type::weapon_aug: return get_current_settings(rage_group_rifles, 7);

			case e_weapon_type::weapon_ssg08: return get_current_settings(rage_group_snipers, 1);
			case e_weapon_type::weapon_awp: return get_current_settings(rage_group_snipers, 2);

			case e_weapon_type::weapon_g3sg1: return get_current_settings(rage_group_autosnipers, 1);
			case e_weapon_type::weapon_scar20: return get_current_settings(rage_group_autosnipers, 2);

			default: return get_current_settings(rage_group_default, 0);
		}
	}

	int aimbot_t::get_minimum_damage(int health) {
		if (globals->m_weapon->item_definition_index() == e_weapon_type::weapon_zeusx27)
			return std::min<int>(health, 100);

		auto mindamage = hotkeys->override_damage.m_active ? aimbot->m_settings->mindamage_override : aimbot->m_settings->mindamage;
		if (mindamage > health)
			return mindamage <= 100 ? health + 1 : health + (mindamage - 100);

		return mindamage + 1;
	}

	__forceinline bool is_baim_hitbox(e_hitbox id) {
		switch (id) {
			case e_hitbox::pelvis:
			case e_hitbox::stomach:
			case e_hitbox::lower_chest:
			case e_hitbox::chest:
				return true;
		}

		return false;
	}

	STFI bool aimbot_begin() {
		aimbot->m_players.clear();
		aimbot->m_players.reserve(entities->m_players.size());

		aimbot->m_data_counter = 0;
		aimbot->m_total_points_scanned = 0;
		aimbot->m_eye_position = aimbot->m_shoot_position = globals->m_eye_position;
		aimbot->m_shot = false;
		aimbot->m_should_predictive_stop = false;

		for (auto& data: aimbot->m_data)
			data.reset();

		const float inaccuracy = globals->m_weapon->get_inaccuracy();
		aimbot->m_inaccuracy_delta = std::abs(inaccuracy - aimbot->m_last_inaccuracy);
		aimbot->m_last_inaccuracy = inaccuracy;
		return true;
	}

	STFI void aimbot_end() {
		for (auto player: aimbot->m_players)
			restore_player(player);

		aimbot->m_players.clear();
	}

	STFI bool performance_check(cs_player_t* player) {
		// this is a very bad way to do this, so this didn't work as expected

		/*static int skip_ticks{};
		if (!globals->m_frametime_dropped) {
			skip_ticks = 0;
			return true;
		}

		++skip_ticks;
		return player->index() % (skip_ticks + 1) == interfaces::global_vars->m_tickcount % (skip_ticks + 1);*/

		return true;
	}

	STFI void store_players() {
		for (auto& [player, entry]: entities->m_players) {
			if (player == globals->m_local || player->is_teammate() || !player->is_alive() || player->dormant() || player->gungame_immunity())
				continue;

			if (!entry.m_initialized)
				continue;

			if (performance_check(player)) {
				entry.m_last_valid_record = nullptr;
				backup_player(player);
				aimbot->m_players.emplace_back(player);
			}
		}
	}

	std::vector<e_hitbox> aimbot_t::get_hitboxes_to_scan(bool primary) {
		std::vector<e_hitbox> hitboxes{};
		hitboxes.reserve(9);

		if (globals->m_weapon->item_definition_index() == e_weapon_type::weapon_zeusx27)
			return { e_hitbox::chest, e_hitbox::stomach, e_hitbox::pelvis };

		if (primary) {
			if (/*!g_Binds[bind_baim].active && */ aimbot->m_settings->hitboxes.at(0))
				hitboxes.emplace_back(e_hitbox::head);

			if (aimbot->m_settings->hitboxes.at(5))
				hitboxes.emplace_back(e_hitbox::stomach);

			if (aimbot->m_settings->hitboxes.at(2))
				hitboxes.emplace_back(e_hitbox::chest);

			return hitboxes;
		}

		if (/*!g_Binds[bind_baim].active && */ aimbot->m_settings->hitboxes.at(0))
			hitboxes.emplace_back(e_hitbox::head);

		if (aimbot->m_settings->hitboxes.at(5))
			hitboxes.emplace_back(e_hitbox::stomach);

		if (aimbot->m_settings->hitboxes.at(3))
			hitboxes.emplace_back(e_hitbox::lower_chest);

		if (aimbot->m_settings->hitboxes.at(2))
			hitboxes.emplace_back(e_hitbox::chest);

		if (aimbot->m_settings->hitboxes.at(4))
			hitboxes.emplace_back(e_hitbox::pelvis);

		if (aimbot->m_settings->hitboxes.at(1))
			hitboxes.emplace_back(e_hitbox::upper_chest);

		if (aimbot->m_settings->hitboxes.at(6)) {
			hitboxes.emplace_back(e_hitbox::left_shin);
			hitboxes.emplace_back(e_hitbox::right_shin);
			hitboxes.emplace_back(e_hitbox::left_thigh);
			hitboxes.emplace_back(e_hitbox::right_thigh);
		}

		if (aimbot->m_settings->hitboxes.at(7)) {
			hitboxes.emplace_back(e_hitbox::left_foot);
			hitboxes.emplace_back(e_hitbox::right_foot);
		}

		if (aimbot->m_settings->hitboxes.at(8)) {
			hitboxes.emplace_back(e_hitbox::left_hand);
			hitboxes.emplace_back(e_hitbox::right_hand);
			hitboxes.emplace_back(e_hitbox::left_forearm);
			hitboxes.emplace_back(e_hitbox::right_forearm);
			hitboxes.emplace_back(e_hitbox::left_upper_arm);
			hitboxes.emplace_back(e_hitbox::right_upper_arm);
		}

		return hitboxes;
	}

	struct capsule_t {
		vec3d m_min{};
		vec3d m_max{};
		float m_radius{};

		__forceinline capsule_t(const vec3d& mins = {}, const vec3d& maxs = {}, float radius = 0.f) : m_min(mins), m_max(maxs), m_radius(radius){};
	};

	STFI bool get_hitbox_capsule(mstudiobbox_t* bbox, matrix3x4_t* bones, capsule_t& capsule) {
		if (bbox->m_radius < 0.f)
			return false;

		vec3d mins{}, maxs{};
		math::vector_transform(bbox->m_min, bones[bbox->m_bone], mins);
		math::vector_transform(bbox->m_max, bones[bbox->m_bone], maxs);

		capsule = { mins, maxs, bbox->m_radius };
		return true;
	}

	STFI bool get_intersection_point(mstudiobbox_t* bbox, std::pair<matrix3x4_t*, matrix3x4_t*> bones, vec3d& intersection) {
		capsule_t first{}, second{};

		if (get_hitbox_capsule(bbox, bones.first, first) && get_hitbox_capsule(bbox, bones.second, second)) {
			vec3d center{};
			if (math::capsule_get_contact_center(first.m_min, first.m_max, first.m_radius, second.m_min, second.m_max, second.m_radius, center)) {
				intersection = center;
				return true;
			}
		}

		return false;
	}

	STFI void add_capsule_safe_point(std::vector<aimbot_t::point_t>& points, mstudiobbox_t* bbox, std::pair<matrix3x4_t*, matrix3x4_t*> bones) {
		vec3d point{};
		if (get_intersection_point(bbox, bones, point))
			if (!point.is_nan() && !point.is_zero())
				points.emplace_back(point, false, e_hitbox::head);
	}

	STFI float get_multipoint_scale(const vec3d& origin) {
		const auto weapon_info = globals->m_weapon->get_cs_weapon_info();
		if (weapon_info == nullptr)
			return 0.f;

		static auto scale = [](float x, float min, float max) {
			return 1.f - 1.f / (1.f + pow(2.f, (-([](float x, float min, float max) { return ((x - min) * 2.f) / (max - min) - 1.f; }(x, min, max)) / 0.115f)));
		};

		return std::clamp(scale(origin.dist(globals->m_eye_position), 0.f, weapon_info->m_range / 4.f), 0.f, 0.95f);
	}

	static inline float correct_multipoint_scale(float hitbox_radius, const vec3d& hitbox_center, const vec3d& point) {
		const auto weapon_spread = globals->m_weapon_spread + globals->m_weapon_inaccuracy;
		const auto dist = hitbox_center.dist(point) / std::sin(DEG2RAD(90.f - RAD2DEG(weapon_spread)));
		return std::clamp(hitbox_radius - dist * weapon_spread, 0.f, hitbox_radius * 0.9f);
	}

	void aimbot_t::collect_points(lag_record_t* record, float scale, const model_t* model, studiohdr_t* hdr,
								  mstudiohitboxset_t* set, matrix3x4_t* bones, e_hitbox hitbox, std::vector<aimbot_t::point_t>& points, bool primary_hitscan) {
		auto bbox = set->hitbox((int)hitbox);
		if (bbox == nullptr)
			return;

		auto add_point = [&points, &bones, &bbox, &hitbox](vec3d point, bool center = false, bool capsule = true) {
			if (capsule)
				math::vector_transform(point, bones[bbox->m_bone], point);

			points.emplace_back(point, center, hitbox);
		};

		if (bbox->m_radius > 0.f) {
			const auto hitbox_center = (bbox->m_min + bbox->m_max) * 0.5f;

			if (hitbox == e_hitbox::head) {
				if (aimbot->m_settings->head_scale != 0)
					scale = (float)aimbot->m_settings->head_scale * 0.01f;

				const float radius = bbox->m_radius * scale;
				static float rotation = std::cos(DEG2RAD(45.f));

				if (record != nullptr && !primary_hitscan) {
					add_point(hitbox_center, true);
					add_point({ bbox->m_max.x + (rotation * radius), bbox->m_max.y + (-rotation * radius), bbox->m_max.z });
				}

				add_point({ bbox->m_max.x, bbox->m_max.y - radius, bbox->m_max.z });

				if (record != nullptr && !globals->m_frametime_dropped) {
					if (!primary_hitscan) {
						if (settings->ragebot.computing_limit == 0) {
							add_capsule_safe_point(points, bbox, { record->m_right_bones, record->m_zero_bones });
							add_capsule_safe_point(points, bbox, { record->m_left_bones, record->m_zero_bones });
							add_capsule_safe_point(points, bbox, { record->m_left_bones, record->m_right_bones });
						}

						const auto left = vec3d{ bbox->m_max.x, bbox->m_max.y, bbox->m_max.z + radius };
						const auto right = vec3d{ bbox->m_max.x, bbox->m_max.y, bbox->m_max.z - radius };

						add_point({ bbox->m_max.x, bbox->m_max.y, bbox->m_max.z + correct_multipoint_scale(radius, hitbox_center, left) });
						add_point({ bbox->m_max.x, bbox->m_max.y, bbox->m_max.z - correct_multipoint_scale(radius, hitbox_center, right) });
					}
				}
			} else {
				if (aimbot->m_settings->body_scale != 0)
					scale = (float)aimbot->m_settings->body_scale * 0.01f;

				const float radius = bbox->m_radius * scale, r2 = bbox->m_radius * (scale * 2.f - 1.f);

				if (primary_hitscan && record != nullptr) {
					if (hitbox == e_hitbox::stomach) {
						const auto left = vec3d{ hitbox_center.x, hitbox_center.y, bbox->m_min.z + radius };
						const auto right = vec3d{ hitbox_center.x, hitbox_center.y, bbox->m_max.z - radius };

						add_point({ hitbox_center.x, hitbox_center.y, bbox->m_min.z + correct_multipoint_scale(radius, hitbox_center, right) });
						add_point({ hitbox_center.x, hitbox_center.y, bbox->m_max.z - correct_multipoint_scale(radius, hitbox_center, left) });
					} else
						add_point(hitbox_center, true);
				} else {
					if (hitbox == e_hitbox::stomach) {
						add_point(hitbox_center, true);

						if (!globals->m_frametime_dropped) {
							const auto left = vec3d{ hitbox_center.x, hitbox_center.y, bbox->m_max.z - radius };
							const auto right = vec3d{ hitbox_center.x, hitbox_center.y, bbox->m_min.z + radius };
							const auto back = vec3d{ hitbox_center.x, bbox->m_max.y - radius, hitbox_center.z };

							add_point({ hitbox_center.x, hitbox_center.y, bbox->m_max.z + correct_multipoint_scale(radius, hitbox_center, right) });
							add_point({ hitbox_center.x, hitbox_center.y, bbox->m_max.z - correct_multipoint_scale(radius, hitbox_center, left) });

							add_point({ hitbox_center.x, bbox->m_max.y - correct_multipoint_scale(radius, hitbox_center, back), hitbox_center.z });
						}
					} else if (hitbox == e_hitbox::pelvis || hitbox == e_hitbox::upper_chest) {
						add_point(hitbox_center, true);

						if (hitbox == e_hitbox::upper_chest) {
							const auto left = vec3d{ hitbox_center.x, hitbox_center.y, bbox->m_min.z - r2 };
							const auto right = vec3d{ hitbox_center.x, hitbox_center.y, bbox->m_max.z + r2 };

							add_point({ hitbox_center.x, hitbox_center.y, bbox->m_min.z - correct_multipoint_scale(radius, hitbox_center, right) });
							add_point({ hitbox_center.x, hitbox_center.y, bbox->m_max.z + correct_multipoint_scale(radius, hitbox_center, left) });
						}
					} else if (hitbox == e_hitbox::lower_chest || hitbox == e_hitbox::chest) {
						if (!globals->m_frametime_dropped) {
							const auto left = vec3d{ hitbox_center.x, hitbox_center.y, bbox->m_min.z - r2 };
							const auto right = vec3d{ hitbox_center.x, hitbox_center.y, bbox->m_max.z + r2 };

							add_point({ hitbox_center.x, hitbox_center.y, bbox->m_min.z - correct_multipoint_scale(radius, hitbox_center, right) });
							add_point({ hitbox_center.x, hitbox_center.y, bbox->m_max.z + correct_multipoint_scale(radius, hitbox_center, left) });
						}

						add_point({ hitbox_center.x, bbox->m_max.y - radius, hitbox_center.z }); // point on spine
					}

					else if (hitbox == e_hitbox::right_shin || hitbox == e_hitbox::left_shin) {
						add_point(hitbox_center, true);
						if (!globals->m_frametime_dropped)
							add_point({ bbox->m_max.x - (radius / 2.f), bbox->m_max.y, bbox->m_max.z }, false);
					}

					else if (hitbox == e_hitbox::right_thigh || hitbox == e_hitbox::left_thigh) {
						if (!globals->m_frametime_dropped)
							add_point(hitbox_center, true);
					} else if (hitbox == e_hitbox::right_upper_arm || hitbox == e_hitbox::left_upper_arm) {
						if (!globals->m_frametime_dropped)
							add_point({ bbox->m_max.x + radius, hitbox_center.y, hitbox_center.z });
					} else {
						if (!globals->m_frametime_dropped)
							add_point(hitbox_center, true);
					}
				}
			}
		}
	}

	void aimbot_t::collect_points(lag_record_t* record, matrix3x4_t* bones, const std::vector<e_hitbox>& hitboxes, std::vector<point_t>& points, bool primary_hitscan) {
		const auto model = record->m_player->get_model();
		if (model == nullptr)
			return;

		auto hdr = interfaces::model_info->get_studio_model(model);
		if (hdr == nullptr)
			return;

		auto set = hdr->hitbox_set(record->m_player->hitbox_set());
		if (set == nullptr)
			return;

		const auto player_center = record->m_player->origin() + record->m_player->view_offset() * 0.5f;

		for (auto hitbox: hitboxes)
			collect_points(record, get_multipoint_scale(player_center), model, hdr, set, bones, hitbox, points, primary_hitscan);
	}

	void aimbot_t::collect_points(cs_player_t* player, matrix3x4_t* bones, const std::vector<sdk::e_hitbox>& hitboxes, std::vector<point_t>& points, bool primary_hitscan) {
		const auto model = player->get_model();
		if (model == nullptr)
			return;

		auto hdr = interfaces::model_info->get_studio_model(model);
		if (hdr == nullptr)
			return;

		auto set = hdr->hitbox_set(player->hitbox_set());
		if (set == nullptr)
			return;

		const auto player_center = player->origin() + player->view_offset() * 0.5f;

		for (auto hitbox: hitboxes)
			collect_points(nullptr, get_multipoint_scale(player_center), model, hdr, set, bones, hitbox, points, primary_hitscan);
	}

	enum {
		safe_none = 0u,
		safe_right = 1u,
		safe_left = 2u,
		safe_all = safe_right | safe_left
	};

	void aimbot_t::scan_point(lag_record_t* record, point_t& point) {
		point.m_damage = 0;
		point.m_valid_damage = false;

		record->m_aimbot.m_primary = true;
		record->m_aimbot.m_safety = safe_none;

		const auto& info = autowall->run(m_eye_position, point.m_position, globals->m_weapon, record, nullptr, true);
		++aimbot->m_total_points_scanned;

		if (!info.m_did_hit)
			return;

		point.m_damage = (int)info.m_damage;

		if (info.m_hitgroup != hitbox_to_hitgroup(point.m_hitbox))
			return;

		if (info.m_damage < get_minimum_damage(record->m_player->health()))
			return;

		point.m_valid_damage = true;
		point.m_safe = record->m_aimbot.m_safety;
	}

	void aimbot_t::scan_points(lag_record_t* record, std::vector<point_t>& points) {
		for (auto& point: points)
			scan_point(record, point);
	}

	STFI int get_estimated_total_damage(lag_record_t* record) {
		record->apply(record->m_bones);

		std::vector<aimbot_t::point_t> points{};
		std::vector<sdk::e_hitbox> hitboxes{};
		switch (settings->ragebot.computing_limit) {
			case 0: hitboxes = { e_hitbox::head, e_hitbox::stomach, e_hitbox::left_foot, e_hitbox::right_foot }; break;
			case 1: hitboxes = { e_hitbox::head, e_hitbox::stomach, e_hitbox::left_foot, e_hitbox::right_foot }; break;
			case 2: hitboxes = { e_hitbox::head, e_hitbox::stomach }; break;
		}

		aimbot->collect_points(record, record->m_bones, hitboxes, points, settings->ragebot.computing_limit != 0);
		aimbot->scan_points(record, points);

		int ret = 0;
		for (const auto& point: points)
			ret += point.m_damage;

		return ret;
	}

	STFI lag_record_t* select_record(lag_record_t* first, lag_record_t* last) {
		if (first == last)
			return first;

		if (first != nullptr && last != nullptr)
			return get_estimated_total_damage(last) > get_estimated_total_damage(first) ? last : first;

		return first != nullptr ? first : last;
	}

	// this function is used to select the best record to shoot at
	STFI lag_record_t* select_record(aimbot_t::data_t& data) {
		auto entry = players->find_entry(data.m_player);
		if (entry == nullptr)
			return nullptr;

		auto& valid_records = data.m_valid_records = players->find_records_if(entry, lag_record_t::valid);

		if (valid_records.empty())
			return nullptr;

		if (valid_records.size() == 1)
			return valid_records.front();

		const auto vulnerable_record = ranges::find_if(valid_records, lag_record_t::vulnerable);
		const auto first = vulnerable_record != valid_records.end() ? *vulnerable_record : valid_records.front();

		return select_record(first, valid_records.back());
	}

	STFI bool can_stop() {
		if (movement->m_peek_state.m_going_back)
			return false;

		if (globals->m_cmd->m_buttons.has(in_speed) && globals->m_airtick > 0)
			return false;

		if (!aimbot->m_settings.has_value() || !aimbot->m_settings->quick_stop)
			return false;

		if (globals->m_weapon->item_definition_index() == e_weapon_type::weapon_zeusx27)
			return false;

		auto weapon_info = globals->m_weapon->get_cs_weapon_info();
		if (weapon_info == nullptr)
			return false;

		int tickbase = 0;
		if (exploits->m_type == exploits_t::e_type::hideshot)
			tickbase = globals->m_local->tickbase() - 8;

		const auto ticks_to_stop = get_ticks_to_standing_accuracy();
		const auto next_attack_close = get_ticks_to_shoot() <= ticks_to_stop && std::max<int>(TIME_TO_TICKS(globals->m_local->next_attack()) - globals->m_local->tickbase(), 0) <= ticks_to_stop;

		if (globals->m_weapon->item_definition_index() == e_weapon_type::weapon_revolver)
			return next_attack_close || aimbot->m_revolver_fire;

		const auto between_shots = weapon_info->m_cycle_time <= 0.4f && is_between_shots() /*&& !exploits->m_in_shift*/;
		return between_shots || next_attack_close || is_able_to_shoot(tickbase);
	}

	STFI bool is_valid_data(aimbot_t::data_t& data) {
		if (!data.m_best_point.has_value())
			return false;

		auto& best_point = *data.m_best_point;
		if (!best_point.m_valid_damage)
			return false;

		aimbot->m_should_work = true;
		return true;
	}

	STFI bool check_safety(e_hitbox hitbox, bool forced = false) {
		switch (hitbox) {
			case e_hitbox::head:
				return forced ? aimbot->m_settings->force_safepoint.at(0) : aimbot->m_settings->prefer_safepoint;
			case e_hitbox::pelvis:
			case e_hitbox::stomach:
			case e_hitbox::lower_chest:
			case e_hitbox::chest:
			case e_hitbox::upper_chest:
				return forced ? aimbot->m_settings->force_safepoint.at(1) : aimbot->m_settings->prefer_safepoint;
			case e_hitbox::right_thigh:
			case e_hitbox::left_thigh:
			case e_hitbox::right_shin:
			case e_hitbox::left_shin:
			case e_hitbox::left_foot:
			case e_hitbox::right_foot:
			case e_hitbox::right_hand:
			case e_hitbox::left_hand:
			case e_hitbox::right_upper_arm:
			case e_hitbox::right_forearm:
			case e_hitbox::left_upper_arm:
			case e_hitbox::left_forearm:
				return forced ? aimbot->m_settings->force_safepoint.at(2) : aimbot->m_settings->prefer_safepoint;
		}

		return false;
	}

	// predict how much damage we can deal with exploits
	int aimbot_t::correct_damage(int damage) {
		if (hotkeys->peek_assist.m_active)
			return damage;

		const auto bullets = exploits->get_exploit_bullets();
		if (exploits->m_dt_shots <= bullets && (exploits->m_dt_shots > 0 || exploits->m_charged))
			return damage * bullets;

		return damage;
	}

	STFI std::optional<aimbot_t::point_t> select_best_point(aimbot_t::data_t& data) {
		auto& points = data.m_points;
		if (points.empty())
			return std::nullopt;

		points.erase(std::remove_if(points.begin(), points.end(), [&](const aimbot_t::point_t& point) {
						 return check_safety(point.m_hitbox, true) && (point.m_safe & safe_all) != safe_all || !point.m_valid_damage;
					 }),
					 points.end());

		const auto hitchance = 1.f - aimbot->m_settings->hitchance * 0.01f;

		for (auto& point: points) {
			const auto calculated_chance = calculate_hitchance_fast(data.m_record, aimbot->m_eye_position, point.m_position, point.m_hitbox, data.m_record->m_bones);
			auto hitchance_correct = point.m_hitchance = calculated_chance;

			//switch (aimbot->m_settings->priority_hitgroup) {
			//case 1:
			//	if (point.m_hitbox == e_hitbox::head)
			//		hitchance_correct += hitchance * 0.25f;
			//	break;
			//case 2:
			//	switch (point.m_hitbox) {
			//	case e_hitbox::pelvis:
			//	case e_hitbox::stomach:
			//		hitchance_correct += hitchance * 0.25f;
			//	}
			//	break;
			//case 3:
			//	switch (point.m_hitbox) {
			//	case e_hitbox::lower_chest:
			//	case e_hitbox::chest:
			//	case e_hitbox::upper_chest:
			//		hitchance_correct += hitchance * 0.25f;
			//	}
			//	break;
			//}

			if (check_safety(point.m_hitbox) && (point.m_safe & safe_all) != 0 && calculated_chance >= 1.f) {
				if (point.m_safe & safe_right)
					hitchance_correct += hitchance * 0.25f;

				if (point.m_safe & safe_left)
					hitchance_correct += hitchance * 0.25f;
			}

			point.m_lethal = is_baim_hitbox(point.m_hitbox) && aimbot->correct_damage(point.m_damage) > data.m_player->health();

			if (point.m_lethal)
				hitchance_correct += hitchance + calculated_chance * 0.1f;

			if (point.m_center)
				hitchance_correct *= 1.1f;

			hitchance_correct += ((float)point.m_extra_safe / (float)aimbot_t::max_extra_safe_count);

			point.m_priority = (int)(hitchance_correct * 100.f);
		}

		if (points.empty())
			return std::nullopt;

		if (points.size() == 1)
			return points[0];

		ranges::sort(points, [](const auto& a, const auto& b) { return a.m_damage > b.m_damage; });
		ranges::sort(points, [](const auto& a, const auto& b) { return a.m_priority > b.m_priority; });

#ifdef _DEBUG
		int i = 0;
		for (const auto& p: points) {
			if (i == 0)
				interfaces::debug_overlay->add_text_overlay(p.m_position, interfaces::global_vars->m_interval_per_tick * 2.f, "x");
			else
				interfaces::debug_overlay->add_text_overlay(p.m_position, interfaces::global_vars->m_interval_per_tick * 2.f, "o");

			++i;
		}
#endif

		return points[0];
	}

	STFI void set_collision_bounds(void* collideable, vec3d* mins, vec3d* maxs) {
		static auto fn = patterns::set_collision_bounds.as<void(__thiscall*)(void*, vec3d*, vec3d*)>();
		fn(collideable, mins, maxs);
	}

	STFI void reclamp_bones(lag_record_t* record, const vec3d& origin, const vec3d& eye_angles, matrix3x4_t* bones) {
		auto player = record->m_player;
		auto& backup_record = aimbot->m_backup_records[player->index()];

		const auto doubletap = hvh::exploits->m_charged && hvh::exploits->m_type != hvh::exploits_t::e_type::none;
		const auto tickbase_error = doubletap && hvh::exploits->m_last_skip_shift;
		const auto curtime = interfaces::global_vars->m_curtime /*TICKS_TO_TIME(globals->m_tickbase - (tickbase_error ? (CVAR_INT("sv_maxusrcmdprocessticks") - 2) : 0))*/;

		//// 0_0 pizdec ////
		player->collision_change_height() = origin.z + (record->m_collision_change_height - record->m_origin.z);
		player->collision_change_time() = curtime + (record->m_collision_change_time - record->m_received_curtime);

		//player->collision_change_height() = origin.z + (backup_record.m_collision_change_height - backup_record.m_origin.z);
		//player->collision_change_time() = curtime + (backup_record.m_collision_change_time - interfaces::global_vars->m_curtime);

		auto collideable = player->collideable();

		SET_AND_RESTORE(collideable->mins(), backup_record.m_mins);
		SET_AND_RESTORE(collideable->maxs(), backup_record.m_maxs);

		clamp_bones_in_bbox(player, bones, 0x7FF00, curtime, eye_angles);

		player->collision_change_height() = backup_record.m_collision_change_height;
		player->collision_change_time() = backup_record.m_collision_change_time;
	}

	STFI bool add_extra_bones(aimbot_t::data_t& data, matrix3x4_t* bones, lag_record_t* record) {
		if (data.m_extra_safe_counter >= aimbot_t::max_extra_safe_count)
			return false;

		auto temp_matrix = aimbot->m_extra_safe[data.m_player->index()].m_bones[data.m_extra_safe_counter++];
		utils::memcpy_sse(temp_matrix, bones, sizeof(matrix3x4_t) * sdk::max_bones);
		record->m_player->set_abs_origin(data.m_record->m_origin);
		record->m_player->origin() = data.m_record->m_origin;

		reclamp_bones(record, data.m_record->m_origin, record->m_eye_angles, temp_matrix);
		return true;
	}

	STFI void for_each_extra_safe(const aimbot_t::data_t& data, std::function<void(matrix3x4_t*)> callback) {
		for (int i = 0; i < data.m_extra_safe_counter; ++i)
			callback(aimbot->m_extra_safe[data.m_player->index()].m_bones[i]);
	}

	STFI void handle_extra_safety(aimbot_t::data_t& data) {
		constexpr auto yaw_tolerance = 15.f;
		std::vector<lag_record_t*> different_records = data.m_valid_records;

		// sort by yaw difference
		std::sort(different_records.begin(), different_records.end(),
				  [original_yaw = data.m_record->m_eye_angles.y](const auto& a, const auto& b) {
					  const auto delta_a = std::abs(math::normalize_yaw(a->m_eye_angles.y - original_yaw));
					  const auto delta_b = std::abs(math::normalize_yaw(b->m_eye_angles.y - original_yaw));
					  return delta_a < delta_b;
				  });

		// erase elements beyond the unique range
		different_records.erase(
				std::unique(
						different_records.begin(), different_records.end(), [](const auto& a, const auto& b) {
							return std::abs(math::normalize_yaw(a->m_eye_angles.y - b->m_eye_angles.y)) < yaw_tolerance;
						}),
				different_records.end());

		if (different_records.size() > aimbot_t::max_extra_safe_count)
			different_records.erase(different_records.begin() + aimbot_t::max_extra_safe_count, different_records.end());

		data.m_extra_safe_max = different_records.size();

		for (auto record: different_records)
			if (!add_extra_bones(data, data.m_record->m_bones, record))
				break;

		auto player = data.m_player;
		studiohdr_t* hdr = interfaces::model_info->get_studio_model(player->get_model());
		if (hdr == nullptr)
			return;

		auto set = hdr->hitbox_set(0);
		if (set == nullptr)
			return;

		for_each_extra_safe(data, [&](auto bones) {
			for (auto& point: data.m_points) {
				auto bbox = set->hitbox((int)point.m_hitbox);
				if (bbox == nullptr)
					continue;

				if (can_hit_hitbox(bbox, globals->m_eye_position, point.m_position, player, point.m_hitbox, bones) == 1.f)
					++point.m_extra_safe;
			}
		});
	}

	STFI void scan_player(aimbot_t::data_t& data) {
		auto record = select_record(data);
		if (record == nullptr)
			return;

		data.m_points.reserve(XOR32(100));
		//record->apply(record->m_bones);

		data.m_player->set_abs_origin(record->m_origin);
		data.m_player->origin() = record->m_origin;

		// reclamp bones in bbox with several fixes
		{
			const auto& backup_record = aimbot->m_backup_records[record->m_player->index()];
			reclamp_bones(record, record->m_origin, backup_record.m_eye_angles, record->m_bones);
			reclamp_bones(record, record->m_origin, backup_record.m_eye_angles, record->m_inversed_bones);
			reclamp_bones(record, record->m_origin, backup_record.m_eye_angles, record->m_unresolved_bones);
		}

		data.m_player->set_bone_cache(record->m_bones);
		aimbot->collect_points(record, record->m_bones, aimbot->get_hitboxes_to_scan(), data.m_points);

		if (data.m_points.empty())
			return;

		data.m_record = record;

		handle_extra_safety(data);
		record->apply(record->m_bones);
		aimbot->scan_points(record, data.m_points);

		data.m_best_point = select_best_point(data);
		data.m_tickcount = TIME_TO_TICKS(record->m_simulation_time + lerp_time());
	}

	STFI auto& create_data(int i, user_cmd_t* cmd, cs_player_t* player) {
		auto& data = aimbot->m_data[i];
		data.m_player = player;
		data.m_command_number = cmd->m_command_number;
		data.m_tickcount = cmd->m_tickcount;
		data.m_best_point = std::nullopt;
		data.m_points.clear();
		return data;
	}

	STFI void scan_players_threads(user_cmd_t* cmd) {
		threading_t::callbacks_t callbacks{};

		for (size_t i = 0; i < aimbot->m_players.size(); ++i) {
			create_data(i, cmd, aimbot->m_players[i]);
			callbacks.emplace_back(
					[i]() {
						auto& data = aimbot->m_data[i];
						if (data.m_tickcount == globals->m_cmd->m_tickcount)
							scan_player(data);
					});
		}

		threading->run(callbacks);
	}

	STFI void scan_players_linear(user_cmd_t* cmd) {
		for (size_t i = 0; i < aimbot->m_players.size(); ++i) {
			auto& data = create_data(i, cmd, aimbot->m_players[i]);
			if (data.m_tickcount == globals->m_cmd->m_tickcount)
				scan_player(data);
		}
	}

	STFI void scan_players(user_cmd_t* cmd) {
#ifdef __NO_OBF
		//if (settings->multithreading.value())
		//	scan_players_threads(cmd);
		//else
		scan_players_linear(cmd);
#else
		scan_players_threads(cmd);
#endif
	}

	STFI void draw_capsule(cs_player_t* player, matrix3x4_t* bones, color_t color, float time) {
		studiohdr_t* hdr = interfaces::model_info->get_studio_model(player->get_model());
		if (hdr == nullptr)
			return;

		mstudiohitboxset_t* set = hdr->hitbox_set(0);
		if (set == nullptr)
			return;

		for (int i = 0; i < set->m_num_hitboxes; i++) {
			mstudiobbox_t* bbox = set->hitbox(i);
			if (bbox == nullptr)
				continue;

			vec3d mins{}, maxs{};
			math::vector_transform(bbox->m_min, bones[bbox->m_bone], mins);
			math::vector_transform(bbox->m_max, bones[bbox->m_bone], maxs);

			if (bbox->m_radius > 0.f)
				interfaces::debug_overlay->add_capsule_overlay(mins, maxs, bbox->m_radius, color.r(), color.g(), color.b(), color.a(), time, 0, 1);
		}
	}

	STFI std::string hitbox_to_string(e_hitbox hitbox) {
		switch (hitbox) {
			case e_hitbox::head: return STR("head");
			case e_hitbox::neck: return STR("neck");
			case e_hitbox::pelvis: return STR("pelvis");
			case e_hitbox::stomach: return STR("stomach");
			case e_hitbox::lower_chest: return STR("lower chest");
			case e_hitbox::chest: return STR("chest");
			case e_hitbox::upper_chest: return STR("upper chest");
			case e_hitbox::right_thigh: return STR("right thigh");
			case e_hitbox::left_thigh: return STR("left thigh");
			case e_hitbox::right_shin: return STR("right shin");
			case e_hitbox::left_shin: return STR("left shin");
			case e_hitbox::right_foot: return STR("right foot");
			case e_hitbox::left_foot: return STR("left foot");
			case e_hitbox::right_hand: return STR("right hand");
			case e_hitbox::left_hand: return STR("left hand");
			case e_hitbox::right_upper_arm: return STR("right upper arm");
			case e_hitbox::right_forearm: return STR("right forearm");
			case e_hitbox::left_upper_arm: return STR("left upper arm");
			case e_hitbox::left_forearm: return STR("left forearm");
		}

		return STR("?");
	}

	vec3d aimbot_t::get_advanced_point(cs_player_t* player, const vec3d& origin, float angle, float length) {
		vec3d forward{};
		math::angle_vectors({ 0.f, angle, 0.f }, forward);
		auto end = origin + forward * length;
		ray_t r{};
		trace_t t;
		trace_filter_t filter;
		filter.m_skip = player;

		r.init(origin, end, player->mins(), player->maxs());
		interfaces::traces->trace_ray(r, MASK_PLAYERSOLID, &filter, &t);

		return t.m_end;
	}

	vec3d aimbot_t::get_point(sdk::cs_player_t* player, sdk::e_hitbox hitbox, matrix3x4_t* bones) {
		studiohdr_t* studio_model = interfaces::model_info->get_studio_model(player->get_model());
		mstudiohitboxset_t* set = studio_model->hitbox_set(0);

		if (set == nullptr)
			return {};

		mstudiobbox_t* bbox = set->hitbox((int)hitbox);
		if (bbox == nullptr)
			return {};

		vec3d mins{}, maxs{};
		const auto modifier = bbox->m_radius != -1.f ? bbox->m_radius : 0.f;
		math::vector_transform(bbox->m_min + modifier, bones[bbox->m_bone], maxs);
		math::vector_transform(bbox->m_max - modifier, bones[bbox->m_bone], mins);
		return (mins + maxs) * 0.5f;
	}

	STFI bool shoot(user_cmd_t* cmd, aimbot_t::data_t& data) {
		int tickbase = 0;
		if (exploits->m_type == exploits_t::e_type::hideshot)
			tickbase = globals->m_local->tickbase() - 8;

		if (globals->m_weapon->item_definition_index() == e_weapon_type::weapon_revolver) {
			if (!aimbot->m_revolver_fire)
				return false;
		}

		if (!is_able_to_shoot(tickbase) || data.m_record == nullptr)
			return false;

		data.m_record->apply(data.m_record->m_bones);

		aimbot->m_eye_position = predict_local_player_origin(globals->m_local->origin(), globals->m_local->velocity(), globals->m_old_velocity, get_ticks_to_standing_accuracy()) + globals->m_local->view_offset();

		std::vector<aimbot_t::point_t> points{};
		bool has_valid_damage = false, has_lethal_point_far = false, has_lethal_point_near = false;

		if (!exploits->m_in_shift) {
			aimbot->collect_points(data.m_record, data.m_record->m_bones, aimbot->get_hitboxes_to_scan(true), points, true);
			aimbot->scan_points(data.m_record, points);

			has_valid_damage = ranges::any_of(points, [](const auto& p) { return p.m_valid_damage; });

			//has_lethal_point_far = ranges::any_of(peek_state->m_players, [](const auto& p) { return p.m_has_lethal_point; });
			//has_lethal_point_near = ranges::any_of(points,
			//	[health = data.m_player->health()](const auto& p) {
			//		return is_baim_hitbox(p.m_hitbox) && aimbot->correct_damage(p.m_damage) > health;
			//	}
			//);

			//const auto has_any_damage = ranges::any_of(peek_state->m_players, [](const auto& p) { return p.m_has_damage; });
			if (has_valid_damage)
				if (aimbot->m_settings->autoscope && globals->m_weapon->zoom_level() == 0 && (globals->m_weapon->is_sniper() /* || weapon is AUG or SG556*/))
					cmd->m_buttons.add(in_zoom);
		}

		data.m_can_stop = true;
		aimbot->m_last_target = data.m_player;

		if (!data.m_best_point.has_value() || !data.m_best_point->m_valid_damage) {
			if (!has_valid_damage)
				aimbot->m_last_target = nullptr;

			if (has_valid_damage) {
				aimbot->m_should_predictive_stop = true;
				return true;
			}

			return false;
		}

		auto& best_point = data.m_best_point.value();
		//if ((has_lethal_point_far && !has_lethal_point_near) && !best_point.m_lethal)
		//	return true;

		data.m_record->m_aimbot.m_safety = best_point.m_safe;
		data.m_record->m_aimbot.m_primary = false;

		globals->m_eye_position = aimbot->m_eye_position = aimbot->get_shoot_position(math::calculate_angle(globals->m_eye_position, best_point.m_position));

		//aimbot->m_eye_position = globals->m_eye_position;

		calculate_hitchance(&best_point.m_hitchance,
							globals->m_weapon->get_spread() + globals->m_weapon->get_inaccuracy()
							/*engine_prediction->m_unpredicted_data.m_spread + engine_prediction->m_unpredicted_data.m_inaccuracy*/,
							data.m_record, aimbot->m_eye_position, best_point.m_position,
							aimbot->m_settings->hitchance * 0.01f, best_point.m_hitbox, data.m_record->m_bones, false, true);

		const float inaccuracy = globals->m_weapon->get_inaccuracy();

		if (aimbot->m_inaccuracy_delta <= 0.00001f)
			++aimbot->m_ticks_inaccuracy_stop_changing;

		aimbot->m_last_inaccuracy = inaccuracy;

		bool can_stop = true;
		if (best_point.m_hitchance < aimbot->m_settings->hitchance * 0.01f) {
			if (can_stop) {
				if (!aimbot->m_settings->strict_hitchance) {
					if (aimbot->m_ticks_inaccuracy_stop_changing < 7)
						return true;
				} else
					return true;
			} else {
				data.m_can_stop = false;
				return true;
			}
		}

		if (settings->ragebot.autofire)
			cmd->m_buttons.add(e_buttons::in_attack);

		if (cmd->m_buttons.has(e_buttons::in_attack)) {
			cmd->m_tickcount = data.m_tickcount;
			cmd->m_viewangles = math::calculate_angle(aimbot->m_eye_position, best_point.m_position);

			aimbot->m_aim_position = cmd->m_viewangles;

			cmd->m_viewangles -= globals->m_local->punch_angle() * CVAR_FLOAT("weapon_recoil_scale");

			if (!settings->ragebot.silent)
				interfaces::engine->set_view_angles(cmd->m_viewangles);

			int bt = std::max<int>(0, interfaces::client_state->m_clock_drift_manager.m_server_tick - TIME_TO_TICKS(data.m_record->m_simulation_time));

			const auto message_colored = dformat(STR("$2Fired shot $3{} $2in $3{} $2for $3{}hp$2; $3{}%$2; $3sp: {}-{}-{}$2; $3bt: {}t$2; r: $3{}\n"), data.m_player->name(),
												 hitbox_to_string(best_point.m_hitbox), best_point.m_damage, (int)(best_point.m_hitchance * 100.f),
												 best_point.m_safe.get(), data.m_extra_safe_counter, data.m_extra_safe_max,
												 bt, extract_resolver_info_short(data.m_record->m_resolver.m_mode.m_value));

			const auto message_cheatlog = dformat(STR("Fired shot {} in {} for {}hp; {}%; sp: {}-{}-{}; bt: {}t; r: {}\n"), data.m_player->name(),
												  hitbox_to_string(best_point.m_hitbox), best_point.m_damage, (int)(best_point.m_hitchance * 100.f),
												  best_point.m_safe.get(), data.m_extra_safe_counter, data.m_extra_safe_max,
												  bt, extract_resolver_info_short(data.m_record->m_resolver.m_mode.m_value));

			//for_each_extra_safe(data, [&data](auto* bones) {
			//	draw_capsule(data.m_player, bones, color_t{}.modify_alpha(0.25f), 4.f);
			//});

			std::memcpy(&aimbot->m_last_fired_record, data.m_record, sizeof(lag_record_t));

			
			aimbot->m_on_weapon_fire = [message_colored, message_cheatlog, best_point,
										eye_pos = aimbot->m_eye_position,
										curtime = interfaces::global_vars->m_curtime](lag_record_t* record) {
				if (record == nullptr || !players->is_exist(record->m_player))
					return;

				auto& shot = bullet_impacts->m_shots.emplace_back();
				shot.m_eye_position = eye_pos;
				shot.m_time = curtime;
				shot.m_hitbox = best_point.m_hitbox;
				shot.m_safe = best_point.m_safe;
				shot.m_position = best_point.m_position;

				std::memcpy(&shot.m_lag_record, record, sizeof(lag_record_t));
				game_console->print_colored_id(message_colored);
				if (settings->misc.log_filter.at(0))
					cheat_logs->add_info(message_cheatlog);

				esp::chams->add_hitmatrix(record->m_player, record->m_bones);
				/*draw_capsule(record->m_player, record->m_bones, {}, 4.f);
						draw_capsule(record->m_player, record->m_inversed_bones, { 255, 35, 35 }, 4.f);
						draw_capsule(record->m_player, record->m_unresolved_bones, { 35, 35, 255 }, 4.f);*/
			};

			aimbot->m_shot = true;

			//for (int i = 0; i < data.m_points.size(); ++i) {
			//	const auto& point = data.m_points[i];
			//	printf("[%d] dmg: %d center: %d hc: %.2f priority: %d\n", i + 1, point.m_damage, point.m_center, point.m_hitchance, point.m_priority);
			//}
		}

		return true;
	}

	STFI void sort_players() {
		auto local_origin = globals->m_local->origin();

		// sort players by distance
		ranges::sort(aimbot->m_players, [=](cs_player_t* first, cs_player_t* second) {
			return first->origin().dist_sqr(local_origin) < second->origin().dist_sqr(local_origin);
		});

		if (exploits->m_in_shift && aimbot->m_players.size() > 2)
			aimbot->m_players.erase(aimbot->m_players.begin() + 1, aimbot->m_players.end());
	}

	STFI void run_aimbot(user_cmd_t* cmd) {
		aimbot->m_autostop_called = aimbot->m_should_predictive_stop = false;
		bool shot = false;

		const auto _can_stop = can_stop();

		for (int i = 0; i < 64; ++i) {
			auto& data = aimbot->m_data[aimbot->m_sorted_indices[i]];

			if (data.m_command_number == cmd->m_command_number) {
				if (shoot(cmd, data)) {
					//if (!cmd->m_buttons.has(in_attack) && data.m_can_stop && aimbot->m_should_predictive_stop && !aimbot->m_autostop_called && _can_stop)
					//	movement->slow_walk(globals->m_cmd, globals->m_original_angle, 3.4f);

					if (cmd->m_buttons.has(in_attack))
						aimbot->m_ticks_inaccuracy_stop_changing = 0;

					shot = true;
					break;
				}
			}

			// fix the memory leak
			data.reset();
		}

		if (!exploits->m_in_shift) {
			if (!shot)
				aimbot->m_last_target = nullptr;
		}
	}

	STFI void aimbot_reset() {
		if (!exploits->m_in_shift)
			aimbot->m_last_target = nullptr;

		aimbot->m_on_weapon_fire = nullptr;
		aimbot->m_aim_position = std::nullopt;
	}

	STFI bool can_work() {
		if (exploits->m_in_shift) {
			// prevent ragebot called a lot times while teleporting
			// if user has a weak PC it may cause huge FPS drops while shifting

			if (settings->ragebot.computing_limit != 0)
				return false;
		}

		if (!settings->ragebot.enable)
			return false;

		if (globals->m_local == nullptr || !globals->m_local->is_alive())
			return false;

		if (globals->m_weapon == nullptr || !globals->m_weapon->is_gun())
			return false;

		if (interfaces::game_rules->is_freeze_time() || globals->m_local->flags().has(fl_frozen))
			return false;

		//if (!can_stop(false))
		//	return false;

		return true;
	}

	STFI void prepare_autostop(user_cmd_t* cmd) {
		if (!aimbot->m_settings.has_value() || !aimbot->m_settings->quick_stop)
			return;

		//const auto ticks_to_stop = get_ticks_to_stop();
		//const auto next_attack_close =
		//	get_ticks_to_shoot() <= ticks_to_stop
		//	&& std::max<int>(TIME_TO_TICKS(globals->m_local->next_attack()) - globals->m_local->tickbase(), 0) <= ticks_to_stop;

		if (aimbot->m_last_target != nullptr && can_stop() || aimbot->m_should_predictive_stop)
			movement->slow_walk(cmd, globals->m_original_angle, (hotkeys->peek_assist.m_active && settings->ragebot.autofire) ? 6.8f : 3.4f);
	}

	void aimbot_t::on_pre_move(user_cmd_t* cmd) {
		prepare_autostop(cmd);
	}

	STFI bool compare_aimbot_data(const aimbot_t::data_t& first, const aimbot_t::data_t& second) {
		const auto& point1 = first.m_best_point;
		const auto& point2 = second.m_best_point;
		if (point1.has_value() && point2.has_value())
			return point1->m_priority > point2->m_priority;

		// and if we don't have both of the points valid let's compare they by having value
		return (int)point1.has_value() > (int)point2.has_value();
	}

	// this function is sorting the players by their priority
	STFI void sort_data() {
		// fill array with indices
		ranges::iota(aimbot->m_sorted_indices, 0);
		// sort indices by comparing the data
		ranges::sort(aimbot->m_sorted_indices, [](int i, int j) { return compare_aimbot_data(aimbot->m_data[i], aimbot->m_data[j]); });
	}

	STFI void setup_settings(incheat_vars::ragebot_settings_t& set) {
	}

	void aimbot_t::on_create_move(user_cmd_t* cmd) {
		aimbot_reset();

		if (!can_work())
			return;

		m_revolver_fire = auto_revolver(cmd);
		m_settings = get_current_settings();

		if (!m_settings.has_value())
			return;

		setup_settings(*m_settings);

		// if we have a valid settings let's run the aimbot
		if (aimbot_begin()) {
			store_players();
			sort_players();
			scan_players(cmd);
			sort_data();
			run_aimbot(cmd);
			aimbot_end();
		}
	}
} // namespace hvh