#pragma once
#include "../../game/engine_prediction.hpp"
#include "../../globals.hpp"
#include "../../interfaces.hpp"
#include "exploits.hpp"
#include "hvh.hpp"

namespace hvh {
	using namespace sdk;

	__forceinline float lerp_time() {
		return std::max<float>(CVAR_FLOAT("cl_interp"), CVAR_FLOAT("cl_interp_ratio") / CVAR_FLOAT("cl_updaterate"));
	}

	__forceinline bool is_able_to_shoot(int tickbase) {
		if (globals->m_local == nullptr || !globals->m_local->is_alive())
			return false;

		if (interfaces::game_rules->is_freeze_time())
			return false;

		if (globals->m_cmd->m_weapon_select != 0)
			return false;

		if (globals->m_local->flags() & 0x40)
			return false;

		if (globals->m_local->wait_for_noattack())
			return false;

		if (globals->m_local->defusing())
			return false;

		if (globals->m_weapon->in_reload())
			return false;

		if (globals->m_local->player_state() > 0)
			return false;

		if (globals->m_weapon->ammo() <= 0 && !globals->m_weapon->is_knife() && globals->m_weapon->item_definition_index() != e_weapon_type::weapon_zeusx27)
			return false;

		if (tickbase == 0) // no argument passed
			tickbase = globals->m_tickbase;

		const auto curtime = TICKS_TO_TIME(tickbase);
		const auto weapon_id = globals->m_weapon->item_definition_index();

		if ((weapon_id == e_weapon_type::weapon_glock || weapon_id == e_weapon_type::weapon_famas) && globals->m_weapon->burst_shots_remaining() > 0) {
			// new burst shot is coming out.
			if (curtime /* - interfaces::global_vars->m_interval_per_tick*/ >= globals->m_weapon->next_burst_shot())
				return true;
		}

		if (globals->m_local->next_attack() > curtime || globals->m_weapon->next_primary_attack() > curtime)
			return false;

		return true;
	}

	__forceinline bool is_shooting(user_cmd_t* cmd) {
		if (globals->m_weapon->is_knife())
			return is_able_to_shoot() && cmd->m_buttons.has(in_attack | in_attack2);
		else if (globals->m_weapon->is_projectile())
			return ((base_grenade_t*)globals->m_weapon)->throw_time() > 0.f;

		return is_able_to_shoot() && cmd->m_buttons.has(in_attack);
	}

	__forceinline e_hitgroup hitbox_to_hitgroup(e_hitbox hitbox) {
		switch (hitbox) {
			case e_hitbox::head: return e_hitgroup::head;
			case e_hitbox::neck: return e_hitgroup::neck;
			case e_hitbox::pelvis:
			case e_hitbox::stomach:
				return e_hitgroup::stomach;
			case e_hitbox::lower_chest:
			case e_hitbox::chest:
			case e_hitbox::upper_chest:
				return e_hitgroup::chest;
			case e_hitbox::right_shin:
			case e_hitbox::right_thigh:
			case e_hitbox::right_foot:
				return e_hitgroup::rightleg;
			case e_hitbox::left_thigh:
			case e_hitbox::left_shin:
			case e_hitbox::left_foot:
				return e_hitgroup::leftleg;
			case e_hitbox::right_upper_arm:
			case e_hitbox::right_forearm:
			case e_hitbox::right_hand:
				return interfaces::global_vars->m_tickcount % 2 ? e_hitgroup::rightarm : e_hitgroup::chest;
			case e_hitbox::left_hand:
			case e_hitbox::left_upper_arm:
			case e_hitbox::left_forearm:
				return interfaces::global_vars->m_tickcount % 2 ? e_hitgroup::leftarm : e_hitgroup::chest;
			default: return e_hitgroup::generic;
		}
	}

	__forceinline const std::vector<e_hitbox> get_hitboxes_in_hitgroup(e_hitbox hitbox) {
		switch (hitbox) {
			case e_hitbox::head:
				return { e_hitbox::head };
			case e_hitbox::pelvis:
			case e_hitbox::stomach:
				return { e_hitbox::stomach, e_hitbox::pelvis };
			case e_hitbox::lower_chest:
			case e_hitbox::chest:
			case e_hitbox::upper_chest:
				return { e_hitbox::chest, e_hitbox::lower_chest, e_hitbox::upper_chest };
			case e_hitbox::right_shin:
			case e_hitbox::right_thigh:
			case e_hitbox::right_foot:
				return { e_hitbox::right_shin, e_hitbox::right_thigh, e_hitbox::right_foot };
			case e_hitbox::left_shin:
			case e_hitbox::left_thigh:
			case e_hitbox::left_foot:
				return { e_hitbox::left_shin, e_hitbox::left_thigh, e_hitbox::left_foot };
			case e_hitbox::right_upper_arm:
			case e_hitbox::right_forearm:
			case e_hitbox::right_hand:
				return { e_hitbox::right_upper_arm, e_hitbox::right_forearm, e_hitbox::right_hand,
						 e_hitbox::lower_chest, e_hitbox::chest, e_hitbox::upper_chest };
			case e_hitbox::left_hand:
			case e_hitbox::left_upper_arm:
			case e_hitbox::left_forearm:
				return { e_hitbox::left_upper_arm, e_hitbox::left_forearm, e_hitbox::left_hand,
						 e_hitbox::lower_chest, e_hitbox::chest, e_hitbox::upper_chest };
			default:
				return { e_hitbox::head, e_hitbox::stomach, e_hitbox::pelvis, e_hitbox::chest, e_hitbox::lower_chest, e_hitbox::upper_chest };
		}
	}

	__forceinline void extrapolate_position(cs_player_t* player, vec3d& origin, vec3d& velocity, flags_t& flags, bool was_onground) {
		if (!was_onground)
			velocity.z -= TICKS_TO_TIME(CVAR_FLOAT("sv_gravity"));
		else if (flags.has(fl_onground) && !was_onground)
			velocity.z = CVAR_FLOAT("sv_jump_impulse");

		vec3d src = origin;
		auto end = src + velocity * TICKS_TO_TIME(1);

		ray_t r{};
		r.init(src, end, player->mins(), player->maxs());

		trace_t t;
		trace_filter_t filter;
		filter.m_skip = player;

		interfaces::traces->trace_ray(r, MASK_PLAYERSOLID, &filter, &t);

		if (t.m_fraction != 1.f) {
			for (int i = 0; i < 2; ++i) {
				velocity -= t.m_plane.m_normal * velocity.dot(t.m_plane.m_normal);

				const auto dot = velocity.dot(t.m_plane.m_normal);
				if (dot < 0.f)
					velocity -= vec3d{ dot * t.m_plane.m_normal.x,
									   dot * t.m_plane.m_normal.y, dot * t.m_plane.m_normal.z };

				end = t.m_end + velocity * TICKS_TO_TIME(1.f - t.m_fraction);

				r.init(t.m_end, end, player->mins(), player->maxs());
				interfaces::traces->trace_ray(r, MASK_PLAYERSOLID, &filter, &t);

				if (t.m_fraction == 1.f)
					break;
			}
		}

		src = end = t.m_end;
		origin = end;
		end.z -= 2.f;

		r.init(src, end, player->mins(), player->maxs());
		interfaces::traces->trace_ray(r, MASK_PLAYERSOLID, &filter, &t);

		flags.remove(fl_onground);

		if (t.did_hit() && t.m_plane.m_normal.z > .7f)
			flags.add(fl_onground);
	}

	__forceinline vec3d predict_player_origin(cs_player_t* player, int ticks, bool debug) {
		vec3d simulated_velocity = player->velocity();
		vec3d simulated_origin = player->origin();
		vec3d previous_origin = simulated_origin;
		flags_t flags = player->flags();
		bool on_ground = flags.has(fl_onground);

		for (int i = 0; i < ticks; ++i) {
			extrapolate_position(player, simulated_origin, simulated_velocity, flags, on_ground);

			if (debug)
				interfaces::debug_overlay->add_line_overlay(previous_origin, simulated_origin,
															255, 255 / ticks, 255 - 255 % ticks, false, interfaces::global_vars->m_interval_per_tick * 2.f);

			previous_origin = simulated_origin;
		}

		return simulated_origin;
	}

	__forceinline vec3d predict_local_player_origin(const vec3d& origin, const vec3d& velocity, const vec3d& old_velocity, int ticks, bool debug) {
		vec3d derivative_velocity = velocity - old_velocity;
		derivative_velocity.z = 0.f;

		bool quick_stop = false;

		if (aimbot->m_settings.has_value())
			quick_stop = aimbot->m_settings->quick_stop;

		if ((aimbot->m_should_predictive_stop || aimbot->m_should_work) && quick_stop)
			derivative_velocity = vec3d{ 0.f, 0.f, 0.f };

		vec3d simulated_velocity = velocity;
		vec3d simulated_origin = origin;
		vec3d previous_origin = simulated_origin;
		flags_t flags = globals->m_local->flags();
		bool on_ground = flags.has(fl_onground);

		for (int i = 0; i < ticks; ++i) {
			extrapolate_position(globals->m_local, simulated_origin, simulated_velocity, flags, on_ground);

			if (on_ground)
				simulated_velocity += derivative_velocity;

			if (debug)
				interfaces::debug_overlay->add_line_overlay(previous_origin, simulated_origin,
															255, 255 / ticks, 255 - 255 % ticks, false, interfaces::global_vars->m_interval_per_tick * 2.f);

			previous_origin = simulated_origin;
		}

		return simulated_origin;
	}

	STFI void set_collision_bounds(void* collideable, vec3d* mins, vec3d* maxs) {
		static auto fn = patterns::set_collision_bounds.as<void(__thiscall*)(void*, vec3d*, vec3d*)>();
		fn(collideable, mins, maxs);
	}

	__forceinline void backup_player(cs_player_t* player) {
		auto& record = aimbot->m_backup_records[player->index()];
		record.m_origin = player->origin();
		record.m_eye_angles = player->eye_angles();
		record.m_mins = player->mins();
		record.m_maxs = player->maxs();
		record.m_flags = player->flags();
		record.m_collision_change_height = player->collision_change_height();
		record.m_collision_change_time = player->collision_change_time();

		player->store_bone_cache(record.m_bones);
	}

	__forceinline void restore_player(cs_player_t* player) {
		auto& record = aimbot->m_backup_records[player->index()];
		player->set_abs_origin(record.m_origin);
		player->origin() = record.m_origin;
		player->eye_angles() = record.m_eye_angles;
		player->flags() = record.m_flags;

		player->mins() = record.m_mins;
		player->maxs() = record.m_maxs;

		/*auto collideable = player->collideable();
		set_collision_bounds(collideable, &record.m_mins, &record.m_maxs);*/

		player->collision_change_height() = record.m_collision_change_height;
		player->collision_change_time() = record.m_collision_change_time;

		player->set_bone_cache(record.m_bones);
		//player->invalidate_bone_cache(interfaces::global_vars->m_framecount);
	}

	__forceinline float get_standing_accuracy() {
		auto weapon_info = globals->m_weapon->get_cs_weapon_info();
		if (weapon_info == nullptr)
			return -1.f;

		return globals->m_weapon->zoom_level() > 0 ? weapon_info->m_max_speed_alt : weapon_info->m_max_speed;
	}

	__forceinline int get_ticks_to_standing_accuracy() {
		return (int)((globals->m_local->velocity().length2d() / get_standing_accuracy()) * 5.f); // unpredicted velocity?
	}

	__forceinline int get_ticks_to_stop() {
		const auto weapon_info = globals->m_weapon->get_cs_weapon_info();
		if (weapon_info == nullptr)
			return -1;

		return (int)(globals->m_local->velocity().length2d() / (globals->m_zoom_level > 0 ? weapon_info->m_max_speed_alt : weapon_info->m_max_speed) * 8.f);
	}

	__forceinline int get_ticks_to_shoot() {
		return std::max<int>(TIME_TO_TICKS(globals->m_weapon->next_primary_attack()) - globals->m_tickbase, 0);
	}

	__forceinline bool is_between_shots() {
		if (globals->m_weapon->ammo() <= 0 || globals->m_weapon->is_knife())
			return false;

		if (interfaces::game_rules->is_freeze_time())
			return false;

		if (globals->m_cmd->m_weapon_select != 0)
			return false;

		if (globals->m_local->flags() & 0x40)
			return false;

		if (globals->m_local->wait_for_noattack())
			return false;

		if (globals->m_local->defusing())
			return false;

		if (globals->m_weapon->in_reload())
			return false;

		if (globals->m_local->player_state() > 0)
			return false;

		const auto curtime = TICKS_TO_TIME(globals->m_tickbase);
		if (globals->m_local->next_attack() > curtime || globals->m_weapon->next_primary_attack() > curtime)
			return true;

		return false;
	}

	__forceinline bool auto_revolver(user_cmd_t* cmd) {
		if (globals->m_weapon->item_definition_index() != e_weapon_type::weapon_revolver)
			return false;

		if (exploits->m_in_shift) {
			cmd->m_buttons.remove(in_attack);
			return false;
		}

		if (globals->m_weapon->ammo() <= 0)
			return false;

		if (std::abs(exploits->m_last_shift_tick - interfaces::client_state->get_current_tick()) < MAX_FAKELAG)
			return false;

		const auto curtime = TICKS_TO_TIME(globals->m_tickbase);

		if (globals->m_local->next_attack() > curtime)
			return false;

		static int last_checked = 0;
		static float last_spawntime = 0.f;
		static int ticks_cocked = 0;
		static int ticks_strip = 0;

		const int max_ticks = TIME_TO_TICKS(0.25f) - 1;
		const int tickbase = TIME_TO_TICKS(curtime);

		if (globals->m_local->spawn_time() != last_spawntime) {
			ticks_cocked = tickbase;
			ticks_strip = tickbase - max_ticks - 1;
			last_spawntime = globals->m_local->spawn_time();
		}

		if (globals->m_weapon->next_primary_attack() > curtime) {
			cmd->m_buttons.remove(in_attack);
			return false;
		}

		if (last_checked == tickbase)
			return false;

		last_checked = tickbase;
		bool revolver_fire = false;

		if (tickbase - ticks_strip > 2 && tickbase - ticks_strip < 14)
			revolver_fire = true;

		if (cmd->m_buttons.has(in_attack) && revolver_fire)
			return true;

		cmd->m_buttons.add(in_attack);

		if (globals->m_weapon->next_secondary_attack() >= curtime)
			cmd->m_buttons.add(in_attack2);

		if (tickbase - ticks_cocked > max_ticks * 2 + 1) {
			ticks_cocked = tickbase;
			ticks_strip = tickbase - max_ticks - 1;
		}

		const bool limit = tickbase - ticks_cocked >= max_ticks;
		const bool after_strip = tickbase - ticks_strip <= max_ticks;

		if (limit || after_strip) {
			ticks_cocked = tickbase;
			cmd->m_buttons.remove(in_attack);

			if (limit)
				ticks_strip = tickbase;
		}

		return revolver_fire;
	}
} // namespace hvh