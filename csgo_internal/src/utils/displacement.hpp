#pragma once
#include "../../sdk/datamap.hpp"
#include "../dump.hpp"
#include "utils.hpp"

#define OFFSET(name, type, offset)                                                 \
	__forceinline type name() {                                                    \
		return *(std::remove_reference_t<type>*)((uintptr_t)this + XOR32(offset)); \
	}

#define OFFSET_PTR(name, type, offset)                   \
	__forceinline type* name() {                         \
		return (type*)((uintptr_t)this + XOR32(offset)); \
	}

#define NETVAR(name, type, table, prop)                                                         \
	__forceinline type name() {                                                                 \
		static uintptr_t offset = netvars->get_offset(XOR32S(HASH(table)), XOR32S(HASH(prop))); \
		return *(std::remove_reference_t<type>*)((uintptr_t)this + offset);                     \
	}

#define NETVAR_PTR(name, type, table, prop)                                                     \
	__forceinline type* name() {                                                                \
		static uintptr_t offset = netvars->get_offset(XOR32S(HASH(table)), XOR32S(HASH(prop))); \
		return (type*)((uintptr_t)this + offset);                                               \
	}

#define NETVAR_OFFSET(name, type, table, prop, offset)                                                           \
	__forceinline type name() {                                                                                  \
		static uintptr_t _offset = netvars->get_offset(XOR32S(HASH(table)), XOR32S(HASH(prop))) + XOR32(offset); \
		return *(std::remove_reference_t<type>*)((uintptr_t)this + _offset);                                     \
	}

#define NETVAR_OFFSET_PTR(name, type, table, prop, offset)                                                       \
	__forceinline type* name() {                                                                                 \
		static uintptr_t _offset = netvars->get_offset(XOR32S(HASH(table)), XOR32S(HASH(prop))) + XOR32(offset); \
		return (std::remove_reference_t<type>*)((uintptr_t)this + _offset);                                      \
	}

#define DATAMAP(func_name, type, name)                                                           \
	__forceinline type func_name() {                                                             \
		static uintptr_t offset = find_in_datamap(this->get_pred_descmap(), XOR32S(HASH(name))); \
		return *(std::remove_reference_t<type>*)((uintptr_t)this + offset);                      \
	}

#define VFUNC(name, type, index, ...)                                            \
	__forceinline decltype(auto) name {                                          \
		using fn_t = type;                                                       \
		return (*address_t(this).as<fn_t**>())[XOR32(index)](this, __VA_ARGS__); \
	}

#define PATTERN(name, type, address, ...)    \
	__forceinline decltype(auto) name {      \
		static auto fn = address.as<type>(); \
		return fn(this, __VA_ARGS__);        \
	}

#define TYPE_PATTERN inline address_t

namespace patterns {
	TYPE_PATTERN local_player{};
	TYPE_PATTERN direct_device{};
	TYPE_PATTERN client_state{};
	TYPE_PATTERN game_rules{};
	TYPE_PATTERN global_vars{};
	TYPE_PATTERN move_helper{};
	TYPE_PATTERN glow_manager{};
	TYPE_PATTERN prediction_random_seed{};
	TYPE_PATTERN prediction_player{};
	TYPE_PATTERN is_load_out_allowed{};
	TYPE_PATTERN steam_client{};
	TYPE_PATTERN setup_bones{};
	TYPE_PATTERN do_extra_bone_processing{};
	TYPE_PATTERN build_transformations{};
	TYPE_PATTERN standard_blending_rules{};
	TYPE_PATTERN return_to_extrapolation{};
	TYPE_PATTERN update_clientside_animations{};
	TYPE_PATTERN set_abs_origin{};
	TYPE_PATTERN set_abs_angles{};

	TYPE_PATTERN cl_move{};
	TYPE_PATTERN read_packets{};
	TYPE_PATTERN frame_stage_notify{};
	TYPE_PATTERN input{};

	TYPE_PATTERN accum_layers{};
	TYPE_PATTERN get_server_edict{};
	TYPE_PATTERN server_hitbox{};
	TYPE_PATTERN sequence_activity{};
	TYPE_PATTERN invalidate_bone_cache{};
	TYPE_PATTERN invalidate_physics_recursive{};

	TYPE_PATTERN studio_hdr{};
	TYPE_PATTERN attachment_helper{};
	TYPE_PATTERN clamp_bones_in_bbox{};
	TYPE_PATTERN init_key_values{};
	TYPE_PATTERN load_from_buffer{};

	TYPE_PATTERN hud_ptr{};
	TYPE_PATTERN find_hud_element{};
	TYPE_PATTERN smoke_count{};
	TYPE_PATTERN filter_simple{};
	TYPE_PATTERN send_netmsg{};
	TYPE_PATTERN should_skip_anim_frame{};
	TYPE_PATTERN update_all_viewmodel_addons{};
	TYPE_PATTERN return_to_maintain_sequence_transitions{};

	TYPE_PATTERN write_user_cmd{};
	TYPE_PATTERN alloc_key_values_engine{};
	TYPE_PATTERN alloc_key_values_client{};
	TYPE_PATTERN calc_viewmodel_bob{};
	TYPE_PATTERN modify_eye_position{};
	TYPE_PATTERN physics_simulate{};
	TYPE_PATTERN list_leaves{};
	TYPE_PATTERN trace_to_studio_csgo_hitgroups_priority{};

	TYPE_PATTERN model_renderable_animating{};
	TYPE_PATTERN get_color_modulation{};
	TYPE_PATTERN setup_per_instance_color_modulation{};
	TYPE_PATTERN is_using_static_prop_debug_modes{};

	TYPE_PATTERN apply_abs_velocity_impulse{};
	TYPE_PATTERN pre_think{};
	TYPE_PATTERN think{};
	TYPE_PATTERN think_unk{};
	TYPE_PATTERN simulate_player_simulated_entities{};
	TYPE_PATTERN post_think_vphysics{};

	TYPE_PATTERN construct_voice_data_message{};
	TYPE_PATTERN destruct_voice_data_message{};
	TYPE_PATTERN svc_msg_voice_data{};
	TYPE_PATTERN interpolate{};

	TYPE_PATTERN lookup_bone{};
	TYPE_PATTERN clear_hud_weapon{};

	TYPE_PATTERN get_pose_parameter{};
	TYPE_PATTERN get_player_viewmodel_arm_config_for_player_model{};
	TYPE_PATTERN load_named_sky{};
	TYPE_PATTERN fire_bullet{};
	TYPE_PATTERN weapon_shootpos{};
	TYPE_PATTERN add_viewmodel_bob{};
	TYPE_PATTERN beams{};

	TYPE_PATTERN set_collision_bounds{};
	TYPE_PATTERN item_schema{};
	TYPE_PATTERN add_renderable{};
	TYPE_PATTERN on_bbox_change_callback{};

	TYPE_PATTERN calc_viewmodel_view{};
	TYPE_PATTERN get_exposure_range{};

	namespace animstate {
		TYPE_PATTERN reset{};
		TYPE_PATTERN create{};
		TYPE_PATTERN update{};
		TYPE_PATTERN setup_velocity{};
		TYPE_PATTERN setup_movement{};
		TYPE_PATTERN setup_aim_matrix{};
	} // namespace animstate

	namespace ik_context {
		TYPE_PATTERN constructor{};
		TYPE_PATTERN destructor{};
		TYPE_PATTERN init{};
		TYPE_PATTERN update_targets{};
		TYPE_PATTERN solve_dependencies{};
	} // namespace ik_context

	namespace setup_bones_context {
		TYPE_PATTERN init_pose{};
		TYPE_PATTERN calc_autoplay_sequences{};
		TYPE_PATTERN calc_bone_adj{};
		TYPE_PATTERN accumulate_pose{};
	} // namespace setup_bones_context

	TYPE_PATTERN screen_matrix{};
	TYPE_PATTERN host_shutdown{};

	extern void init();
} // namespace patterns