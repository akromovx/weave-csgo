/*
	This file was generated automatically at: 11.10.2023 00:53:14
	Hash: 49B03E8
*/

#pragma once
#include <cstdint>

namespace disp {
	inline constexpr uint16_t engine_build_number = 0x3639;
	namespace patterns {
		inline constexpr uint32_t direct_device = 0x30A11;
		inline constexpr uint32_t client_state = 0xBA43C;
		inline constexpr uint32_t glow_manager = 0x2C8F69;
		inline constexpr uint32_t global_vars = 0x260937;
		inline constexpr uint32_t move_helper = 0x2BF085;
		inline constexpr uint32_t prediction_random_seed = 0x343C2E;
		inline constexpr uint32_t prediction_player = 0x343C44;
		inline constexpr uint32_t is_load_out_allowed = 0x412383;
		inline constexpr uint32_t return_to_extrapolation = 0x1E9BD5;
		inline constexpr uint32_t do_extra_bone_processing = 0x3E8EA0;
		inline constexpr uint32_t build_transformations = 0x3E96E0;
		inline constexpr uint32_t setup_bones = 0x1D3140;
		inline constexpr uint32_t standard_blending_rules = 0x1D1780;
		inline constexpr uint32_t accum_layers = 0x3E84C6;
		inline constexpr uint32_t sequence_activity = 0x1D94A0;
		inline constexpr uint32_t init_key_values = 0x9999D0;
		inline constexpr uint32_t load_from_buffer = 0x99BCF0;
		inline constexpr uint32_t set_abs_origin = 0x1EA790;
		inline constexpr uint32_t set_abs_angles = 0x1EA950;
		inline constexpr uint32_t update_clientside_animations = 0x3E7A80;
		inline constexpr uint32_t clamp_bones_in_bbox = 0x432E20;
		inline constexpr uint32_t get_server_edict = 0x249887;
		inline constexpr uint32_t server_hitbox = 0x1D23D0;
		inline constexpr uint32_t attachment_helper = 0x1D1BA0;
		inline constexpr uint32_t frame_stage_notify = 0x262B20;
		inline constexpr uint32_t cl_move = 0xDD290;
		inline constexpr uint32_t screen_matrix = 0x220E78;
		inline constexpr uint32_t invalidate_bone_cache = 0x1D3F90;
		inline constexpr uint32_t invalidate_physics_recursive = 0x1AEC40;
		inline constexpr uint32_t studio_hdr = 0x1D34D7;
		inline constexpr uint32_t host_shutdown = 0x22EA90;
		inline constexpr uint32_t hud_ptr = 0x27746A;
		inline constexpr uint32_t find_hud_element = 0x2D3A40;
		inline constexpr uint32_t game_rules = 0x3FA396;
		inline constexpr uint32_t input = 0x260BBC;
		inline constexpr uint32_t smoke_count = 0x4003B3;
		inline constexpr uint32_t filter_simple = 0x1B0A80;
		inline constexpr uint32_t should_skip_anim_frame = 0x1D3050;
		inline constexpr uint32_t send_netmsg = 0x256910;
		inline constexpr uint32_t update_all_viewmodel_addons = 0x215080;
		inline constexpr uint32_t return_to_maintain_sequence_transitions = 0x1D123C;
		inline constexpr uint32_t write_user_cmd = 0x375D90;
		inline constexpr uint32_t alloc_key_values_engine = 0xB92FA;
		inline constexpr uint32_t alloc_key_values_client = 0x26714E;
		inline constexpr uint32_t calc_viewmodel_bob = 0x1B56B0;
		inline constexpr uint32_t modify_eye_position = 0x43EEA0;
		inline constexpr uint32_t physics_simulate = 0x20E2B0;
		inline constexpr uint32_t list_leaves = 0x271048;
		inline constexpr uint32_t trace_to_studio_csgo_hitgroups_priority = 0x7DF220;
		inline constexpr uint32_t model_renderable_animating = 0x1CE0B4;
		inline constexpr uint16_t get_color_modulation = 0xCF90;
		inline constexpr uint32_t setup_per_instance_color_modulation = 0x31D410;
		inline constexpr uint32_t is_using_static_prop_debug_modes = 0x28E4F0;
		inline constexpr uint32_t read_packets = 0xD99C0;
		inline constexpr uint32_t apply_abs_velocity_impulse = 0x1B04C0;
		inline constexpr uint32_t pre_think = 0x20DBD0;
		inline constexpr uint32_t think = 0x1AC780;
		inline constexpr uint32_t think_unk = 0x1AE300;
		inline constexpr uint32_t simulate_player_simulated_entities = 0x1B3B00;
		inline constexpr uint32_t post_think_vphysics = 0x1B6B50;
		inline constexpr uint32_t construct_voice_data_message = 0x19F550;
		inline constexpr uint32_t svc_msg_voice_data = 0x2866A0;
		inline constexpr uint32_t interpolate = 0x213390;
		inline constexpr uint32_t local_player = 0x1E2DF1;
		inline constexpr uint32_t lookup_bone = 0x1CF8F0;
		inline constexpr uint32_t clear_hud_weapon = 0x55A5A0;
		inline constexpr uint32_t get_pose_parameter = 0x36B690;
		inline constexpr uint32_t get_player_viewmodel_arm_config_for_player_model = 0x3E5F1A;
		inline constexpr uint32_t load_named_sky = 0x137780;
		inline constexpr uint32_t fire_bullet = 0x42DBD0;
		inline constexpr uint32_t weapon_shootpos = 0x42B900;
		inline constexpr uint32_t add_viewmodel_bob = 0x6A50A0;
		inline constexpr uint32_t beams = 0x38B8BA;
		inline constexpr uint32_t set_collision_bounds = 0x28CC90;
		inline constexpr uint32_t item_schema = 0x4071E3;
		inline constexpr uint32_t add_renderable = 0x270460;
		inline constexpr uint32_t on_bbox_change_callback = 0x432580;
		inline constexpr uint32_t calc_viewmodel_view = 0x1B7F80;
		inline constexpr uint32_t get_exposure_range = 0x398F90;
		namespace animstate {
			inline constexpr uint32_t reset = 0x43E550;
			inline constexpr uint32_t create = 0x43E440;
			inline constexpr uint32_t update = 0x43E9E0;
		} // namespace animstate
	}	  // namespace patterns
} // namespace disp
