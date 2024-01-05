#include "displacement.hpp"
#include "../dump.hpp"
#include "utils.hpp"

#define CALC_OFFSET(name, mod) name = mod + XOR32S(disp::patterns::name)

namespace patterns {
	CHEAT_INIT void init() {
		auto shaderapidx9 = (uintptr_t)GetModuleHandleA(STRSC("shaderapidx9.dll"));
		auto engine = (uintptr_t)GetModuleHandleA(STRSC("engine.dll"));
		auto client = (uintptr_t)GetModuleHandleA(STRSC("client.dll"));
		auto materialsystem = (uintptr_t)GetModuleHandleA(STRSC("materialsystem.dll"));

		/* misc modules */

		CALC_OFFSET(direct_device, shaderapidx9);
		CALC_OFFSET(get_color_modulation, materialsystem);

		/* engine.dll */

		CALC_OFFSET(client_state, engine);
		CALC_OFFSET(cl_move, engine);
		CALC_OFFSET(read_packets, engine);
		CALC_OFFSET(send_netmsg, engine);
		CALC_OFFSET(alloc_key_values_engine, engine);
		CALC_OFFSET(is_using_static_prop_debug_modes, engine);
		CALC_OFFSET(construct_voice_data_message, engine);
		CALC_OFFSET(svc_msg_voice_data, engine);
		CALC_OFFSET(load_named_sky, engine);

		/* client.dll */

		CALC_OFFSET(local_player, client);
		CALC_OFFSET(lookup_bone, client);
		CALC_OFFSET(hud_ptr, client);
		CALC_OFFSET(find_hud_element, client);
		CALC_OFFSET(clear_hud_weapon, client);
		CALC_OFFSET(glow_manager, client);
		CALC_OFFSET(global_vars, client);
		CALC_OFFSET(move_helper, client);
		CALC_OFFSET(prediction_random_seed, client);
		CALC_OFFSET(prediction_player, client);
		CALC_OFFSET(is_load_out_allowed, client);
		CALC_OFFSET(return_to_extrapolation, client);
		CALC_OFFSET(do_extra_bone_processing, client);
		CALC_OFFSET(build_transformations, client);
		CALC_OFFSET(setup_bones, client);
		CALC_OFFSET(standard_blending_rules, client);
		CALC_OFFSET(accum_layers, client);
		CALC_OFFSET(sequence_activity, client);
		CALC_OFFSET(init_key_values, client);
		CALC_OFFSET(load_from_buffer, client);
		CALC_OFFSET(set_abs_origin, client);
		CALC_OFFSET(set_abs_angles, client);
		CALC_OFFSET(update_clientside_animations, client);
		CALC_OFFSET(clamp_bones_in_bbox, client);
		CALC_OFFSET(get_server_edict, client);
		CALC_OFFSET(server_hitbox, client);
		CALC_OFFSET(attachment_helper, client);
		CALC_OFFSET(frame_stage_notify, client);
		CALC_OFFSET(screen_matrix, client);
		CALC_OFFSET(invalidate_bone_cache, client);
		CALC_OFFSET(invalidate_physics_recursive, client);
		CALC_OFFSET(studio_hdr, client);
		CALC_OFFSET(host_shutdown, client);
		CALC_OFFSET(game_rules, client);
		CALC_OFFSET(input, client);
		CALC_OFFSET(smoke_count, client);
		CALC_OFFSET(filter_simple, client);
		CALC_OFFSET(should_skip_anim_frame, client);
		CALC_OFFSET(update_all_viewmodel_addons, client);
		CALC_OFFSET(return_to_maintain_sequence_transitions, client);
		CALC_OFFSET(write_user_cmd, client);
		CALC_OFFSET(alloc_key_values_client, client);
		CALC_OFFSET(calc_viewmodel_bob, client);
		CALC_OFFSET(modify_eye_position, client);
		CALC_OFFSET(physics_simulate, client);
		CALC_OFFSET(list_leaves, client);
		CALC_OFFSET(trace_to_studio_csgo_hitgroups_priority, client);
		CALC_OFFSET(model_renderable_animating, client);
		CALC_OFFSET(setup_per_instance_color_modulation, client);
		CALC_OFFSET(apply_abs_velocity_impulse, client);
		CALC_OFFSET(pre_think, client);
		CALC_OFFSET(think, client);
		CALC_OFFSET(think_unk, client);
		CALC_OFFSET(simulate_player_simulated_entities, client);
		CALC_OFFSET(post_think_vphysics, client);
		CALC_OFFSET(interpolate, client);
		CALC_OFFSET(get_pose_parameter, client);
		CALC_OFFSET(get_player_viewmodel_arm_config_for_player_model, client);
		CALC_OFFSET(fire_bullet, client);
		CALC_OFFSET(weapon_shootpos, client);
		CALC_OFFSET(add_viewmodel_bob, client);
		CALC_OFFSET(beams, client);
		CALC_OFFSET(set_collision_bounds, client);
		CALC_OFFSET(item_schema, client);
		CALC_OFFSET(add_renderable, client);
		CALC_OFFSET(on_bbox_change_callback, client);
		CALC_OFFSET(calc_viewmodel_view, client);
		CALC_OFFSET(get_exposure_range, client);

		CALC_OFFSET(animstate::reset, client);
		CALC_OFFSET(animstate::create, client);
		CALC_OFFSET(animstate::update, client);
	}
} // namespace patterns