#include "../features/hvh/exploits.hpp"
#include "../features/hvh/hvh.hpp"
#include "../features/movement.hpp"
#include "../globals.hpp"
#include "../interfaces.hpp"
#include "custom_animations.hpp"
#include "dormant.hpp"
#include "engine_prediction.hpp"
#include "override_entity_list.hpp"
#include "players.hpp"

using namespace sdk;
using namespace hvh;

void players_t::on_create_move() {
	if (!CVAR_BOOL("cl_lagcompensation"))
		return;

	const auto nci = interfaces::engine->get_net_channel();
	if (nci == nullptr)
		return;

	const auto rtt = nci->get_latency(flow_outgoing) + nci->get_latency(flow_incoming);
	const auto lerp_time = hvh::lerp_time();
	const auto is_revolver = globals->m_weapon != nullptr && globals->m_weapon->item_definition_index() == e_weapon_type::weapon_revolver;
	const auto doubletap = hvh::exploits->m_charged && hvh::exploits->m_type != hvh::exploits_t::e_type::none;
	const auto tickbase_error = doubletap && hvh::exploits->m_last_skip_shift;
	const auto tickbase = globals->m_local->tickbase() - (tickbase_error ? (CVAR_INT("sv_maxusrcmdprocessticks") - 2) : 0);

	const auto adjusted_time = TICKS_TO_TIME(tickbase - 1);

	const auto dead_time = (int)(TICKS_TO_TIME(interfaces::client_state->m_last_server_tick) + rtt - 0.2f);
	const auto future_tick = interfaces::client_state->m_last_server_tick + TIME_TO_TICKS(rtt) + 8;
	const auto correct = std::clamp(lerp_time + rtt, 0.f, 0.2f);

	for (auto& [player, entry]: entities->m_players) {
		if (player == nullptr || player->is_teammate())
			continue;

		if (!entry.m_initialized)
			continue;

		const auto teammate = player->is_teammate();
		const vec3d player_origin = player->origin();
		for (ptrdiff_t i = entry.m_records.size() - 1; i >= 0; --i) {
			auto& record = entry.m_records[i];

			if (teammate) {
				record.m_valid = record.m_valid_check = record.m_valid_visual = true;
				continue;
			}

			if (record.m_origin.dist_sqr(player_origin) > 4096.f) {
				record.m_valid_check = false;
				record.m_valid_visual = false;
				continue;
			}

			if (TIME_TO_TICKS(record.m_simulation_time + lerp_time) > future_tick) {
				record.m_valid_check = false;
				record.m_valid_visual = false;
				continue;
			}

			const auto dt_start = correct - (adjusted_time - record.m_simulation_time - TICKS_TO_TIME(1));
			const auto dt_end = correct - (adjusted_time - record.m_simulation_time + TICKS_TO_TIME(1));
			record.m_valid_visual = std::abs(dt_start) < 0.2f && std::abs(dt_end) < 0.2f;

			if (dead_time >= record.m_simulation_time) {
				record.m_valid_check = false;
				continue;
			}

			record.m_valid_check = record.m_valid_visual;
		}
	}
}

void lag_record_t::store_data(player_entry_t& entry) {
	m_player = entry.m_player;
	m_index = entry.m_player->index();
	m_simulation_time = m_player->simtime();
	m_old_simulation_time = m_player->old_simtime();
	m_player->store_poses(m_poses);
	m_player->store_layers(m_layers);

	m_velocity = m_player->velocity();
	m_eye_angles = m_player->eye_angles();
	m_flags = m_player->flags();
	m_eflags = m_player->eflags();
	m_duck_amount = m_player->duck_amount();
	m_lby = m_player->lby();
	m_thirdperson_recoil = m_player->thirdperson_recoil();
	m_origin = m_player->origin();
	m_abs_angle = m_player->get_abs_angles();

	m_mins = m_player->mins();
	m_maxs = m_player->maxs();

	m_ground_entity = m_player->ground_entity_raw();

	const auto time_delta = m_simulation_time - m_old_simulation_time;
	m_lag = TIME_TO_TICKS(std::clamp(time_delta, interfaces::global_vars->m_interval_per_tick, TICKS_TO_TIME(CVAR_INT("sv_maxusrcmdprocessticks"))));

	m_shot = false;

	m_entry = &entry;
	m_animation_start = 0.f;
	m_tick_received = interfaces::client_state->m_clock_drift_manager.m_server_tick;
	m_lerp_time = 0.f;
	m_last_received_time = 0.f;
	m_received_time = interfaces::global_vars->m_realtime;
	m_received_curtime = interfaces::global_vars->m_curtime;

	m_previous = nullptr;
}

void lag_record_t::restore() {
	m_player->velocity() = m_velocity;
	m_player->flags() = m_flags;
	m_player->eflags() = m_eflags;
	m_player->duck_amount() = m_duck_amount;
	m_player->apply_layers(m_layers);
	m_player->apply_poses(m_poses);
	m_player->lby() = m_lby;
	m_player->thirdperson_recoil() = m_thirdperson_recoil;
	m_player->set_abs_origin(m_origin);
}

//STFI void set_collision_bounds(void* collideable, vec3d* mins, vec3d* maxs) {
//	static auto fn = patterns::set_collision_bounds.as<void(__thiscall*)(void*, vec3d*, vec3d*)>();
//	fn(collideable, mins, maxs);
//}

void lag_record_t::apply(matrix3x4_t* bones) {
	m_player->origin() = m_origin;
	m_player->set_abs_origin(m_origin);

	m_player->mins() = m_mins;
	m_player->maxs() = m_maxs;
	//clamp_bones();

	//clamp_bones_in_bbox(m_player, bones, 0x7FF00, TICKS_TO_TIME(globals->m_local->tickbase()), m_player->eye_angles());

	/*auto collideable = m_player->collideable();
	set_collision_bounds(collideable, &this->m_mins, &this->m_maxs);*/

	m_player->set_bone_cache(bones);
	//m_player->invalidate_bone_cache(interfaces::global_vars->m_framecount);
}

bool lag_record_t::is_valid() const {
	return !is_nulled() && m_valid && m_valid_check;
}

void lag_record_t::update_animations() {
	auto state = m_player->animstate();
	if (state == nullptr)
		return;

	{
		SET_AND_RESTORE(interfaces::global_vars->m_curtime, m_player->simtime());
		SET_AND_RESTORE(interfaces::global_vars->m_frametime, interfaces::global_vars->m_interval_per_tick);

		if (state->m_last_update_frame == interfaces::global_vars->m_framecount)
			--state->m_last_update_frame;

		auto weapon = m_player->active_weapon();
		if (weapon)
			state->m_weapon_last = weapon;

		for (auto i = 0u; i < XOR32(13); i++) {
			auto layer = &m_player->animation_layers()[i];
			layer->m_owner = m_player;
			layer->m_studio_hdr = m_player->studio_hdr();

			if (i == XOR32(12))
				layer->m_weight = m_previous != nullptr ? m_previous->m_layers[XOR32(12)].m_weight : 0.f;
		}

		bool old_update = m_player->clientside_anims();

		players->is_updating(m_player) = m_player->clientside_anims() = true;
		m_player->update_clientside_animations();
		players->is_updating(m_player) = false;
		m_player->clientside_anims() = old_update;
	}

	m_player->invalidate_physics_recursive(0xA);
}

void lag_record_t::update_animations(int as) {
	auto animstate = m_player->animstate();

	if (animstate == nullptr)
		return;

	switch (as) {
		case as_left:
		case as_right:
			animstate->m_goal_feet_yaw = math::normalize_yaw(m_eye_angles.y + m_player->max_desync() * as);
			break;
		case as_zero:
			if (m_may_has_fake)
				animstate->m_goal_feet_yaw = animstate->m_eye_yaw;
			break;
		case as_main:
			if (m_may_has_fake) {
				const auto original_angle = m_resolver.m_side == 0 && m_resolver.m_desync == 0.f;
				if (!original_angle)
					animstate->m_goal_feet_yaw = math::normalize_yaw(m_eye_angles.y + (m_player->max_desync() * m_resolver.m_side));
			}
			break;
	}

	return update_animations();
}

//STFI void clamp_bones_in_bbox(cs_player_t* player, matrix3x4_t* bones, uint32_t bone_mask) {
//	patterns::clamp_bones_in_bbox.as<void(__thiscall*)(cs_player_t*, matrix3x4_t*, uint32_t)>()(player, bones, bone_mask);
//}

void lag_record_t::setup_bones(matrix3x4_t* bones, uint32_t bone_mask, lag_record_t* previous, bool main_matrix) {
	int backup_effects = m_player->effects();

	auto& ent_flags = *(int*)((uintptr_t)m_player + 0x68);
	auto& ik_ctx_ptr = *(int*)((uintptr_t)m_player + 0x2670);

	int old_ent_flags = ent_flags;
	int old_ik_ctx_ptr = ik_ctx_ptr;

	m_player->invalidate_bone_cache(interfaces::global_vars->m_framecount);
	const auto curtime_backup = interfaces::global_vars->m_curtime;

	SET_AND_RESTORE(interfaces::global_vars->m_curtime, m_simulation_time);
	SET_AND_RESTORE(interfaces::global_vars->m_frametime, interfaces::global_vars->m_interval_per_tick);

	m_player->effects() |= 8;
	{
		ent_flags |= 2; // ENTCLIENTFLAG_DONTUSEIK
		ik_ctx_ptr = 0;

		vec3d old_abs_origin = m_player->vec_abs_origin();
		m_player->set_abs_origin(m_player->origin());

		players->is_setuping(m_player) = true;
		m_player->setup_bones(bones, max_bones, 0x7FF00, m_simulation_time);
		players->is_setuping(m_player) = false;

		if (main_matrix)
			utils::memcpy_sse(m_visual_bones, bones, sizeof(matrix3x4_t) * sdk::max_bones);

		//if (main_matrix && previous != nullptr && !m_player->is_teammate()) {
		//	vec3d eye_angles_backup = m_player->eye_angles();
		//	if ((m_resolver.m_jitter.m_should_fix || std::abs(math::normalize_yaw(previous->m_eye_angles.y - this->m_eye_angles.y)) > 45.f) && !previous->m_shot && !this->m_shot)
		//		m_player->eye_angles() = previous->m_eye_angles;

		//	clamp_bones_in_bbox(m_player, bones, 0x7FF00, interfaces::global_vars->m_curtime, m_player->eye_angles());

		//	m_player->eye_angles() = eye_angles_backup;
		//} else
		//	clamp_bones_in_bbox(m_player, bones, 0x7FF00, interfaces::global_vars->m_curtime, m_player->eye_angles());

		if (main_matrix) {
			clamp_bones_in_bbox(m_player, m_visual_bones, 0x7FF00, interfaces::global_vars->m_curtime, m_player->eye_angles());
		}

		m_player->set_abs_origin(old_abs_origin);
	}
	m_player->effects() = backup_effects;

	ent_flags = old_ent_flags;
	ik_ctx_ptr = old_ik_ctx_ptr;
}

STFI void build_animation(lag_record_t* record, lag_record_t* previous, int as) {
	auto player = record->m_player;
	auto animstate = player->animstate();
	if (animstate == nullptr)
		return;

	if (previous != nullptr && !record->m_came_from_dormant && !previous->m_came_from_dormant) {
		animstate->m_primary_cycle = record->m_layers[ANIMATION_LAYER_MOVEMENT_MOVE].m_cycle;
		animstate->m_move_weight = record->m_layers[ANIMATION_LAYER_MOVEMENT_MOVE].m_weight;
		// animstate->m_acceleration_weight = record->m_layers[12].m_weight;
	} else {
		auto& layers = record->m_layers;

		// that thing fixes bot on 'spawn tick'
		// so his 'goal feet yaw' will be set to zero due to ResetAnimationState called when 'spawn time' updated
		// idk where it pasted from, so it works
		animstate->m_last_update_time = (record->m_simulation_time - interfaces::global_vars->m_interval_per_tick);

		// this happens only when first record arrived or when enemy is out from dormant
		// so we should set latest data as soon as possible
		animstate->m_primary_cycle = layers[ANIMATION_LAYER_MOVEMENT_MOVE].m_cycle;
		animstate->m_move_weight = layers[ANIMATION_LAYER_MOVEMENT_MOVE].m_weight;

		auto last_update_time = animstate->m_last_update_time;
		auto layer_jump = &record->m_layers[ANIMATION_LAYER_MOVEMENT_JUMP_OR_FALL];
		if (player->get_sequence_activity(layer_jump->m_sequence) == act_csgo_jump) {
			auto duration_in_air = record->m_simulation_time - layer_jump->m_cycle / layer_jump->m_playback_rate;
			if (duration_in_air > last_update_time) {
				animstate->m_on_ground = false;
				player->poses()[6] = 0.f;
				animstate->m_duration_in_air = 0.f;
				animstate->m_last_update_time = duration_in_air;
			}
		}
	}

	if (previous == nullptr || record->m_came_from_dormant || record->m_lag <= 1) {
		if (previous != nullptr && !player->flags().has(fl_onground)) {
			auto layer_jump = &record->m_layers[ANIMATION_LAYER_MOVEMENT_JUMP_OR_FALL];
			auto old_layer_jump = &record->m_layers[ANIMATION_LAYER_MOVEMENT_JUMP_OR_FALL];

			if (layer_jump->m_weight > 0.f && layer_jump->m_cycle > 0.f) {
				if (player->get_sequence_activity(layer_jump->m_sequence) == act_csgo_jump) {
					if (layer_jump->m_cycle != old_layer_jump->m_cycle || layer_jump->m_sequence != old_layer_jump->m_sequence && old_layer_jump->m_cycle > layer_jump->m_cycle) {
						player->poses()[6] = 0.f;
						animstate->m_duration_in_air = layer_jump->m_cycle / layer_jump->m_playback_rate;
					}
				}
			}
		}

		player->velocity() = record->m_velocity;
		player->lby() = record->m_lby;

		record->update_animations(as);
	} else {
		player->lby() = previous->m_lby;

		auto choke_float = static_cast<float>(record->m_lag);

		auto simulation_time_tick = TIME_TO_TICKS(record->m_simulation_time);
		auto prev_simulation_time = simulation_time_tick - record->m_lag;

		int land_time{}, jump_time{};

		if (previous != nullptr) {
			// we have information that he is on ground now
			// now let's guess, he is landing?
			if (player->flags().has(fl_onground)) {
				auto layer_land = &record->m_layers[ANIMATION_LAYER_MOVEMENT_LAND_OR_CLIMB];
				auto old_layer_land = &previous->m_layers[ANIMATION_LAYER_MOVEMENT_LAND_OR_CLIMB];

				// yes, he possibly landing now
				// because of landing server info was sent to server
				// now calculate start time where he was not landing on choke cycle
				if (!previous->m_flags.has(fl_onground) && layer_land->m_weight != 0.f && old_layer_land->m_weight == 0.f) {
					auto sequence_activity = player->get_sequence_activity(layer_land->m_sequence);
					if (sequence_activity == act_csgo_land_light || sequence_activity == act_csgo_land_heavy)
						land_time = TIME_TO_TICKS(record->m_simulation_time - (layer_land->m_cycle / layer_land->m_playback_rate)) + 1;
				}
			} else {
				auto layer_jump = &record->m_layers[ANIMATION_LAYER_MOVEMENT_JUMP_OR_FALL];
				auto old_layer_jump = &previous->m_layers[ANIMATION_LAYER_MOVEMENT_JUMP_OR_FALL];

				// another case - he is jumping now
				// now calculate start time where he was not jumping on choke cycle
				if (layer_jump->m_cycle != old_layer_jump->m_cycle || layer_jump->m_sequence != old_layer_jump->m_sequence && layer_jump->m_cycle < old_layer_jump->m_cycle) {
					auto sequence_activity = player->get_sequence_activity(layer_jump->m_sequence);
					if (sequence_activity == act_csgo_jump)
						jump_time = TIME_TO_TICKS(record->m_simulation_time - (layer_jump->m_cycle / layer_jump->m_playback_rate)) + 1;
				}
			}
		}

		for (int i = 0; i <= record->m_lag; ++i) {
			auto current_command_tick = prev_simulation_time + i;
			auto current_command_time = TICKS_TO_TIME(current_command_tick);

			if (current_command_tick == land_time) {
				player->flags().add(fl_onground);

				auto record_layer_land = &record->m_layers[ANIMATION_LAYER_MOVEMENT_LAND_OR_CLIMB];
				auto layer_land = &player->animation_layers()[ANIMATION_LAYER_MOVEMENT_LAND_OR_CLIMB];
				layer_land->m_cycle = 0.f;
				layer_land->m_playback_rate = record_layer_land->m_playback_rate;
			}

			if (current_command_tick == jump_time - 1)
				player->flags().add(fl_onground);

			if (current_command_tick == jump_time) {
				player->flags().remove(fl_onground);

				auto record_layer_jump = &record->m_layers[ANIMATION_LAYER_MOVEMENT_JUMP_OR_FALL];
				auto layer_jump = &player->animation_layers()[ANIMATION_LAYER_MOVEMENT_JUMP_OR_FALL];
				layer_jump->m_cycle = 0.f;
				layer_jump->m_playback_rate = record_layer_jump->m_playback_rate;
			}

			const auto new_simtime = record->m_old_simulation_time + TICKS_TO_TIME(i);
			const auto sim_tick = TIME_TO_TICKS(new_simtime);
			const auto lerp = (float)(i) / (float)record->m_lag;

			//player->eye_angles() = math::clamp_angles(math::lerp(previous->m_eye_angles, record->m_eye_angles, lerp));
			player->duck_amount() = math::lerp(previous->m_duck_amount, record->m_duck_amount, lerp);

			if (record->m_shot) {
				if (record->m_last_shot_time <= new_simtime) {
					player->lby() = record->m_lby;
					player->thirdperson_recoil() = record->m_thirdperson_recoil;
				}
			}

			player->abs_velocity() = player->velocity() = record->m_velocity;

			float simtime_backup = player->simtime();
			player->simtime() = new_simtime;
			record->update_animations(as);
			player->simtime() = simtime_backup;
		}
	}

	if (previous != nullptr) {
		record->m_layers[12].m_weight = previous->m_layers[12].m_weight;
		record->m_layers[12].m_cycle = previous->m_layers[12].m_cycle;
	} else {
		record->m_layers[12].m_weight = 0.f;
		record->m_layers[12].m_cycle = 0.f;
	}

	switch (as) {
		case as_right:
			player->store_layers(record->m_right_layers);
			record->setup_bones(record->m_right_bones, 0x7FF00, previous);
			player->apply_layers(record->m_layers);
			break;
		case as_left:
			player->store_layers(record->m_left_layers);
			record->setup_bones(record->m_left_bones, 0x7FF00, previous);
			player->apply_layers(record->m_layers);
			break;
		case as_zero:
			player->store_layers(record->m_zero_layers);
			record->setup_bones(record->m_zero_bones, 0x7FF00, previous);
			player->apply_layers(record->m_layers);
			break;
		case as_main:
			player->apply_layers(record->m_layers);
			record->setup_bones(record->m_main_bones, 0x7FF00, previous, true);
			break;
	}
}

void player_entry_t::update_animations() {
	auto record = &m_records.back();
	record->m_on_ground = record->m_flags.has(fl_onground);

	auto weapon = m_player->active_weapon();
	if (m_previous != nullptr && !m_previous->m_came_from_dormant) {
		if (weapon != nullptr) {
			record->m_last_shot_time = weapon->last_shot_time();
			record->m_shot = record->m_simulation_time >= record->m_last_shot_time && record->m_last_shot_time > record->m_old_simulation_time;
		}
	}

	animation_state_t state{};
	std::memcpy(&state, m_player->animstate(), sizeof(animation_state_t));

	const auto index = m_player->index();
	const auto teammate = m_player->is_teammate();
	auto height_before = m_player->collision_change_height();

	if (!teammate) {
		auto weapon = m_player->active_weapon();
		if (weapon) {
			auto world_weapon = (cs_player_t*)weapon->world_model().get();
			if (world_weapon != nullptr) {
				for (int i = 0; i < 13; i++) {
					auto layer = &m_player->animation_layers()[i];
					layer->m_owner = m_player;
					layer->m_studio_hdr = m_player->studio_hdr();

					if (layer->m_sequence >= 2 && layer->m_weight > 0.f)
						m_player->update_dispatch_layer(layer, world_weapon->studio_hdr(), layer->m_sequence);
				}
			}
		}

		for (int i = as_right; i < as_main; i++) {
			build_animation(record, m_previous, i);
			std::memcpy(m_player->animstate(), &state, sizeof(animation_state_t));
		}

		resolver->run(record, m_previous);
		auto& resolver_info = resolver->m_info[index];
		resolver_info.m_last_side = resolver_info.m_side;

		if ((record->m_came_from_dormant && !record->m_may_has_fake) || record->m_lag <= 1) {
			resolver->m_info[index].m_side = 0;
			resolver->m_info[index].m_desync = 0.f;
			resolver->force_off(m_player);
		}

		resolver->apply_resolver_info(record);
	}

	build_animation(record, m_previous, as_main);
	record->m_bones = record->m_main_bones;

	//players->is_clamping(m_player) = true;
	//m_player->update_collision_bounds();
	//players->is_clamping(m_player) = false;

	record->m_collision_change_height = m_player->collision_change_height();
	record->m_collision_change_time = m_player->collision_change_time();

	if (!teammate) {
		switch (resolver->m_info[index].m_side) {
			case as_right:
				record->m_inversed_bones = record->m_left_bones;
				record->m_unresolved_bones = record->m_zero_bones;
				break;
			case as_left:
				record->m_inversed_bones = record->m_right_bones;
				record->m_unresolved_bones = record->m_zero_bones;
				break;
			case as_zero:
				record->m_inversed_bones = record->m_right_bones;
				record->m_unresolved_bones = record->m_left_bones;
				break;
		}
	} else
		record->m_bones = record->m_inversed_bones = record->m_unresolved_bones = record->m_visual_bones;
}

STFI void get_animation_layers(animation_layers_t output_layer) {
	globals->m_local->store_layers(output_layer);
	std::memcpy(&output_layer[ANIMATION_LAYER_MOVEMENT_JUMP_OR_FALL],
				&players->m_local_player.m_layers[ANIMATION_LAYER_MOVEMENT_JUMP_OR_FALL], sizeof(animation_layer_t));

	std::memcpy(&output_layer[ANIMATION_LAYER_MOVEMENT_LAND_OR_CLIMB],
				&players->m_local_player.m_layers[ANIMATION_LAYER_MOVEMENT_LAND_OR_CLIMB], sizeof(animation_layer_t));

	std::memcpy(&output_layer[ANIMATION_LAYER_ALIVELOOP],
				&players->m_local_player.m_layers[ANIMATION_LAYER_ALIVELOOP], sizeof(animation_layer_t));

	std::memcpy(&output_layer[ANIMATION_LAYER_LEAN],
				&players->m_local_player.m_layers[ANIMATION_LAYER_LEAN], sizeof(animation_layer_t));

	std::memcpy(&output_layer[ANIMATION_LAYER_MOVEMENT_MOVE],
				&players->m_local_player.m_layers[ANIMATION_LAYER_MOVEMENT_MOVE], sizeof(animation_layer_t));

	std::memcpy(&output_layer[ANIMATION_LAYER_MOVEMENT_STRAFECHANGE],
				&players->m_local_player.m_layers[ANIMATION_LAYER_MOVEMENT_STRAFECHANGE], sizeof(animation_layer_t));

	std::memcpy(&output_layer[ANIMATION_LAYER_ALIVELOOP],
				&players->m_local_player.m_layers[ANIMATION_LAYER_ALIVELOOP], sizeof(animation_layer_t));
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

	vec3d angles = player->eye_angles();

	const auto hideshot = hvh::exploits->m_type == hvh::exploits_t::e_type::hideshot && globals->m_weapon->is_gun() && globals->m_weapon->item_definition_index() != e_weapon_type::weapon_revolver && hvh::exploits->m_charged;

	if (hideshot)
		interfaces::global_vars->m_curtime -= TICKS_TO_TIME(8);

	const auto command_number = interfaces::client_state->get_current_tick();
	auto& entry = engine_prediction->m_entries[command_number % max_input];
	if (entry.m_shift > 0 && entry.m_sequence == command_number) {
		interfaces::global_vars->m_curtime += TICKS_TO_TIME(entry.m_shift);
	}

	animstate->update(angles.y, angles.x);
	animation_state_t pred = *animstate;

	*animstate = backup_state;
	player->apply_layers(backup_layers);
	player->apply_poses(backup_poses);
	interfaces::global_vars->m_curtime = curtime_backup;

	return pred;
}

STFI void update_local_strafe_state(animation_state_t* state, user_cmd_t* cmd) {
	if (state->m_velocity_length_xy > 0 && state->m_on_ground) {
		float raw_yaw_ideal = (std::atan2(-globals->m_local->velocity().y, -globals->m_local->velocity().x) * 180 / fPI);
		if (raw_yaw_ideal < 0)
			raw_yaw_ideal += 360;

		state->m_move_yaw = state->m_move_yaw_ideal = math::normalize_yaw(math::angle_diff(raw_yaw_ideal, players->m_local_player.m_last_goal_feet_yaw));
	}

	const auto predicted_animstate = predict_animation_state(globals->m_local);
	if (!predicted_animstate.has_value())
		return;

	float foot_yaw = predicted_animstate->m_goal_feet_yaw;
	int buttons = globals->m_local->buttons();

	vec3d forward;
	vec3d right;
	vec3d up;

	math::angle_vectors({ 0.f, foot_yaw, 0.f }, forward, right, up);
	right = right.normalized();

	const auto speed = state->m_speed_as_portion_of_walk_top_speed;

	float vel_to_right_dot = state->m_velocity_normalized_non_zero.dot(right);
	float vel_to_foward_dot = state->m_velocity_normalized_non_zero.dot(forward);

	bool move_right = (buttons & (in_moveright)) != 0;
	bool move_left = (buttons & (in_moveleft)) != 0;
	bool move_forward = (buttons & (in_forward)) != 0;
	bool move_backward = (buttons & (in_back)) != 0;

	bool strafe_right = (speed >= 0.73f && move_right && !move_left && vel_to_right_dot < -0.63f);
	bool strafe_left = (speed >= 0.73f && move_left && !move_right && vel_to_right_dot > 0.63f);
	bool strafe_forward = (speed >= 0.65f && move_forward && !move_backward && vel_to_foward_dot < -0.55f);
	bool strafe_backward = (speed >= 0.65f && move_backward && !move_forward && vel_to_foward_dot > 0.55f);

	globals->m_local->strafing() = (strafe_right || strafe_left || strafe_forward || strafe_backward);
}

STFI void update_fake_model(user_cmd_t* cmd) {
	auto& animstate = players->m_local_player.m_fake_state;
	//auto animstate = globals->m_local->animstate();
	static base_handle_t* selfhandle = nullptr;
	static float spawntime = globals->m_local->spawn_time();

	const auto alloc = animstate == nullptr;
	const auto change = !alloc && selfhandle != &globals->m_local->get_ref_handle();
	const auto reset = !alloc && !change && globals->m_local->spawn_time() != spawntime;

	if (change) {
		std::memset(animstate, 0, sizeof(animation_state_t));
		selfhandle = (base_handle_t*)&globals->m_local->get_ref_handle();
	}
	if (reset) {
		animstate->reset();
		spawntime = globals->m_local->spawn_time();
		players->m_local_player.m_valid = false;
	}

	if (alloc || change) {
		if (animstate != nullptr) {
			interfaces::mem_alloc->free(animstate);
			animstate = nullptr;
		}

		players->m_local_player.m_valid = false;

		animstate = (animation_state_t*)interfaces::mem_alloc->alloc(sizeof(animation_state_t));
		if (animstate != nullptr)
			animstate->create((base_entity_t*)globals->m_local);
	}

	if (animstate == nullptr)
		return;

	if (animstate->m_last_update_frame == interfaces::global_vars->m_framecount)
		animstate->m_last_update_frame -= 1;

	animation_layers_t layers{};
	float poses[24]{};

	globals->m_local->store_layers(layers);
	globals->m_local->store_poses(poses);

	if (*globals->m_send_packet) {
		update_local_strafe_state(animstate, cmd);

		players->is_updating(globals->m_local) = true;
		animstate->update(cmd->m_viewangles.y, cmd->m_viewangles.x);
		players->is_updating(globals->m_local) = false;

		globals->m_local->invalidate_bone_cache(interfaces::global_vars->m_framecount);

		players->is_setuping(globals->m_local) = true;
		globals->m_local->setup_bones(nullptr, -1, 0x7FF00, interfaces::global_vars->m_curtime);
		players->is_setuping(globals->m_local) = false;

		players->is_clamping(globals->m_local) = true;
		clamp_bones_in_bbox(globals->m_local, globals->m_local->bone_cache().base(), 0x7FF00, interfaces::global_vars->m_curtime, cmd->m_viewangles);
		players->is_clamping(globals->m_local) = false;

		globals->m_local->store_bone_cache(players->m_local_player.m_fake_bones);
		math::change_bones_position(players->m_local_player.m_fake_bones, sdk::max_bones, globals->m_local->render_origin(), vec3d{ 0.f, 0.f, 0.f });
	}

	globals->m_local->apply_layers(layers);
	globals->m_local->apply_poses(poses);
}

STFI int lookup_bone(base_entity_t* entity, const char* name) {
	return patterns::lookup_bone.as<int(__thiscall*)(base_entity_t*, const char*)>()(entity, name);
}

STFI vec3d matrix_get_origin(const matrix3x4_t& src) {
	return { src[0][3], src[1][3], src[2][3] };
}

STFI vec3d get_head_pos(cs_player_t* player, matrix3x4_t* bones) {
	const auto head = lookup_bone(player, STRC("head_0"));
	return matrix_get_origin(bones[head]);
}

void players_t::update_local_player(user_cmd_t* cmd) {
	interfaces::prediction->set_local_viewangles(cmd->m_viewangles);

	if (!globals->m_local->is_alive() || !ctx->is_valid()) {
		m_local_player.m_valid = false;
		return;
	}

	update_fake_model(cmd);

	//globals->m_local->draw_server_hitboxes();

	auto& animstate = m_local_player.m_real_state;
	//auto animstate = globals->m_local->animstate();
	static base_handle_t* selfhandle = nullptr;
	static float spawntime = globals->m_local->spawn_time();

	const auto alloc = animstate == nullptr;
	const auto change = !alloc && selfhandle != &globals->m_local->get_ref_handle();
	const auto reset = !alloc && !change && globals->m_local->spawn_time() != spawntime;

	if (change) {
		std::memset(animstate, 0, sizeof(animation_state_t));
		selfhandle = (base_handle_t*)&globals->m_local->get_ref_handle();
	}
	if (reset) {
		animstate->reset();
		spawntime = globals->m_local->spawn_time();
		players->m_local_player.m_valid = false;
	}

	if (alloc || change) {
		if (animstate != nullptr) {
			interfaces::mem_alloc->free(animstate);
			animstate = nullptr;
		}

		players->m_local_player.m_valid = false;

		animstate = (animation_state_t*)interfaces::mem_alloc->alloc(sizeof(animation_state_t));
		if (animstate != nullptr)
			animstate->create((base_entity_t*)globals->m_local);
	}

	if (animstate == nullptr)
		return;

	if (animstate->m_last_update_frame == interfaces::global_vars->m_framecount)
		animstate->m_last_update_frame -= 1;

	static auto deadflag = netvars->get_offset(HASH("DT_BasePlayer"), HASH("deadflag"));
	vec3d old_ang = *(vec3d*)((DWORD)globals->m_local + deadflag + 4);
	const auto& current_angle = m_local_player.m_current_angle;
	*(vec3d*)((DWORD)globals->m_local + deadflag + 4) = current_angle;

	if (m_local_player.m_valid) {
		//animation_layers_t layers{};
		//get_animation_layers(layers);
		//globals->m_local->apply_layers(layers);
		//play_custom_animations(animstate, globals->m_local->animation_layers(), cmd);
	}

	//if (*globals->m_send_packet)
	update_local_strafe_state(animstate, cmd);

	const auto curtime_backup = interfaces::global_vars->m_curtime;
	interfaces::global_vars->m_curtime = TICKS_TO_TIME(globals->m_local->tickbase());

	const auto hideshot = hvh::exploits->m_type == hvh::exploits_t::e_type::hideshot && globals->m_weapon->is_gun() && globals->m_weapon->item_definition_index() != e_weapon_type::weapon_revolver && hvh::exploits->m_charged;

	//if (hideshot)
	//	interfaces::global_vars->m_curtime -= TICKS_TO_TIME(8);

	auto should_update = *globals->m_send_packet;
	if (hvh::exploits->m_shift_ticks > 0) {
		m_local_player.m_current_angle = cmd->m_viewangles;
		should_update = true;
	}

	//if (exploits->m_breaking_lc)
	//	should_update = false;

	players->is_updating(globals->m_local) = true;
	animstate->update(current_angle.y, current_angle.x);
	players->is_updating(globals->m_local) = false;

	globals->m_local->invalidate_bone_cache(interfaces::global_vars->m_framecount);

	players->is_setuping(globals->m_local) = true;
	globals->m_local->setup_bones(nullptr, -1, 0x7FF00, interfaces::global_vars->m_curtime);
	players->is_setuping(globals->m_local) = false;

	auto& entry = engine_prediction->m_entries[cmd->m_command_number % max_input];
	if (entry.m_shift > 0 && entry.m_sequence == cmd->m_command_number) {
		interfaces::global_vars->m_curtime += TICKS_TO_TIME(entry.m_shift);
	}

	globals->m_local->store_layers(m_local_player.m_layers);
	globals->m_local->store_poses(m_local_player.m_poses);

	players->m_local_player.m_valid = true;
	interfaces::global_vars->m_curtime = curtime_backup;

	if (*globals->m_send_packet) {
		m_local_player.m_last_goal_feet_yaw = animstate->m_goal_feet_yaw;
		if (m_local_player.m_fake_state != nullptr) {
			m_local_player.m_desync = std::clamp(
					math::normalize_yaw(
							math::angle_diff(m_local_player.m_fake_state->m_goal_feet_yaw, m_local_player.m_real_state->m_goal_feet_yaw)),
					-58.f, 58.f);

			m_local_player.m_head_delta = get_head_pos(globals->m_local, m_local_player.m_bones).dist(get_head_pos(globals->m_local, m_local_player.m_fake_bones));
		}
	}

	//if (hideshot && hvh::is_shooting(cmd)/*cmd->m_buttons.has(in_attack)*/)
	//	should_update = false;

	if (should_update) {
		players->is_clamping(globals->m_local) = true;
		clamp_bones_in_bbox(globals->m_local, globals->m_local->bone_cache().base(), 0x7FF00, interfaces::global_vars->m_curtime, m_local_player.m_current_angle);
		players->is_clamping(globals->m_local) = false;

		globals->m_local->store_bone_cache(m_local_player.m_bones);

		math::change_bones_position(m_local_player.m_bones, sdk::max_bones, globals->m_local->render_origin(), vec3d{ 0.f, 0.f, 0.f });
	}

	m_local_player.m_layers[12].m_weight = FLT_EPSILON;
	*(vec3d*)((DWORD)globals->m_local + deadflag + 4) = old_ang;

	//m_local_player.m_real_state = globals->m_local->animstate();
	std::memcpy(globals->m_local->animstate(), m_local_player.m_real_state, sizeof(animation_state_t));
}