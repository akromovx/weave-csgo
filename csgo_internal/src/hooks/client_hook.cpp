#include "hooker.hpp"
#include "hooks.hpp"

#include "../globals.hpp"
#include "../interfaces.hpp"

#include "../features/hvh/exploits.hpp"
#include "../features/hvh/hvh.hpp"

#include "../features/bullets.hpp"
#include "../features/misc.hpp"
#include "../features/visuals/esp.hpp"
#include "../features/visuals/skin_changer.hpp"

#include "../game/engine_prediction.hpp"
#include "../game/net_data.hpp"
#include "../game/override_entity_list.hpp"

#include "../features/network.hpp"
#include "../utils/animation_handler.hpp"
#include "../utils/hotkeys.hpp"

namespace hooks::client {
	using namespace sdk;

	STFI void write_user_cmd(bf_write* buf, user_cmd_t* cmd_in, user_cmd_t* cmd_out) {
		static auto fn = patterns::write_user_cmd.as<void(__fastcall*)(void*, user_cmd_t*, user_cmd_t*)>();
		__asm {
			mov     ecx, buf
			mov     edx, cmd_in
			push    cmd_out
			call    fn
			add     esp, 4
		}
	}

	STFI void pvs_fix() {
		if (!ctx->is_valid())
			return;

		for (auto& [player, entry]: entities->m_players) {
			if (player == globals->m_local || !player->is_alive() || /*player->is_teammate() ||*/ player->dormant())
				continue;

			*(int*)((uintptr_t)player + 0xA30) = interfaces::global_vars->m_framecount;
			*(int*)((uintptr_t)player + 0xA28) = 0;
		}
	}

	bool __fastcall write_user_cmd_delta_to_buffer(void* ecx, void*, int slot, bf_write* buf, int from, int to, bool isnewcommand) {
		static auto original = vmt::client->original<bool(__thiscall*)(void*, int, void*, int, int, bool)>(XOR32(24));

		if (globals->m_local == nullptr || !globals->m_local_alive || interfaces::game_rules->is_freeze_time() || globals->m_local->flags().has(fl_frozen))
			return original(ecx, slot, buf, from, to, isnewcommand);

		if (hvh::exploits->m_in_shift)
			return original(ecx, slot, buf, from, to, isnewcommand);

		if (hvh::exploits->m_shift_cmd == 0)
			return original(ecx, slot, buf, from, to, isnewcommand);

		if (from != -1)
			return true;

		auto final_from = -1;

		uintptr_t frame_ptr = 0;
		__asm mov frame_ptr, ebp;

		auto backup_commands = (int*)(frame_ptr + XOR32S(0xFD8));
		auto new_commands = (int*)(frame_ptr + XOR32S(0xFDC));

		auto newcmds = *new_commands;
		auto shift = std::clamp(hvh::exploits->m_shift_cmd, 1, CVAR_INT("sv_maxusrcmdprocessticks") - 2);

		hvh::exploits->m_shift_cmd = 0;
		*backup_commands = 0;

		auto choked_modifier = newcmds + shift;

		if (choked_modifier > 62)
			choked_modifier = 62;

		*new_commands = choked_modifier;

		auto next_cmdnr = interfaces::client_state->get_current_tick();
		auto final_to = next_cmdnr - newcmds + 1;

		if (final_to <= next_cmdnr) {
			while (original(ecx, slot, buf, final_from, final_to, true)) {
				final_from = final_to++;

				if (final_to > next_cmdnr)
					goto next_cmd;
			}

			return false;
		}

	next_cmd:
		auto user_cmd = interfaces::input->get_user_cmd(final_from);

		//if (!user_cmd)
		//	return true;

		//user_cmd_t to_cmd;
		//user_cmd_t from_cmd;

		//from_cmd = *user_cmd;
		//to_cmd = from_cmd;

		//to_cmd.m_command_number++;
		//to_cmd.m_tickcount += 200;

		//if (newcmds > choked_modifier)
		//	return true;

		//for (auto i = choked_modifier - newcmds + 1; i > 0; --i) {
		//	write_user_cmd(buf, &to_cmd, &from_cmd);

		//	from_cmd = to_cmd;
		//	to_cmd.m_command_number++;
		//	to_cmd.m_tickcount++;
		//}

		user_cmd_t to_cmd{};
		user_cmd_t from_cmd{};

		from_cmd = *user_cmd;
		to_cmd = from_cmd;

		++to_cmd.m_command_number;
		to_cmd.m_tickcount = INT_MAX;

		do {
			write_user_cmd(buf, &to_cmd, &from_cmd);

			++to_cmd.m_command_number;
			shift--;
		} while (shift > 0);

		hvh::exploits->m_tickbase.m_simulation_ticks = choked_modifier - newcmds + 1;
		return true;
	}

	__declspec(naked) void __fastcall create_move_proxy(void* _this, int, int sequence_number, float input_sample_frametime, bool active) {
		__asm {
			push ebp
			mov  ebp, esp
			push ebx
			push esp
			push dword ptr[active]
			push dword ptr[input_sample_frametime]
			push dword ptr[sequence_number]
			call create_move
			pop  ebx
			pop  ebp
			retn 0Ch
		}
	}

	void __stdcall create_move(int sequence_number, float input_sample_frametime, bool active, bool& send_packet) {
		static auto original = vmt::client->original<decltype(&create_move_proxy)>(XOR32(22));

		globals->m_send_packet = &send_packet;
		original(interfaces::client, 0, sequence_number, input_sample_frametime, active);
	}

	STFI void skybox_changer() {
		static auto load_named_sky = patterns::load_named_sky.as<void(__fastcall*)(const char*)>();

		const auto sky_name = STR("sky_csgo_night02");
		if (settings->visuals.nightmode.enable && globals->m_previous_skybox != sky_name) {
			load_named_sky(sky_name.c_str());
			globals->m_previous_skybox = sky_name;
		} else if (!settings->visuals.nightmode.enable && globals->m_previous_skybox != globals->m_default_skybox) {
			load_named_sky(globals->m_default_skybox.c_str());
			globals->m_previous_skybox = globals->m_default_skybox;
		}

		// disable 3d skybox render
		globals->m_local->skybox_area() = settings->visuals.nightmode.enable ? 255 : 0;
	}

	void __stdcall frame_stage_notify(e_client_frame_stage stage) {
		static auto original = vmt::client->original<void(__stdcall*)(e_client_frame_stage)>(XOR32(37));

		static bool update = false;

		const auto disconnected = !interfaces::engine->is_connected() || !interfaces::engine->is_in_game() || globals->m_local == nullptr;

		if (disconnected || !ctx->is_valid()) {
			update = true;
			players->m_local_player.m_valid = false;
			globals->m_is_connected = false;
			std::memset(hvh::aimbot->m_data, 0, sizeof(hvh::aimbot->m_data));
			bullet_tracer->clear();

			//if (players->m_local_player.m_real_state != nullptr) {
			//	interfaces::mem_alloc->free(players->m_local_player.m_real_state);
			//	players->m_local_player.m_real_state = nullptr;
			//}

			if (disconnected) {
				skin_changer->reset();

				globals->m_default_skybox = GET_CVAR("sv_skyname")->m_string;
				globals->m_previous_skybox = "";
			}

			if (stage == e_client_frame_stage::frame_render_start) {
				::misc->enable_hidden_convars();
				network::on_frame_render_start();

				if (render::can_render) {
					render::begin();
					if (render::in_fsn) {
						render::end();
					}
				}
			}

			return original(stage);
		}

		globals->m_is_connected = true;
		if (update) {
			interfaces::update();
			update = false;
		}

		switch (stage) {
			case e_client_frame_stage::frame_start:
				break;
			case e_client_frame_stage::frame_net_update_start:
				break;
			case e_client_frame_stage::frame_net_update_postdataupdate_start:
				engine_prediction->on_net_update_postdataupdate_start();
				skin_changer->on_net_update_postdataupdate_start();
				break;
			case e_client_frame_stage::frame_net_update_postdataupdate_end:
				break;
			case e_client_frame_stage::frame_net_update_end:
				players->on_net_update_end();
				break;
			case e_client_frame_stage::frame_render_start:

				::misc->preserve_kill_feed(globals->m_round_start);
				globals->m_round_start = false;
				globals->on_render_start();
				hotkeys->update();
				animations_fsn->update();

				skybox_changer();
				pvs_fix();

				players->on_render_start();

				if (render::can_render) {
					render::begin();
					if (render::in_fsn) {
						render::end();
					}
				}

				projectiles->on_frame_render_start();
				bullet_tracer->on_render_start();
				::misc->on_render_start();
				::misc->enable_hidden_convars();

				engine_prediction->on_render_start(false);
				break;
			case e_client_frame_stage::frame_render_end:
				break;
			default:
				break;
		}

		skin_changer->on_frame_stage_notify(stage);

		original(stage);

		switch (stage) {
			case e_client_frame_stage::frame_start:
				break;
			case e_client_frame_stage::frame_net_update_start:
				break;
			case e_client_frame_stage::frame_net_update_postdataupdate_start:
				break;
			case e_client_frame_stage::frame_net_update_postdataupdate_end:
				break;
			case e_client_frame_stage::frame_net_update_end:
				net_data->apply();
				break;
			case e_client_frame_stage::frame_render_start:
				if (globals->m_local->is_alive()) {
					auto view_angles = interfaces::engine->get_view_angles();
					interfaces::prediction->set_local_viewangles(view_angles);
				}
				engine_prediction->on_render_start(true);
				break;
			case e_client_frame_stage::frame_render_end:
				break;
			default:
				break;
		}
	}
} // namespace hooks::client