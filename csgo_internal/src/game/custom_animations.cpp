#include "custom_animations.hpp"
#include "../../sdk/ik_solver.hpp"
#include "../globals.hpp"
#include "../interfaces.hpp"
#include "engine_prediction.hpp"
#include "players.hpp"

using namespace sdk;

void animation_state_t::increment_layer_cycle(animation_layer_t* layer, bool loop) {
	float new_cycle = (layer->m_playback_rate * this->m_last_update_increment) + layer->m_cycle;
	if (!loop && new_cycle >= 1.0f)
		new_cycle = 0.999f;

	new_cycle -= (int)new_cycle;
	if (new_cycle < 0.0f)
		new_cycle += 1.0f;

	if (new_cycle > 1.0f)
		new_cycle -= 1.0f;

	layer->m_cycle = new_cycle;
}

bool animation_state_t::is_layer_sequence_finished(animation_layer_t* layer, float time) {
	return (layer->m_playback_rate * time) + layer->m_cycle >= 1.0f;
}

void animation_state_t::set_layer_cycle(animation_layer_t* layer, float_t cycle) {
	if (layer != nullptr)
		layer->m_cycle = cycle;
}

void animation_state_t::set_layer_rate(animation_layer_t* layer, float rate) {
	if (layer != nullptr)
		layer->m_playback_rate = rate;
}

void animation_state_t::set_layer_weight(animation_layer_t* layer, float weight) {
	if (layer != nullptr)
		layer->m_weight = weight;
}

void animation_state_t::set_layer_weight_rate(animation_layer_t* layer, float prev) {
	if (layer != nullptr)
		layer->m_weight_delta_rate = (layer->m_weight - prev) / this->m_last_update_increment;
}

void animation_state_t::set_layer_sequence(animation_layer_t* layer, int activity) {
	int sequence = this->select_sequence_from_activity_modifier(activity);
	if (sequence < 2)
		return;

	layer->m_cycle = 0.0f;
	layer->m_weight = 0.0f;
	layer->m_sequence = sequence;
	layer->m_playback_rate = m_player->get_layer_sequence_cycle_rate(layer, sequence);
}

int animation_state_t::select_sequence_from_activity_modifier(int activity) {
	bool ducking = this->m_anim_duck_amount > 0.55f;
	bool running = this->m_speed_as_portion_of_walk_top_speed > 0.25f;

	int current_sequence = 0;
	switch (activity) {
		case act_csgo_jump:
			current_sequence = 15 + (int)running;
			if (ducking)
				current_sequence = 17 + (int)running;
			break;
		case act_csgo_alive_loop:
			current_sequence = 8;
			if (this->m_weapon_last != this->m_weapon)
				current_sequence = 9;
			break;
		case act_csgo_land_light:
			current_sequence = 20;
			if (running)
				current_sequence = 22;

			if (ducking) {
				current_sequence = 21;
				if (running)
					current_sequence = 19;
			}
			break;
		case act_csgo_land_heavy:
			current_sequence = 23;
			if (running)
				current_sequence = 24;
			break;

		case act_csgo_idle_adjust_stoppedmoving: current_sequence = 6; break;
		case act_csgo_fall: current_sequence = 14; break;
		case act_csgo_idle_turn_balanceadjust: current_sequence = 4; break;
		case act_csgo_climb_ladder: current_sequence = 13; break;
	}

	return current_sequence;
}

__forceinline void play_custom_animations(animation_state_t* state, animation_layers_t layers, user_cmd_t* cmd) {
	if (interfaces::game_rules->is_freeze_time() || (globals->m_local->flags().has(fl_frozen))) {
		players->m_local_player.m_move_type = e_move_type::type_none;
		players->m_local_player.m_flags = fl_onground;
		return;
	}

	auto alive_loop = &layers[11];
	if (alive_loop == nullptr)
		return;

	if (globals->m_local->get_sequence_activity(alive_loop->m_sequence) != act_csgo_alive_loop) {
		state->set_layer_sequence(alive_loop, act_csgo_alive_loop);
		state->set_layer_cycle(alive_loop, math::random_float(0.0f, 1.0f));
		state->set_layer_rate(alive_loop, globals->m_local->get_layer_sequence_cycle_rate(alive_loop, alive_loop->m_sequence) * math::random_float(0.8f, 1.1f));
	} else {
		float retain_cycle = alive_loop->m_cycle;
		if (state->m_weapon != state->m_weapon_last) {
			state->set_layer_sequence(alive_loop, act_csgo_alive_loop);
			state->set_layer_cycle(alive_loop, retain_cycle);
		} else if (state->is_layer_sequence_finished(alive_loop, state->m_last_update_increment))
			state->set_layer_rate(alive_loop,
								  globals->m_local->get_layer_sequence_cycle_rate(alive_loop, alive_loop->m_sequence) * math::random_float(0.8f, 1.1f));
		else
			state->set_layer_weight(alive_loop, remap_value(state->m_speed_as_portion_of_run_top_speed, 0.55f, 0.9f, 1.0f, 0.0f));
	}

	state->increment_layer_cycle(alive_loop, true);

	auto land_or_climb = &layers[5];
	if (land_or_climb == nullptr)
		return;

	auto jump_or_fall = &layers[4];
	if (jump_or_fall == nullptr)
		return;

	auto movetype = globals->m_local->movetype();

	if (players->m_local_player.m_move_type != e_move_type::type_ladder && movetype == e_move_type::type_ladder)
		state->set_layer_sequence(land_or_climb, act_csgo_climb_ladder);
	else if (players->m_local_player.m_move_type == e_move_type::type_ladder && movetype != e_move_type::type_ladder)
		state->set_layer_sequence(jump_or_fall, act_csgo_fall);
	else {
		if (globals->m_local->flags().has(fl_onground)) {
			if (!players->m_local_player.m_flags.has(fl_onground))
				state->set_layer_sequence(jump_or_fall, state->m_duration_in_air > 1.f ? act_csgo_land_heavy : act_csgo_land_light);
		} else if (players->m_local_player.m_flags.has(fl_onground)) {
			if (globals->m_local->velocity().z > 0.0f)
				state->set_layer_sequence(jump_or_fall, act_csgo_jump);
			else
				state->set_layer_sequence(jump_or_fall, act_csgo_fall);
		}
	}

	players->m_local_player.m_move_type = movetype;
	players->m_local_player.m_flags = engine_prediction->m_unpredicted_data.m_flags;
}

STFI int lookup_bone(base_entity_t* entity, const char* name) {
	return patterns::lookup_bone.as<int(__thiscall*)(base_entity_t*, const char*)>()(entity, name);
}

void clamp_bones_in_bbox(cs_player_t* player, matrix3x4_t* bones, int mask, float curtime, const vec3d& eye_angles) {
	// abobus fix
	if (player->flags().has(fl_frozen)) {
		player->maxs().z = 72.f;

		auto collidable = player->collideable();

		if (collidable != nullptr)
			collidable->maxs().z = 72.f;
	}

	SET_AND_RESTORE(player->eye_angles(), eye_angles);
	SET_AND_RESTORE(interfaces::global_vars->m_curtime, curtime);

	players->is_clamping(player) = true;
	patterns::clamp_bones_in_bbox.as<void(__thiscall*)(cs_player_t*, matrix3x4_t*, uint32_t)>()(player, bones, mask);
	players->is_clamping(player) = false;

	//auto hdr = player->studio_hdr();
	//if (!hdr)
	//	return;

	//auto studiohdr = *(studiohdr_t**)hdr;
	//if (!studiohdr)
	//	return;

	//auto head_bone = lookup_bone(player, STRSC("head_0"));
	//if (head_bone < 0)
	//	return;

	//auto collideable = player->collideable();
	//if (!collideable)
	//	return;

	//const auto collision_origin = collideable->get_collision_origin();
	//const auto collision_bb_max = collideable->maxs();

	//auto max_collision_pos_z = collision_origin.z + collision_bb_max.z;
	//auto change_collision_time_diff = curtime - player->collision_change_time();
	//if (change_collision_time_diff < 0.2f) {
	//	auto time_multiplier = std::clamp(change_collision_time_diff * 5.f, 0.f, 1.f);
	//	max_collision_pos_z = ((max_collision_pos_z - player->collision_change_height()) * time_multiplier) + player->collision_change_height();
	//}

	//auto head_bone_position = bones[head_bone].get_origin();

	//vec3d eye_position{}, forward{}, right{}, up{};
	//eye_position = player->eye_position();
	//math::angle_vectors(eye_angles, forward, right, up);

	//vec3d head_position_to_change{};
	//auto target_difference = head_bone_position.dot(right) - eye_position.dot(right);
	//if (std::abs(target_difference) <= 3.f) {
	//	head_position_to_change = head_bone_position;
	//} else {
	//	auto head_direction = 0.f;
	//	if (target_difference < 0.f)
	//		head_direction = -1.f;
	//	else
	//		head_direction = 1.f;

	//	auto adjusted_right_direction = (right.normalize_in_place() * 3.f) * head_direction;
	//	head_position_to_change = (head_bone_position - (right * target_difference)) + adjusted_right_direction;
	//}

	//if (head_position_to_change.z > (max_collision_pos_z - 4.f))
	//	head_position_to_change.z = max_collision_pos_z - 4.f;

	//auto head_position_delta = vec3d{ head_position_to_change.x - eye_position.x, head_position_to_change.y - eye_position.y, 0.f };
	//if (head_position_delta.length2d() > 11.f) {
	//	auto normalized_delta = head_position_delta.normalize_in_place();

	//	head_position_to_change.x = (normalized_delta.x * 11.f) + eye_position.x;
	//	head_position_to_change.y = (normalized_delta.y * 11.f) + eye_position.y;
	//}

	//auto head_pos_difference = head_bone_position - head_position_to_change;
	//if (!head_pos_difference.is_valid() || head_pos_difference.length_sqr() >= 900.f)
	//	return;

	//auto left_ankle_bone = lookup_bone(player, STRSC("ankle_L"));
	//auto right_ankle_bone = lookup_bone(player, STRSC("ankle_R"));
	//auto spine_3_bone = lookup_bone(player, STRSC("spine_3"));

	//mstudioikchain_t* left_chain = nullptr;
	//mstudioikchain_t* right_chain = nullptr;

	//for (int i = 0; i < studiohdr->m_numikchains; i++) {
	//	mstudioikchain_t* ik_chain = studiohdr->ik_chain(i);
	//	const mstudioiklink_t* ik_link = ik_chain->link(2);

	//	if (ik_link->bone == left_ankle_bone)
	//		left_chain = ik_chain;
	//	else if (ik_link->bone == right_ankle_bone)
	//		right_chain = ik_chain;

	//	if (left_chain && right_chain)
	//		break;
	//}

	//if (studiohdr->m_num_bones <= 0)
	//	return;

	//auto left_ankle_bone_position = bones[left_ankle_bone].get_origin();
	//auto right_ankle_bone_position = bones[right_ankle_bone].get_origin();
	//auto lowest_ankle_position_z = std::min<float>(left_ankle_bone_position.z, right_ankle_bone_position.z);

	//auto solve_left_ik = false;
	//auto solve_right_ik = false;

	//auto ground_entity = (base_entity_t*)player->ground_entity_handle().get();

	//for (int i = 0; i < studiohdr->m_num_bones; ++i) {
	//	auto& current_bone = bones[i];
	//	auto current_bone_origin = current_bone.get_origin();

	//	if (!(hdr->m_bone_flags[i] & mask))
	//		continue;

	//	if (i == left_ankle_bone && left_chain && ground_entity) {
	//		solve_left_ik = true;
	//		continue;
	//	} else if (i == right_ankle_bone && right_chain && ground_entity) {
	//		solve_right_ik = true;
	//		continue;
	//	}
	//	auto head_adjust = 1.f;
	//	auto height_amount = 0.f;
	//	auto bone_parent = -1;

	//	if (ground_entity) {
	//		if (hdr->m_bone_parent.count()) {
	//			if (i >= 0 && spine_3_bone >= 0) {
	//				auto studio_hdr_mat_iter = *reinterpret_cast<int*>(reinterpret_cast<std::uintptr_t>(hdr) + 0x44);
	//				bone_parent = *reinterpret_cast<int*>(studio_hdr_mat_iter + 4 * i);
	//				if (bone_parent != -1) {
	//					while (bone_parent != spine_3_bone) {
	//						bone_parent = *reinterpret_cast<int*>(studio_hdr_mat_iter + 4 * bone_parent);
	//						if (bone_parent == -1)
	//							goto LABEL_56;
	//					}
	//					goto LABEL_63;
	//				}
	//			}
	//		}
	//	LABEL_56:
	//		auto height_diff = current_bone_origin.z - (head_bone_position.z - head_position_to_change.z);
	//		if (lowest_ankle_position_z == head_bone_position.z) {
	//			if ((height_diff - head_bone_position.z) >= 0.f) {
	//				head_adjust = 1.f;
	//				goto LABEL_62;
	//			}
	//		LABEL_60:
	//			head_adjust = 0.f;
	//		} else {
	//			auto height_lerp = (height_diff - lowest_ankle_position_z) / (head_bone_position.z - lowest_ankle_position_z);
	//			if (height_lerp < 0.f)
	//				goto LABEL_60;
	//			head_adjust = std::min<float>(height_lerp, 1.0);
	//		}
	//	LABEL_62:
	//		height_amount = head_bone_position.z - head_position_to_change.z;
	//	}
	//LABEL_63:
	//	current_bone_origin -= (head_pos_difference * head_adjust);
	//	current_bone.set_origin(current_bone_origin);
	//}

	//if (solve_left_ik)
	//	studio_solve_ik(left_chain->link(0)->bone, left_chain->link(1)->bone, left_ankle_bone, left_ankle_bone_position, bones);

	//if (solve_right_ik)
	//	studio_solve_ik(right_chain->link(0)->bone, right_chain->link(1)->bone, right_ankle_bone, right_ankle_bone_position, bones);
}

//
//namespace bone_merge {
//	std::uintptr_t& get_bone_merge(cs_player_t* player) {
//		static auto get_bone_merge_offset = utils::find_pattern("client.dll", "89 86 ? ? ? ? E8 ? ? ? ? FF 75 08").add(2).as<uintptr_t*>();
//		return *(uintptr_t*)((std::uintptr_t)player + *get_bone_merge_offset);
//	}
//
//	void update_cache(std::uintptr_t& bone_merge) {
//		static auto update_merge_cache = utils::find_pattern("client.dll", "E8 ? ? ? ? 83 7E 10 ? 74 64").rel32(1).as<void(__thiscall*)(std::uintptr_t)>();
//		update_merge_cache(bone_merge);
//	}
//} // namespace bone_merge
//
//struct mstudioposeparamdesc_t {
//	int name_index;
//
//	char* const get_name(void) const {
//		return ((char*)this) + name_index;
//	}
//
//	int flags;
//	float start;
//	float end;
//	float loop;
//};
//
//static mstudioposeparamdesc_t* get_pose_parameter(studio_hdr_t* hdr, int i) {
//	static auto fn = utils::find_pattern("client.dll", "55 8B EC 8B 45 08 57 8B F9 8B 4F 04 85 C9 75 15 8B").as<mstudioposeparamdesc_t*(__thiscall*)(studio_hdr_t*, int)>();
//	return fn(hdr, i);
//}
//
//static float studio_get_pose_parameter(cs_player_t* player, int index, float value) {
//	auto hdr = player->studio_hdr();
//	if (!hdr)
//		return 0.f;
//
//	if (index < 0 || index > 24)
//		return 0.f;
//
//	auto pose_parameter = get_pose_parameter(hdr, index);
//	if (!pose_parameter)
//		return 0.f;
//
//	if (pose_parameter->loop) {
//		float wrap = (pose_parameter->start + pose_parameter->end) / 2.f + pose_parameter->loop / 2.f;
//		float shift = pose_parameter->loop - wrap;
//
//		value = value - pose_parameter->loop * std::floorf((value + shift) / pose_parameter->loop);
//	}
//
//	return (value - pose_parameter->start) / (pose_parameter->end - pose_parameter->start);
//}
//
//void merge_matching_poses(std::uintptr_t& bone_merge, float* poses, float* target_poses) {
//	bone_merge::update_cache(bone_merge);
//
//	if (*reinterpret_cast<std::uintptr_t*>(bone_merge + 0x10) && *reinterpret_cast<std::uintptr_t*>(bone_merge + 0x8C)) {
//		int* index = reinterpret_cast<int*>(bone_merge + 0x20);
//		for (int i = 0; i < 24; ++i) {
//			if (*index != -1) {
//				cs_player_t* target = *(cs_player_t**)(bone_merge + 0x4);
//				studio_hdr_t* hdr = target->studio_hdr();
//				float pose_param_value = 0.f;
//
//				if (hdr && *(studiohdr_t**)hdr && i >= 0) {
//					float pose = target_poses[i];
//					auto pose_param = get_pose_parameter(hdr, i);
//
//					pose_param_value = pose * (pose_param->end - pose_param->start) + pose_param->start;
//				}
//
//				auto second_target = *(cs_player_t**)(bone_merge);
//				auto second_hdr = second_target->studio_hdr();
//
//				poses[*index] = studio_get_pose_parameter(second_target, *index, pose_param_value);
//			}
//
//			++index;
//		}
//	}
//}
//
//bool c_bone_builder::can_be_animated(cs_player_t* player) {
//	if (!player->use_new_animstate() || !player->animstate())
//		return false;
//
//	auto weapon = player->active_weapon();
//
//	if (!weapon)
//		return false;
//
//	auto world_model = (cs_player_t*)weapon->world_model().get();
//	if (!world_model || *reinterpret_cast<short*>(reinterpret_cast<std::uintptr_t>(world_model) + 0x26E) == -1)
//		return player == globals->m_local;
//
//	return true;
//}
//
///*
//
//		ik_context_ptr = utils::find_pattern("client.dll", "8B 8F ? ? ? ? 89 4C 24 1C");
//		ik_ctx_construct = utils::find_pattern("client.dll", "53 8B D9 F6 C3 03 74 0B FF 15 ? ? ? ? 84 C0 74 01 ? C7 83 ? ? ? ? ? ? ? ? 8B CB");
//		ik_ctx_destruct = utils::find_pattern("client.dll", "56 8B F1 57 8D 8E ? ? ? ? E8 ? ? ? ? 8D 8E ? ? ? ? E8 ? ? ? ? 83 BE ? ? ? ? ?");
//		ik_ctx_init = utils::find_pattern("client.dll", "55 8B EC 83 EC 08 8B 45 08 56 57 8B F9 8D 8F");
//		ik_ctx_update_targets = utils::find_pattern("client.dll", "E8 ? ? ? ? 8B 47 FC 8D 4F FC F3 0F 10 44 24").rel32(1);
//		ik_ctx_solve_dependencies = utils::find_pattern("client.dll", "E8 ? ? ? ? 8B 44 24 40 8B 4D 0C").rel32(1);
//
//*/
//
//struct c_ik_context {
//	void constructor() {
//		static auto ik_ctor = utils::find_pattern("client.dll", "53 8B D9 F6 C3 03 74 0B FF 15 ? ? ? ? 84 C0 74 01 ? C7 83 ? ? ? ? ? ? ? ? 8B CB").as<void(__thiscall*)(void*)>();
//		ik_ctor(this);
//	}
//
//	void destructor() {
//		static auto ik_dector = utils::find_pattern("client.dll", "56 8B F1 57 8D 8E ? ? ? ? E8 ? ? ? ? 8D 8E ? ? ? ? E8 ? ? ? ? 83 BE").as<void(__thiscall*)(void*)>();
//		ik_dector(this);
//	}
//
//	void clear_targets() {
//		auto i = 0;
//		auto count = *reinterpret_cast<int*>(reinterpret_cast<std::uintptr_t>(this) + 0xFF0);
//
//		if (count > 0) {
//			auto target = reinterpret_cast<int*>(reinterpret_cast<std::uintptr_t>(this) + 0xD0);
//
//			do {
//				*target = -9999;
//				target += 85;
//				++i;
//			} while (i < count);
//		}
//	}
//
//	void init(studio_hdr_t* hdr, vec3d* angles, vec3d* origin, float time, int frames, int mask) {
//		using init_fn = void(__thiscall*)(void*, studio_hdr_t*, vec3d*, vec3d*, float, int, int);
//		static auto fn = utils::find_pattern("client.dll", "55 8B EC 83 EC 08 8B 45 08 56 57 8B F9 8D 8F").as<init_fn>();
//		fn(this, hdr, angles, origin, time, frames, mask);
//	}
//
//	void update_targets(vec3d* pos, quaternion_t* qua, matrix3x4_t* matrix, uint8_t* bone_computed) {
//		using update_targets_fn = void(__thiscall*)(void*, vec3d*, quaternion_t*, matrix3x4_t*, uint8_t*);
//		static auto fn = utils::find_pattern("client.dll", "E8 ? ? ? ? 8B 47 FC 8D 4F FC F3 0F 10 44 24").rel32(1).as<update_targets_fn>();
//		fn(this, pos, qua, matrix, bone_computed);
//	}
//
//	void solve_dependencies(vec3d* pos, quaternion_t* qua, matrix3x4_t* matrix, uint8_t* bone_computed) {
//		using solve_dependencies_fn = void(__thiscall*)(void*, vec3d*, quaternion_t*, matrix3x4_t*, uint8_t*);
//		static auto fn = utils::find_pattern("client.dll", "E8 ? ? ? ? 8B 44 24 40 8B 4D 0C").rel32(1).as<solve_dependencies_fn>();
//		fn(this, pos, qua, matrix, bone_computed);
//	}
//};
//
//struct bone_setup_t {
//	studio_hdr_t* hdr{};
//	int mask{};
//	float* pose_parameter{};
//	void* pose_debugger{};
//
//	void init_pose(vec3d pos[], quaternion_t q[], studio_hdr_t* hdr) {
//		static auto init_pose_fn = utils::find_pattern("client.dll", "55 8B EC 83 EC 10 53 8B D9 89 55 F8 56 57 89 5D F4 8B 0B 89 4D F0").as<uint64_t>();
//
//		__asm
//		{
//			mov eax, this
//			mov esi, q
//			mov edx, pos
//			push dword ptr[hdr + 4]
//			mov ecx, [eax]
//			push esi
//			call init_pose_fn
//			add esp, 8
//		}
//	}
//
//	void accumulate_pose(vec3d pos[], quaternion_t q[], int sequence, float cycle, float weight, float time, c_ik_context* ctx) {
//		using accumulate_pose_fn = void(__thiscall*)(bone_setup_t*, vec3d*, quaternion_t*, int, float, float, float, c_ik_context*);
//		static auto fn = utils::find_pattern("client.dll", "55 8B EC 83 E4 F0 B8 ? ? ? ? E8 ? ? ? ? A1").as<accumulate_pose_fn>();
//		fn(this, pos, q, sequence, cycle, weight, time, ctx);
//	}
//
//	void calc_autoplay_sequences(vec3d pos[], quaternion_t q[], float real_time, c_ik_context* ctx) {
//		static auto calc_autoplay_sequences_fn = utils::find_pattern("client.dll", "55 8B EC 83 EC ? 53 56 57 8B 7D ? 8B D9 F3 0F 11 5D").as<uint64_t>();
//
//		__asm
//		{
//			movss xmm3, real_time
//			mov eax, ctx
//			mov ecx, this
//			push eax
//			push q
//			push pos
//			call calc_autoplay_sequences_fn
//		}
//	}
//
//	void calc_bone_adjust(vec3d pos[], quaternion_t q[], float* controllers, int mask) {
//		static auto calc_bone_adjust_fn = utils::find_pattern("client.dll", "55 8B EC 83 E4 F8 81 EC ? ? ? ? 8B C1 89 54 24 04 89 44 24 2C 56 57 8B").as<uint64_t>();
//
//		__asm
//		{
//			mov eax, controllers
//			mov ecx, this
//			mov edx, pos; a2
//			push dword ptr[ecx + 4]; a5
//			mov ecx, [ecx]; a1
//			push eax; a4
//			push q; a3
//			call calc_bone_adjust_fn
//			add esp, 0xC
//		}
//	}
//};
//
//void c_bone_builder::get_skeleton(vec3d* position, quaternion_t* q) {
//	alignas(16) vec3d new_position[128]{};
//	alignas(16) quaternion_t new_q[128]{};
//
//	auto ik_context = reinterpret_cast<c_ik_context*>(animating->ik_ctx());
//
//	if (!ik_ctx)
//		ik_context = nullptr;
//
//	alignas(16) char buffer[32];
//	alignas(16) bone_setup_t* bone_setup = (bone_setup_t*)&buffer;
//
//	bone_setup->hdr = hdr;
//	bone_setup->mask = mask;
//	bone_setup->pose_parameter = poses.data();
//	bone_setup->pose_debugger = nullptr;
//
//	bone_setup->init_pose(position, q, hdr);
//	bone_setup->accumulate_pose(position, q, animating->sequence(), animating->cycle(), 1.f, time, ik_context);
//
//	int layer[13]{};
//
//	for (int i = 0; i < layer_count; ++i) {
//		auto& final_layer = layers[i];
//
//		if (final_layer.m_weight > 0.f && final_layer.m_order != 13 && final_layer.m_order >= 0 && final_layer.m_order < layer_count)
//			layer[final_layer.m_order] = i;
//	}
//
//	char tmp_buffer[4208]{};
//	auto world_ik = reinterpret_cast<c_ik_context*>(tmp_buffer);
//
//	auto weapon = animating->active_weapon();
//	cs_player_t* world_weapon = nullptr;
//	if (weapon)
//		world_weapon = (cs_player_t*)weapon->world_model().get();
//
//	auto wrong_weapon = [&]() {
//		if (can_be_animated(animating) && world_weapon) {
//			auto bone_merge = bone_merge::get_bone_merge(world_weapon);
//			if (bone_merge) {
//				merge_matching_poses(bone_merge, poses_world.data(), poses.data());
//
//				auto world_hdr = world_weapon->studio_hdr();
//
//				world_ik->constructor();
//				world_ik->init(world_hdr, &angles, &origin, time, 0, 0X00040000);
//
//				alignas(16) char buffer2[32]{};
//				alignas(16) bone_setup_t* world_setup = reinterpret_cast<bone_setup_t*>(&buffer2);
//
//				world_setup->hdr = world_hdr;
//				world_setup->mask = 0X00040000;
//				world_setup->pose_parameter = poses_world.data();
//				world_setup->pose_debugger = nullptr;
//
//				world_setup->init_pose(new_position, new_q, world_hdr);
//
//				for (int i = 0; i < layer_count; ++i) {
//					auto layer = &layers[i];
//
//					if (layer && layer->m_sequence > 1 && layer->m_weight > 0.f) {
//						if (dispatch && animating == globals->m_local)
//							animating->update_dispatch_layer(layer, world_hdr, layer->m_sequence);
//
//						if (!dispatch || layer->m_dispatched_src <= 0 || layer->m_dispatched_src >= (*reinterpret_cast<studiohdr_t**>(world_hdr))->m_local_seq)
//							bone_setup->accumulate_pose(position, q, layer->m_sequence, layer->m_cycle, layer->m_weight, time, ik_context);
//						else if (dispatch) {
//
//							static auto copy_from_follow =
//									utils::find_pattern("client.dll", "55 8B EC 83 EC ? 53 56 57 8B F9 89 7D ? E8 ? ? ? ? 83 7F ? ? 0F 84 ? ? ? ? 8B 87 ? ? ? ? 85 C0 74")
//											.as<void(__thiscall*)(std::uintptr_t, vec3d*, quaternion_t*, int, vec3d*, quaternion_t*)>();
//
//							static auto add_dependencies =
//									utils::find_pattern("client.dll", "55 8B EC 81 EC BC ? ? ? 53 56 57")
//											.as<void(__thiscall*)(c_ik_context*, float, int, float, const float[], float)>();
//
//							static auto copy_to_follow =
//									utils::find_pattern("client.dll", "55 8B EC 83 EC ? 53 56 57 8B F9 89 7D ? E8 ? ? ? ? 83 7F ? ? 0F 84 ? ? ? ? 8B 87 ? ? ? ? 85 C0 0F 84")
//											.as<void(__thiscall*)(std::uintptr_t, vec3d*, quaternion_t*, int, vec3d*, quaternion_t*)>();
//
//							copy_from_follow(bone_merge, position, q, 0X00040000, new_position, new_q);
//							if (ik_context)
//								add_dependencies(ik_context, *reinterpret_cast<float*>(reinterpret_cast<std::uintptr_t>(animating) + 0xA14), layer->m_sequence, layer->m_cycle, poses.data(), layer->m_weight);
//
//							world_setup->accumulate_pose(new_position, new_q, layer->m_dispatched_src, layer->m_cycle, layer->m_weight, time, world_ik);
//							copy_to_follow(bone_merge, new_position, new_q, 0X00040000, position, q);
//						}
//					}
//				}
//
//				world_ik->destructor();
//				return false;
//			}
//		}
//		return true;
//	};
//
//	if (wrong_weapon()) {
//		for (int i = 0; i < this->layer_count; ++i) {
//			int layer_count = layer[i];
//
//			if (layer_count >= 0 && layer_count < this->layer_count) {
//				auto final_layer = &layers[i];
//				bone_setup->accumulate_pose(position, q, final_layer->m_sequence, final_layer->m_cycle, final_layer->m_weight, time, ik_context);
//			}
//		}
//	}
//
//	if (ik_context) {
//		world_ik->constructor();
//		world_ik->init(hdr, &angles, &origin, time, 0, mask);
//		bone_setup->calc_autoplay_sequences(position, q, time, world_ik);
//		world_ik->destructor();
//	} else
//		bone_setup->calc_autoplay_sequences(position, q, time, nullptr);
//
//	bone_setup->calc_bone_adjust(position, q, reinterpret_cast<float*>(reinterpret_cast<std::uintptr_t>(animating) + 0xA54), mask);
//}
//
//void c_bone_builder::studio_build_matrices(studio_hdr_t* hdr, const matrix3x4_t& world_transform, vec3d* pos, quaternion_t* q, int mask, matrix3x4_t* out, uint32_t* bone_computed) {
//	int i = 0;
//	int chain_length = 0;
//	int bone = -1;
//	auto studio_hdr = *reinterpret_cast<studiohdr_t**>(hdr);
//
//	if (bone < -1 || bone >= studio_hdr->m_num_bones)
//		bone = 0;
//
//	utils::utl_vector_t<int>* bone_parent = reinterpret_cast<utils::utl_vector_t<int>*>(reinterpret_cast<std::uintptr_t>(hdr) + 0x44);
//	utils::utl_vector_t<int>* bone_flags = reinterpret_cast<utils::utl_vector_t<int>*>(reinterpret_cast<std::uintptr_t>(hdr) + 0x30);
//
//	int chain[128]{};
//	if (bone <= -1) {
//		chain_length = studio_hdr->m_num_bones;
//
//		for (i = 0; i < studio_hdr->m_num_bones; ++i)
//			chain[chain_length - i - 1] = i;
//	} else {
//		i = bone;
//
//		do {
//			chain[chain_length++] = i;
//			i = bone_parent->element(i);
//		} while (i != -1);
//	}
//
//	matrix3x4_t bone_matrix{};
//	for (int j = chain_length - 1; j >= 0; --j) {
//		i = chain[j];
//
//		if ((1 << (i & 0x1F)) & bone_computed[i >> 5])
//			continue;
//
//		int flag = bone_flags->element(i);
//		int parent = bone_parent->element(i);
//
//		if ((flag & mask) && q) {
//			bone_matrix.quaternion_matrix(q[i], pos[i]);
//
//			if (parent == -1)
//				out[i] = world_transform.contact_transforms(bone_matrix);
//			else
//				out[i] = out[parent].contact_transforms(bone_matrix);
//		}
//	}
//}
//
//void c_bone_builder::store(cs_player_t* player, matrix3x4_t* matrix, int mask) {
//	auto state = player->animstate();
//
//	animating = player;
//	origin = player == globals->m_local ? player->get_abs_origin() : player->origin();
//	layers = player->animation_layers();
//	hdr = player->studio_hdr();
//	layer_count = 13;
//	angles = player->get_abs_angles();
//	this->matrix = matrix;
//	this->mask = mask;
//	time = TICKS_TO_TIME(globals->m_local->tickbase());
//	attachments = false;
//	ik_ctx = false;
//	dispatch = true;
//
//	player->store_poses(poses.data());
//
//	eye_angles = player->eye_angles();
//
//	auto weapon = player->active_weapon();
//
//	base_combat_weapon_t* world_weapon = nullptr;
//	if (weapon)
//		world_weapon = (base_combat_weapon_t*)weapon->world_model().get();
//
//	if (world_weapon)
//		std::memcpy(poses_world.data(), world_weapon->poses(), sizeof(float) * 24);
//	else
//		std::memcpy(poses_world.data(), player->poses(), sizeof(float) * 24);
//
//	filled = true;
//}
//
//void c_bone_builder::setup() {
//	alignas(16) vec3d position[128]{};
//	alignas(16) quaternion_t q[128]{};
//
//	auto ik_context = (c_ik_context*)animating->ik_ctx();
//
//	if (!ik_ctx)
//		ik_context = nullptr;
//
//	hdr = animating->studio_hdr();
//
//	if (!hdr)
//		return;
//
//	std::uint32_t bone_computed[8]{};
//	std::memset(bone_computed, 0, 8 * sizeof(std::uint32_t));
//
//	bool sequences_available = !*reinterpret_cast<int*>(*reinterpret_cast<std::uintptr_t*>(hdr) + 0x150) || *reinterpret_cast<int*>(*reinterpret_cast<std::uintptr_t*>(hdr) + 0x4);
//
//	if (ik_context) {
//		ik_context->init(hdr, &angles, &origin, time, TIME_TO_TICKS(time), mask);
//
//		if (sequences_available)
//			get_skeleton(position, q);
//
//		animating->update_ik_locks(time);
//		ik_context->update_targets(position, q, matrix, (std::uint8_t*)bone_computed);
//		animating->calculate_ik_locks(time);
//		ik_context->solve_dependencies(position, q, matrix, (std::uint8_t*)bone_computed);
//	} else if (sequences_available)
//		get_skeleton(position, q);
//
//	matrix3x4_t transform{};
//	math::angle_matrix(angles, origin, transform);
//
//	studio_build_matrices(hdr, transform, position, q, mask, matrix, bone_computed);
//
//	if (mask & 0x00000200 && attachments)
//		animating->attachment_helper();
//
//	animating->last_setup_bone_time() = time;
//
//	animating->bone_accessor()->m_readable_bones |= mask;
//	animating->bone_accessor()->m_writable_bones |= mask;
//
//	animating->invalidate_bone_cache(interfaces::global_vars->m_framecount);
//
//	clamp_bones_in_bbox(animating, matrix, mask, time, animating->eye_angles());
//}