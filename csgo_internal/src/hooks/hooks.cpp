#include "hooks.hpp"
#include "hooker.hpp"

#include "../cheat.hpp"
#include "../features/visuals/skin_changer.hpp"
#include "../interfaces.hpp"
#include "../utils/displacement.hpp"

#include <intrin.h>

using namespace sdk;

bool cheat_t::is_valid() const {
	return interfaces::client_state->m_delta_tick != -1;
}

namespace hooks {
	static void host_shutdown() {
		cheat::unload();
		detour->original (&host_shutdown)();
	}

	// https://www.unknowncheats.me/forum/counterstrike-global-offensive/501277-fix-white-chams-office-dust.html
	// client.dll 55 8B EC 51 80 3D ? ? ? ? ? 0F 57
	void __fastcall get_exposure_range(float* min, float* max) {
		static auto original = detour->original(&get_exposure_range);

		*min = 1.f;
		*max = 1.f;

		original(min, max);
	}

	namespace misc {
		static void* __fastcall alloc_key_values_memory(void* thisptr, int edx, int size) {
			static auto original = detour->original(&alloc_key_values_memory);

			// return addresses of check function
			// @credits: danielkrupinski (anko1337 furcore best cheat ever)
			const auto return_address = (uint8_t*)(_ReturnAddress());
			if (return_address == patterns::alloc_key_values_client.as<void*>() || return_address == patterns::alloc_key_values_engine.as<void*>())
				return nullptr;

			return original(thisptr, edx, size);
		}
	} // namespace misc

	namespace proxy {
		static recv_prop_hook_t* sequence_hook = new recv_prop_hook_t{};
		static recv_prop_hook_t* spotted_hook = new recv_prop_hook_t{};
		static recv_prop_hook_t* simulation_time_hook = new recv_prop_hook_t{};

		void _cdecl sequence(const recv_proxy_data_t* data_const, void* p_struct, void* p_out) {
			static auto original = sequence_hook->original();
			const auto proxy_data = (recv_proxy_data_t*)data_const;
			const auto view_model = static_cast<base_viewmodel_t*>(p_struct);

			if (view_model != nullptr && proxy_data != nullptr)
				skin_changer->sequence_remap(proxy_data, view_model);

			original(data_const, p_struct, p_out);
		}

		void spotted(const recv_proxy_data_t* data_const, void* p_struct, void* p_out) {
			static auto original = spotted_hook->original();
			const auto proxy_data = (recv_proxy_data_t*)data_const;

			proxy_data->m_value.m_int = settings->ragebot.enable;
			original(data_const, p_struct, p_out);
		}

		void simulation_time(const recv_proxy_data_t* data_const, void* p_struct, void* p_out) {
			static auto original = simulation_time_hook->original();
			const auto proxy_data = (recv_proxy_data_t*)data_const;

			if (!proxy_data->m_value.m_int)
				return;

			original(data_const, p_struct, p_out);
		}
	} // namespace proxy

	CHEAT_INIT void init() {
		const auto device = (void*)(**patterns::direct_device.as<uintptr_t**>());
		const auto shadows = GET_CVAR("cl_csm_shadows");
		const auto vstdlib = GetModuleHandleA(STRSC("vstdlib"));
		if (vstdlib == NULL)
			return;

		vmt::directx->setup(device);
		vmt::client->setup(interfaces::client);
		vmt::client_mode->setup(interfaces::client_mode);
		vmt::vpanel->setup(interfaces::v_panel);
		vmt::engine->setup(interfaces::engine);
		vmt::client_state->setup((void*)((uintptr_t)interfaces::client_state + 8));
		vmt::model_render->setup(interfaces::model_render);
		vmt::prediction->setup(interfaces::prediction);
		vmt::shadows->setup(shadows);
		vmt::movement->setup(interfaces::game_movement);
		vmt::query->setup(interfaces::engine->get_bsp_tree_query());
		vmt::traces->setup(interfaces::traces);
		vmt::studio_render->setup(interfaces::studio_render);
		vmt::sounds->setup(interfaces::engine_sound);

		vmt::vpanel->hook(XOR32S(41), v_panel::paint_traverse);

		vmt::directx->hook(XOR32S(42), directx::end_scene);
		vmt::directx->hook(XOR32S(16), directx::reset);

		vmt::sounds->hook(XOR32S(5), misc::emit_sound);

		vmt::client_mode->hook(XOR32S(17), client_mode::should_draw_fog);
		vmt::client_mode->hook(XOR32S(18), client_mode::override_view);
		vmt::client_mode->hook(XOR32S(24), client_mode::create_move);
		vmt::client_mode->hook(XOR32S(35), client_mode::get_viewmodel_fov);
		vmt::client_mode->hook(XOR32S(44), client_mode::do_post_screen_effects);

		vmt::client->hook(XOR32S(22), client::create_move_proxy);
		vmt::client->hook(XOR32S(24), client::write_user_cmd_delta_to_buffer);
		vmt::client->hook(XOR32S(37), client::frame_stage_notify);

		vmt::engine->hook(XOR32S(27), engine::is_connected);
		vmt::engine->hook(XOR32S(93), engine::is_hltv);
		vmt::engine->hook(XOR32S(90), engine::is_paused);

		vmt::client_state->hook(XOR32S(5), client_state::packet_start);

		vmt::model_render->hook(XOR32S(21), model_render::draw_model_execute);

		vmt::prediction->hook(XOR32S(14), prediction::in_prediction);
		vmt::prediction->hook(XOR32S(19), prediction::run_command);

		vmt::movement->hook(XOR32S(1), movement::process_movement);

		vmt::studio_render->hook(XOR32S(9), misc::begin_frame);
		vmt::studio_render->hook(XOR32S(48), misc::draw_model_array);

		vmt::shadows->hook(XOR32S(13), misc::should_draw_shadow);
		vmt::query->hook(XOR32S(6), misc::list_leaves_in_box);
		vmt::traces->hook(XOR32S(4), misc::clip_ray_to_collideable);

		detour->hook(engine::cl_move, patterns::cl_move);
		detour->hook(engine::read_packets, patterns::read_packets);

		detour->hook(player::update_clientside_animations, patterns::update_clientside_animations);
		detour->hook(player::setup_bones, patterns::setup_bones);
		detour->hook(player::do_extra_bone_processing, patterns::do_extra_bone_processing);
		detour->hook(player::standard_blending_rules, patterns::standard_blending_rules);
		detour->hook(player::build_transformations, patterns::build_transformations);
		detour->hook(player::clamp_bones_in_bbox, patterns::clamp_bones_in_bbox);

		// detour->hook(player::server_clamp_bones_in_bbox, utils::find_pattern("server.dll", "55 8B EC 83 E4 F8 83 EC 70 56 57 8B F9 89 7C 24 38"));
		detour->hook(player::should_skip_anim_frame, patterns::should_skip_anim_frame);
		detour->hook(player::calc_viewmodel_bob, patterns::calc_viewmodel_bob);
		detour->hook(player::calc_viewmodel_view, patterns::calc_viewmodel_view);
		detour->hook(player::modify_eye_position, patterns::modify_eye_position);
		detour->hook(player::physics_simulate, patterns::physics_simulate);
		detour->hook(player::add_renderable, patterns::add_renderable);
		detour->hook(player::on_bbox_change_callback, patterns::on_bbox_change_callback);

		detour->hook(client_state::send_netmsg, patterns::send_netmsg);

		detour->hook(misc::fire_bullet, patterns::fire_bullet);
		detour->hook(misc::weapon_shootpos, patterns::weapon_shootpos);
		detour->hook(misc::add_viewmodel_bob, patterns::add_viewmodel_bob);

		detour->hook(misc::model_renderable_animating, (void*)(patterns::model_renderable_animating + 4 + *patterns::model_renderable_animating.as<uintptr_t*>()));
		detour->hook(misc::get_color_modulation, patterns::get_color_modulation);
		detour->hook(misc::setup_per_instance_color_modulation, patterns::setup_per_instance_color_modulation);
		detour->hook(misc::is_using_static_prop_debug_modes, patterns::is_using_static_prop_debug_modes);
		detour->hook(misc::svc_msg_voice_data, patterns::svc_msg_voice_data);
		detour->hook(misc::interpolate, patterns::interpolate);
		detour->hook(misc::alloc_key_values_memory, utils::vfunc<void* (*)()>((void*)(GetProcAddress(vstdlib, STRSC("KeyValuesSystem")))(), 2));

		detour->hook(get_exposure_range, patterns::get_exposure_range);

		// detour->hook(host_shutdown, patterns::host_shutdown);

		detour->hook_all();

		proxy::sequence_hook->hook(HASH("CBaseViewModel"), HASH("m_nSequence"), proxy::sequence);
		proxy::spotted_hook->hook(HASH("CBaseEntity"), HASH("m_bSpotted"), proxy::spotted);
		proxy::simulation_time_hook->hook(HASH("CBaseEntity"), HASH("m_flSimulationTime"), proxy::simulation_time);
	}

	CHEAT_INIT void unload() {
		vmt::directx->unhook_all();
		vmt::client_mode->unhook_all();
		vmt::client->unhook_all();
		vmt::vpanel->unhook_all();
		vmt::model_render->unhook_all();
		vmt::prediction->unhook_all();
		vmt::shadows->unhook_all();
		vmt::movement->unhook_all();
		vmt::query->unhook_all();
		vmt::traces->unhook_all();
		vmt::sounds->unhook_all();

		detour->unhook_all();

		delete proxy::sequence_hook;
		delete proxy::spotted_hook;
		delete proxy::simulation_time_hook;
	}
} // namespace hooks