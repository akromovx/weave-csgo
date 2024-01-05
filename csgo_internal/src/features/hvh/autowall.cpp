#include "autowall.hpp"
#include "../../globals.hpp"
#include "../../interfaces.hpp"

namespace sdk {
	enum {
		char_tex_antlion = 'A',
		char_tex_bloodyflesh = 'B',
		char_tex_concrete = 'C',
		char_tex_dirt = 'D',
		char_tex_eggshell = 'E',
		char_tex_flesh = 'F',
		char_tex_grate = 'G',
		char_tex_alienflesh = 'H',
		char_tex_clip = 'I',
		char_tex_plastic = 'L',
		char_tex_metal = 'M',
		char_tex_sand = 'N',
		char_tex_foliage = 'O',
		char_tex_computer = 'P',
		char_tex_slosh = 'S',
		char_tex_tile = 'T',
		char_tex_cardboard = 'U',
		char_tex_vent = 'V',
		char_tex_wood = 'W',
		char_tex_glass = 'Y',
		char_tex_warpshield = 'Z',
	};

	bool trace_filter_t::should_hit_entity(base_entity_t* entity, int) {
		auto id = entity->client_class();
		if (id != nullptr && std::strcmp(m_ignore, STRC(""))) {
			if (id->m_network_name == m_ignore)
				return false;
		}

		return entity != m_skip;
	}

	bool trace_filter_players_only_skip_one_t::should_hit_entity(base_entity_t* entity, int) {
		return entity != e && entity->client_class()->m_class_id == e_class_id::CCSPlayer;
	}

	bool trace_filter_friendly_t::should_hit_entity(base_entity_t* entity, int) {
		auto player = (cs_player_t*)m_entity;
		return entity == nullptr || !entity->is_player() ||
			   (entity->index() != player->index() && ((cs_player_t*)entity)->team() == player->team());
	}
} // namespace sdk

namespace hvh {
	using namespace sdk;

	STFI bool is_breakable_entity(base_entity_t* entity) {
		if (entity == nullptr)
			return false;

		auto client_class = entity->client_class();

		if (client_class == nullptr)
			return false;

		return client_class->m_class_id == e_class_id::CBreakableSurface ||
			   (client_class->m_class_id == e_class_id::CBaseEntity && entity->collideable()->get_solid() == 1);
	}

	STFI bool trace_to_exit(game_trace_t& enter_trace, game_trace_t& exit_trace, vec3d start_position, vec3d direction) {
		constexpr auto max_distance = 90.f;
		constexpr auto ray_extension = 4.f;

		float current_distance = 0.f;
		int first_contents = 0;

		while (current_distance <= max_distance) {
			current_distance += ray_extension;

			auto start = start_position + direction * current_distance;

			if (first_contents == 0)
				first_contents = interfaces::traces->get_point_contents(start, MASK_SHOT_PLAYER);

			const auto point_contents = interfaces::traces->get_point_contents(start, MASK_SHOT_PLAYER);

			if ((point_contents & MASK_SHOT_HULL) == 0 || ((point_contents & CONTENTS_HITBOX) && point_contents != first_contents)) {
				const auto end = start - direction * ray_extension;

				ray_t r{};
				r.init(start, end);

				static const auto filter_simple = *(uint32_t*)((uint32_t)(patterns::filter_simple.as<void*>()) + 0x3D);
				uint32_t filter_ptr[4] = { filter_simple, (uint32_t)globals->m_local, 0, 0 };

				auto filter = (trace_filter_t*)(filter_ptr);

				interfaces::traces->trace_ray(r, MASK_SHOT_PLAYER, filter, &exit_trace);

				if (exit_trace.m_start_solid && exit_trace.m_surface.m_flags & SURF_HITBOX) {
					r.init(start, start_position);
					filter->m_skip = exit_trace.m_entity;
					interfaces::traces->trace_ray(r, MASK_SHOT_PLAYER, filter, &exit_trace);

					if (exit_trace.did_hit() && !exit_trace.m_start_solid) {
						start = exit_trace.m_end;
						return true;
					}

					continue;
				}

				if (exit_trace.did_hit() && !exit_trace.m_start_solid) {
					if (is_breakable_entity(enter_trace.m_entity) && is_breakable_entity(exit_trace.m_entity))
						return true;

					if (enter_trace.m_surface.m_flags & SURF_NODRAW || (!(exit_trace.m_surface.m_flags & SURF_NODRAW) && exit_trace.m_plane.m_normal.dot(direction) <= 1.f)) {
						const auto mult_amount = exit_trace.m_fraction * 4.f;
						start -= direction * mult_amount;
						return true;
					}
					continue;
				}

				if (!exit_trace.did_hit() || exit_trace.m_start_solid) {
					if (enter_trace.did_hit_non_world_entity() && is_breakable_entity(enter_trace.m_entity)) {
						exit_trace = enter_trace;
						exit_trace.m_end = start;
						return true;
					}
				}
			}
		}

		return false;
	}

	STFI bool handle_bullet_penetration(cs_weapon_info_t* info, game_trace_t& enter_trace, vec3d& eye_position, const vec3d& direction, int& penetration_count, float& current_damage) {
		game_trace_t exit_trace{};
		auto enemy = (cs_player_t*)enter_trace.m_entity;
		const auto enter_surface_data = interfaces::phys_surface->get_surface_data(enter_trace.m_surface.m_surface_props);
		const int enter_material = enter_surface_data->m_game.m_material;
		const bool is_grate = enter_trace.m_contents & CONTENTS_GRATE;
		const bool is_no_draw = !!(enter_trace.m_surface.m_flags & SURF_NODRAW);

		if ((penetration_count == 0 && is_grate && !is_no_draw && enter_material != char_tex_grate && enter_material != char_tex_glass) || (info->m_penetration <= 0.f || penetration_count <= 0) || (!trace_to_exit(enter_trace, exit_trace, enter_trace.m_end, direction) && !(interfaces::traces->get_point_contents(enter_trace.m_end, MASK_SHOT_HULL) & MASK_SHOT_HULL)))
			return false;

		const auto exit_surface_data = interfaces::phys_surface->get_surface_data(exit_trace.m_surface.m_surface_props);
		const auto exit_material = exit_surface_data->m_game.m_material;

		auto damage_lost = .16f;
		auto penetration_modifier = enter_surface_data->m_game.m_penetration_modifier;
		if (is_grate || is_no_draw || enter_material == char_tex_grate || enter_material == char_tex_glass) {
			if (enter_material == char_tex_grate || enter_material == char_tex_glass) {
				penetration_modifier = 3.f;
				damage_lost = .05f;
			} else
				penetration_modifier = 1.f;

		} else if (enter_material == char_tex_flesh && enemy && enemy->team() == globals->m_local->team() && CVAR_FLOAT("ff_damage_reduction_bullets") == 0.f) {
			const auto damage_bullet_penetration = CVAR_FLOAT("ff_damage_bullet_penetration");
			if (damage_bullet_penetration == 0.f)
				return false;

			penetration_modifier = damage_bullet_penetration;
		} else
			penetration_modifier = (penetration_modifier + exit_surface_data->m_game.m_penetration_modifier) / 2.f;

		if (enter_material == exit_material) {
			if (exit_material == char_tex_cardboard || exit_material == char_tex_wood)
				penetration_modifier = 3.f;
			else if (exit_material == char_tex_plastic)
				penetration_modifier = 2.f;
		}

		const auto dist = (exit_trace.m_end - enter_trace.m_end).length();
		const auto pen_mod = std::max<float>(0.f, (1.f / penetration_modifier));
		const auto wpn_mod = current_damage * damage_lost + std::max<float>(0.f, (3.f / info->m_penetration) * 1.25f) * (pen_mod * 3.f);
		const auto lost_damage = wpn_mod + (pen_mod * dist * dist) / 24.f;
		current_damage -= std::max<float>(0.f, lost_damage);

		if (current_damage < 1.f)
			return false;

		eye_position = exit_trace.m_end;
		--penetration_count;
		return true;
	}

	STFI float scale_damage(cs_player_t* target, float damage, float weapon_armor_ratio, e_hitgroup hitgroup) {
		const auto is_armored = [&]() -> bool {
			if (target->armor() > 0.f) {
				switch (hitgroup) {
					case e_hitgroup::generic:
					case e_hitgroup::chest:
					case e_hitgroup::stomach:
					case e_hitgroup::leftarm:
					case e_hitgroup::rightarm:
						return true;
					case e_hitgroup::head:
						return target->helmet();
					default:
						break;
				}
			}

			return false;
		};

		switch (hitgroup) {
			case e_hitgroup::head:
				if (target->heavy_armor())
					damage = (damage * 4.f) * .5f;
				else
					damage *= 4.f;
				break;
			case e_hitgroup::stomach:
				damage *= 1.25f;
				break;
			case e_hitgroup::leftleg:
			case e_hitgroup::rightleg:
				damage *= .75f;
				break;
			default:
				break;
		}

		if (is_armored()) {
			auto modifier = 1.f, armor_bonus_ratio = .5f, armor_ratio = weapon_armor_ratio * .5f;

			if (target->heavy_armor()) {
				armor_bonus_ratio = .33f;
				armor_ratio = (weapon_armor_ratio * .5f) * .5f;
				modifier = .33f;
			}

			auto new_damage = damage * armor_ratio;

			if (target->heavy_armor())
				new_damage *= .85f;

			if ((damage - damage * armor_ratio) * (modifier * armor_bonus_ratio) > target->armor())
				new_damage = damage - target->armor() / armor_bonus_ratio;

			damage = new_damage;
		}

		return damage;
	}

	static bool __declspec(noinline) trace_to_studio_csgo_hitgroups_priority(cs_player_t* player, uint32_t contents_mask, vec3d* origin, game_trace_t* tr, ray_t* r, matrix3x4_t* mat) {
		const auto studio_model = interfaces::model_info->get_studio_model(player->get_model());
		const auto r_ = uintptr_t(r);
		const auto tr_ = uintptr_t(tr);
		const auto scale_ = player->model_scale();
		const auto origin_ = uintptr_t(origin);
		const auto mat_ = uintptr_t(mat);
		const auto set_ = uintptr_t(studio_model->hitbox_set(player->hitbox_set()));
		const auto fn_ = patterns::trace_to_studio_csgo_hitgroups_priority;
		const auto hdr_ = uintptr_t(player->studio_hdr());
		auto rval = false;

		_asm {
			mov edx, r_
			push tr_
			push scale_
			push origin_
			push contents_mask
			push mat_
			push set_
			push hdr_
			mov eax, [fn_]
			call eax
			add esp, 0x1C
			mov rval, al
		}

		return rval;
	}

	autowall_t::penetration_t autowall_t::run(const vec3d& src, const vec3d& end, base_combat_weapon_t* weapon, lag_record_t* target, cs_player_t* const override_player, bool no_opt) {
		const auto player = override_player != nullptr ? override_player : globals->m_local;

		if (weapon == nullptr)
			weapon = globals->m_weapon;

		penetration_t result{};
		if (target != nullptr) {
			trace_filter_friendly_t filter(player);
			result = fire_bullet(weapon->get_cs_weapon_info(), src, end, &filter, target,
								 override_player ? false : weapon->item_definition_index() == e_weapon_type::weapon_zeusx27, no_opt);
		} else {
			static const auto filter_simple = *(uint32_t*)((uint32_t)(patterns::filter_simple.as<void*>()) + 0x3D);
			uint32_t filter_ptr[4] = { filter_simple, (uint32_t)player, 0, 0 };
			auto filter = (trace_filter_t*)(filter_ptr);

			result = fire_bullet(weapon->get_cs_weapon_info(), src, end, filter, target,
								 override_player ? false : weapon->item_definition_index() == e_weapon_type::weapon_zeusx27, no_opt);
		}

		if (result.m_damage < 1.f)
			result.m_did_hit = false;

		return result;
	}

	autowall_t::penetration_t autowall_t::fire_bullet(cs_weapon_info_t* data, vec3d src, const vec3d& pos, i_trace_filter* filter, lag_record_t* target, bool is_zeus, bool no_opt) {
		vec3d angles;
		math::vector_angles(pos - src, angles);

		vec3d direction;
		math::angle_vectors(angles, direction);

		direction.normalize_in_place();

		const vec3d start = src;
		const auto max_length = no_opt ? data->m_range : std::min<float>(data->m_range, (start - pos).length());
		auto penetrate_count = 4;
		auto length = 0.f, damage = (float)data->m_damage, real_damage = damage;

		game_trace_t enter_trace{};
		penetration_t result{};
		result.m_direction = direction;

		while (damage > 0.f) {
			const auto length_remaining = max_length - length;
			auto end = src + direction * length_remaining;

			if (enter_trace.m_entity)
				((trace_filter_t*)filter)->m_skip = enter_trace.m_entity;

			ray_t r{};
			r.init(src, end);
			interfaces::traces->trace_ray(r, MASK_SHOT, filter, &enter_trace);

			result.m_end = enter_trace.m_end;
			if (target != nullptr && (!no_opt || !result.m_did_hit)) {
				const auto dist = (start - target->m_origin).length();
				const auto behind = (start - enter_trace.m_end).length() > dist;

				if (behind) {
					result.m_did_hit = true;
					damage *= std::powf(data->m_range_modifier, (dist - length) * .002f);
					real_damage = damage;
					if (!no_opt)
						break;
				}
			}

			length += enter_trace.m_fraction * length_remaining;
			if (!result.m_did_hit || enter_trace.m_fraction == 1.f)
				damage *= std::powf(data->m_range_modifier, length * .002f);

			if (enter_trace.m_fraction == 1.f) {
				if (!result.m_did_hit)
					real_damage = damage;

				result.m_did_hit = true;
				break;
			}

			result.m_impacts[result.m_impact_count++] = enter_trace.m_end;

			const auto enter_surface = interfaces::phys_surface->get_surface_data(enter_trace.m_surface.m_surface_props);
			if ((length > 3000.f && data->m_penetration > 0.f) || (!enter_surface || enter_surface->m_game.m_penetration_modifier < .1f))
				penetrate_count = 0;

			if (!handle_bullet_penetration(data, enter_trace, src, direction, penetrate_count, damage))
				break;
		}

		if (!result.m_did_hit || target == nullptr) {
			result.m_damage = real_damage;
			result.m_impacts[result.m_impact_count++] = enter_trace.m_end;
			return result;
		}

		matrix3x4_t* mat_[max_bones]{};
		result.m_damage = 0.f;
		const vec3d initial_end = result.m_end;

		const auto test = [&](const matrix3x4_t* mat, bool real = false) {
			for (auto i = 0; i < max_bones; i++)
				mat_[i] = const_cast<matrix3x4_t*>(&mat[i]);

			ray_t r{};
			r.init(start, initial_end);
			game_trace_t tr{};
			tr.m_contents = enter_trace.m_contents;
			tr.m_start = r.m_start + r.m_start_offset;
			tr.m_end = tr.m_start + r.m_delta;

			const auto ret = trace_to_studio_csgo_hitgroups_priority(target->m_player, MASK_SHOT_PLAYER, &target->m_origin, &tr, &r, (matrix3x4_t*)mat_);
			if (!ret) {
				result.m_min_damage = 0.f;
				return ret;
			}

			const auto new_damage = is_zeus ? real_damage : scale_damage(target->m_player, real_damage, data->m_armor_ratio, tr.m_hitgroup);

			if (new_damage < result.m_min_damage)
				result.m_min_damage = new_damage;

			result.m_impacts[result.m_impact_count++] = tr.m_end;

			if (real) {
				result.m_end = tr.m_end;
				result.m_hitbox = e_hitbox(tr.m_hitbox);
				result.m_hitgroup = tr.m_hitgroup;
				result.m_damage = new_damage;
			}

			return ret;
		};

		result.m_min_damage = FLT_MAX;
		test(target->m_bones, true);

		if (target->m_aimbot.m_primary) {
			if (test(target->m_unresolved_bones))
				target->m_aimbot.m_safety |= 1;

			if (test(target->m_inversed_bones))
				target->m_aimbot.m_safety |= 2;
		} else {
			if (target->m_aimbot.m_safety & 1)
				test(target->m_unresolved_bones);

			if (target->m_aimbot.m_safety & 2)
				test(target->m_inversed_bones);
		}

		if (result.m_damage <= 1.f)
			result.m_did_hit = false;

		return result;
	}
} // namespace hvh