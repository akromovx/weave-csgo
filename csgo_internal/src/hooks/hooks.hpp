#pragma once
#include "../../sdk/engine_client.hpp"
#include "../../sdk/engine_sound.hpp"
#include "../../sdk/entities.hpp"
#include "../../sdk/game_movement.hpp"
#include "../../sdk/input.hpp"
#include "../../sdk/material_system.hpp"
#include "../../sdk/net_channel.hpp"
#include "../../sdk/prediction.hpp"
#include "../utils/vmt.hpp"

namespace sdk {
	struct client_state_t;
	struct view_setup_t;

	struct draw_model_state_t;
	struct model_render_info_t;

	struct move_helper_t;
}// namespace sdk

#define PUSH_VMT(name) inline auto name = std::make_unique<utils::vmt_t>()

namespace hooks {
	namespace vmt {
		PUSH_VMT(engine);
		PUSH_VMT(client);
		PUSH_VMT(client_mode);
		PUSH_VMT(prediction);
		PUSH_VMT(vpanel);
		PUSH_VMT(client_state);
		PUSH_VMT(model_render);
		PUSH_VMT(directx);
		PUSH_VMT(shadows);
		PUSH_VMT(movement);
		PUSH_VMT(query);
		PUSH_VMT(traces);
		PUSH_VMT(studio_render);
		PUSH_VMT(sounds);
	}// namespace vmt

	extern void init();
	extern void unload();

	inline uint32_t old_wnd_proc = 0u;
	extern LRESULT WINAPI wnd_proc(HWND, UINT, WPARAM, LPARAM);

	namespace engine {
		void __vectorcall cl_move(float, bool);
		void __vectorcall read_packets(bool final_tick);

		bool __fastcall is_connected(void*);
		bool __fastcall is_hltv(void*, void*);
		bool __fastcall is_paused(void*, void*);
	}// namespace engine

	namespace directx {
		HRESULT __stdcall end_scene(IDirect3DDevice9*);

		HRESULT __stdcall reset(IDirect3DDevice9*, D3DPRESENT_PARAMETERS*);
	}// namespace directx

	namespace client {
		void __stdcall frame_stage_notify(sdk::e_client_frame_stage);
		void __stdcall create_move(int, float, bool, bool&);
		bool __fastcall write_user_cmd_delta_to_buffer(void* ecx, void*, int slot, sdk::bf_write* buf, int from, int to, bool isnewcommand);
		void __fastcall create_move_proxy(void*, int, int, float, bool);
	}// namespace client

	namespace client_mode {
		bool __fastcall should_draw_fog(void*, void*);
		void __stdcall override_view(sdk::view_setup_t*);
		bool __stdcall create_move(float, sdk::user_cmd_t*);
		float __stdcall get_viewmodel_fov();
		bool __fastcall do_post_screen_effects(void*, void*, sdk::view_setup_t*);
	}// namespace client_mode

	namespace v_panel {
		enum panels_t {
			focus_overlay_panel = 0,
			mat_sys_top_panel,
			hud_zoom,
		};

		void __stdcall paint_traverse(unsigned int, bool, bool);
	}// namespace v_panel

	namespace player {
		bool __fastcall setup_bones(void*, void*, matrix3x4_t*, int, int, float);

		void __fastcall do_extra_bone_processing(sdk::base_player_t*, uint32_t, void*, vec3d*, void*,
												 const matrix3x4_t&, uint8_t*, void*);

		void __fastcall build_transformations(void*, void*, void*, void*, void*, const void*, int, void*);
		void __fastcall standard_blending_rules(sdk::base_entity_t*, void*, void*, void*, void*, float, int);
		void __fastcall update_clientside_animations(sdk::cs_player_t*, void*);
		void __fastcall clamp_bones_in_bbox(sdk::cs_player_t*, void*, matrix3x4_t*, int);
		void __fastcall server_clamp_bones_in_bbox(sdk::cs_player_t*, void*, matrix3x4_t*, int);
		bool __fastcall should_skip_anim_frame(void* ecx, void* edx);
		void __fastcall calc_viewmodel_bob(void*, void*, vec3d& position);
		void __fastcall calc_viewmodel_view(void* ecx, void* edx, sdk::cs_player_t* owner, vec3d& eye_position, vec3d& eye_angles);
		void __fastcall modify_eye_position(sdk::animation_state_t* state, void* edx, const vec3d& position);
		void __fastcall physics_simulate(sdk::cs_player_t* player, void* edx);
		void __fastcall add_renderable(void* ecx, void* edx, sdk::renderable_t* renderable, bool render_with_viewmodels, int type, int model_type, int split_screen_enables);
		void __fastcall on_bbox_change_callback(sdk::cs_player_t* ecx, void* edx, vec3d* old_mins, vec3d* new_mins, vec3d* old_maxs, vec3d* new_maxs);
	}// namespace player

	namespace client_state {
		bool __fastcall send_netmsg(sdk::net_channel_t* channel, uint32_t edx, sdk::net_message_t* message, bool reliable, bool voice);
		void __fastcall packet_start(sdk::client_state_t*, uint32_t, int32_t, int32_t);
		void __fastcall packet_end(sdk::client_state_t*, uint32_t);
	}// namespace client_state

	namespace model_render {
		void __fastcall draw_model_execute(void*, void*, void*, const sdk::draw_model_state_t&, const sdk::model_render_info_t&, matrix3x4_t*);
	}

	namespace prediction {
		bool __fastcall in_prediction(sdk::prediction_t* prediction, uint32_t);
		void __fastcall run_command(void*, void*, sdk::cs_player_t*, sdk::user_cmd_t*, sdk::move_helper_t*);
	}// namespace prediction

	namespace movement {
		void __fastcall process_movement(void* ecx, void* edx, sdk::cs_player_t* ent, sdk::move_data_t* move_data);
	}

	namespace misc {
		bool __fastcall should_draw_shadow(void*, void*);
		void __fastcall fire_bullet(sdk::cs_player_t* ecx, void* edx, vec3d eye_position,
									const vec3d& shoot_angles, float flDistance, float flPenetration,
									int penetration_count, int bullet_type, int damage,
									float range_modifier, sdk::base_entity_t* pev_attacker,
									bool do_effects, float x_spread, float y_spread);

		vec3d* __fastcall weapon_shootpos(sdk::cs_player_t* ecx, void* edx, vec3d& eye);
		void __fastcall add_viewmodel_bob(void* ecx, void* edx, sdk::base_viewmodel_t* model, vec3d& origin, vec3d& angles);

		int __fastcall list_leaves_in_box(void* bsp, void* edx, vec3d& mins, vec3d& maxs, uint16_t* list, int list_max);
		void __fastcall clip_ray_to_collideable(void* ecx, void* edx, const sdk::ray_t& ray, unsigned int mask, sdk::collideable_t* collideable, sdk::game_trace_t* trace);

		void __fastcall begin_frame(void* thisptr);

		void __fastcall draw_model_array(void* ecx, void* edx,
										 const sdk::studio_model_array_info2_t& info, int count, sdk::studio_array_data_t* array_data, int stride, int flags);

		void* __fastcall model_renderable_animating(void* ecx, void* edx);
		void __fastcall get_color_modulation(void* ecx, void* edx, float* r, float* g, float* b);
		bool __fastcall is_using_static_prop_debug_modes(void* ecx, void* edx);
		void __fastcall setup_per_instance_color_modulation(void* ecx, void* edx, int cnt, sdk::model_list_by_type_t* list);
		bool __fastcall svc_msg_voice_data(void* ecx, uint32_t, sdk::svc_msg_voice_data_t* msg);

		bool __fastcall interpolate(void* ecx, void* edx, float time);

		void __fastcall emit_sound(sdk::engine_sound_t* thisptr, uint32_t edx, void* filter, int ent_index, int channel, const char* sound_entry, unsigned int sound_entry_hash,
								   const char* sample, float volume, float attenuation, int seed, int flags, int pitch, const vec3d* origin, const vec3d* direction,
								   void* vec_origins, bool update_positions, float sound_time, int speaker_entity, int test);
	}// namespace misc
}// namespace hooks