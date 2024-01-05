#include "hooker.hpp"
#include "hooks.hpp"

#include "../features/hvh/exploits.hpp"
#include "../game/players.hpp"
#include "../globals.hpp"
#include "../interfaces.hpp"

namespace hooks::player {
	using namespace sdk;

	bool __fastcall should_skip_anim_frame(void* ecx, void* edx) {
		static auto original = detour->original(&should_skip_anim_frame);

		auto player = (cs_player_t*)ecx;
		if (player != nullptr && player == globals->m_local)
			return false;
		else
			return original(ecx, edx);
	}

	void __fastcall clamp_bones_in_bbox(sdk::cs_player_t* player, void* edx, matrix3x4_t* bones, int mask) {
		static auto original = detour->original(&clamp_bones_in_bbox);

		if (player == nullptr)
			return original(player, edx, bones, mask);

		// manually force this var to true and call this shit just after setup_bones like server does
		if (!players->is_clamping(player))
			return;

		original(player, edx, bones, mask);
		return;
	}

	void __fastcall server_clamp_bones_in_bbox(sdk::cs_player_t* player, void* edx, matrix3x4_t* bones, int mask) {
		static auto original = detour->original(&server_clamp_bones_in_bbox);
		return;
		//original(player, edx, bones, mask);
	}

	bool __fastcall setup_bones(void* ecx, void* edx, matrix3x4_t* bones, int max_bones, int mask, float curtime) {
		static auto original = detour->original(&setup_bones);

		if (ctx->unload || !ctx->is_valid())
			return original(ecx, edx, bones, max_bones, mask, curtime);

		auto entity = (cs_player_t*)((uintptr_t)ecx - 0x4);
		if (entity == nullptr || globals->m_local == nullptr || !entity->is_player()
			/*|| (entity->is_teammate() && entity != globals->m_local)*/)
			return original(ecx, edx, bones, max_bones, mask, curtime);

		if (!players->is_setuping(entity)) {
			if (bones != nullptr)
				utils::memcpy_sse(bones, entity->bone_cache().base(), sizeof(matrix3x4_t) * max_bones);

			return true;
		} else
			return original(ecx, edx, bones, max_bones, mask, curtime);
	}

	void __fastcall do_extra_bone_processing(base_player_t* player, uint32_t, void* hdr, vec3d* pos, void* q,
											 const matrix3x4_t& mat, uint8_t* bone_computed, void* context) {
		return;
	}

	void __fastcall build_transformations(void* this_pointer, void* edx, void* hdr, void* pos, void* q, const void* camera_transform, int bone_mask, void* bone_computed) {
		static auto original = detour->original(&build_transformations);
		const auto player = (cs_player_t*)this_pointer;

		if (player == nullptr || !player->is_player() || !player->is_alive())
			return original(this_pointer, edx, hdr, pos, q, camera_transform, bone_mask, bone_computed);

		SET_AND_RESTORE(player->jiggle_bones(), false);
		SET_AND_RESTORE(player->use_new_animstate(), false);
		SET_AND_RESTORE(*(int*)((std::uintptr_t)player + 0x26B0), 0);
		original(this_pointer, edx, hdr, pos, q, camera_transform, bone_mask, bone_computed);
	}

	void __fastcall standard_blending_rules(base_entity_t* this_pointer, void* edx, void* hdr, void* pos, void* q, float current_time, int bone_mask) {

		static auto original = detour->original(&standard_blending_rules);

		if (!this_pointer->is_player() || !ctx->is_valid())
			return original(this_pointer, edx, hdr, pos, q, current_time, bone_mask);

		auto& effects = ((cs_player_t*)this_pointer)->effects();
		effects.add(XOR32(8));
		original(this_pointer, edx, hdr, pos, q, current_time, bone_mask);
		effects.remove(XOR32(8));
	}

	void __fastcall update_clientside_animations(cs_player_t* player, void* edx) {
		static auto original = detour->original(&update_clientside_animations);

		if (!ctx->is_valid())
			return;

		if (player == nullptr || !player->is_alive() || globals->m_local == nullptr || !player->is_player())
			return original(player, edx);

		if (player == globals->m_local) {
			if (!players->is_updating(player)) {
				float poses[24]{};
				animation_layers_t layers{};

				player->store_poses(poses);
				player->store_layers(layers);

				player->apply_poses(players->m_local_player.m_poses);
				player->apply_layers(players->m_local_player.m_layers);

				auto state = player->animstate();
				if (state != nullptr) {
					state->m_goal_feet_yaw = players->m_local_player.m_last_goal_feet_yaw;
					player->set_abs_angles(vec3d{ 0.f, state->m_goal_feet_yaw, 0.f });

					math::change_bones_position(players->m_local_player.m_bones, sdk::max_bones, vec3d{ 0.f, 0.f, 0.f }, player->render_origin());
					player->set_bone_cache(players->m_local_player.m_bones);
					//player->last_setup_bone_time() = interfaces::global_vars->m_curtime;

					utils::memcpy_sse(player->bone_accessor()->m_bones, players->m_local_player.m_bones, sizeof(matrix3x4_t) * player->bone_cache().count());

					player->attachment_helper();
					math::change_bones_position(players->m_local_player.m_bones, sdk::max_bones, player->render_origin(), vec3d{ 0.f, 0.f, 0.f });
				}

				player->apply_poses(poses);
				player->apply_layers(layers);
				return;
			}
		} else {
			// prevent update animations when is not updating
			if (!players->is_updating(player)) {
				const auto visual_animation =
						globals->m_local != nullptr && globals->m_local->is_alive()
								? players->find_record_if(player, [](lag_record_t* record) { return record->m_valid_visual && record->m_valid; })
								: players->get_last_record(player);

				if (visual_animation == nullptr || visual_animation->m_player != player) {
					players->is_setuping(player) = true;
					original(player, edx);
					return;
				}

				static matrix3x4_t interp_matrix[sdk::max_bones]{};
				utils::memcpy_sse(interp_matrix, visual_animation->m_visual_bones, sizeof(matrix3x4_t) * sdk::max_bones);
				math::change_bones_position(interp_matrix, sdk::max_bones, visual_animation->m_origin, player->render_origin());
				player->set_bone_cache(interp_matrix);

				utils::memcpy_sse(player->bone_accessor()->m_bones, interp_matrix, sizeof(matrix3x4_t) * player->bone_cache().count());

				player->attachment_helper();
				return;
			}
		}

		return original(player, edx);
	}

	void __fastcall modify_eye_position(animation_state_t* state, void* edx, const vec3d& position) {
		static auto original = detour->original(&modify_eye_position);

		if (state->m_player == nullptr || globals->m_local != state->m_player || !ctx->is_valid())
			return original(state, edx, position);
	}

	void __fastcall physics_simulate(cs_player_t* player, void* edx) {
		static auto original = detour->original(&physics_simulate);
		auto& cmd_ctx = player->command_context();
		// hvh::exploits->fix_tickbase(cmd_ctx.m_command_number);
		original(player, edx);
	}

	void __fastcall calc_viewmodel_bob(void* ecx, void* edx, vec3d& position) {
		static auto original = detour->original(&calc_viewmodel_bob);
		return original(ecx, edx, position);
	}

	void __fastcall calc_viewmodel_view(void* ecx, void* edx, sdk::cs_player_t* owner, vec3d& eye_position, vec3d& eye_angles) {
		static auto original = detour->original(&calc_viewmodel_view);

		if (!globals->m_local_alive || owner == nullptr || globals->m_local == nullptr || owner->index() != globals->m_local->index())
			return original(ecx, edx, owner, eye_position, eye_angles);

		if (globals->m_fake_duck)
			eye_position = owner->render_origin() + vec3d{ 0.0f, 0.0f, interfaces::game_movement->get_player_view_offset(false).z + 0.064f };

		return original(ecx, edx, owner, eye_position, eye_angles);
	}

	void __fastcall add_renderable(void* ecx, void* edx, renderable_t* renderable, bool render_with_viewmodels, int type, int model_type, int split_screen_enables) {
		static auto original = detour->original(&add_renderable);

		auto renderable_addr = (uintptr_t)renderable;
		if (!renderable_addr || *(uintptr_t*)renderable == 0 || renderable_addr == 0x4)
			return original(ecx, edx, renderable, render_with_viewmodels, type, model_type, split_screen_enables);

		auto entity = (base_entity_t*)(renderable_addr - 0x4);
		int index = *(int*)((uintptr_t)entity + 0x64);

		if (index < 1 || index > 64)
			return original(ecx, edx, renderable, render_with_viewmodels, type, model_type, split_screen_enables);

		// set advanced transparency type for fixing z order (chams renders behind props)
		type = index == interfaces::engine->get_local_player() ? 1 : 2;

		original(ecx, edx, renderable, render_with_viewmodels, type, model_type, split_screen_enables);
	}

	void __fastcall on_bbox_change_callback(cs_player_t* player, void* edx, vec3d* old_mins, vec3d* new_mins, vec3d* old_maxs, vec3d* new_maxs) {
		static auto original = detour->original(&on_bbox_change_callback);

		//if (player == nullptr)
		//	return original(player, edx, old_mins, new_mins, old_maxs, new_maxs);

		//if (!players->is_clamping(player))
		//	return;

		//original(player, edx, old_mins, new_mins, old_maxs, new_maxs);
		player->collision_change_height() = player->origin().z + old_maxs->z;
		player->collision_change_time() = interfaces::global_vars->m_curtime;
	}
} // namespace hooks::player