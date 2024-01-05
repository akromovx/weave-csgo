#include "hooker.hpp"
#include "hooks.hpp"

#include <intrin.h>

#include "../globals.hpp"
#include "../interfaces.hpp"

#include "../features/hvh/exploits.hpp"
#include "../features/hvh/hvh.hpp"

#include "../game/engine_prediction.hpp"

namespace hooks::engine {
	using namespace sdk;
	using namespace hvh;

	void __vectorcall cl_move(float accumulated_extra_samples, bool final_tick) {
		static auto original = detour->original(&cl_move);

		auto cs = interfaces::client_state;

		exploits->m_charging = false;
		globals->on_cl_move();

		if (!interfaces::engine->is_in_game() || cs->m_net_channel == nullptr || cs->m_paused || globals->m_local == nullptr || !globals->m_local->is_alive()) {
			exploits->on_cl_move(final_tick);
			engine_prediction->update();
			original(accumulated_extra_samples, final_tick);
			globals->m_cmds.clear();
			return;
		}

		exploits->on_cl_move(final_tick);

		if (exploits->m_recharge_ticks > 0 && interfaces::client_state->m_choked_commands == 0) {
			exploits->m_charging = true;
			exploits->m_shift_ticks = 0;
			if (--exploits->m_recharge_ticks <= 0) {
				exploits->m_charged = true;
				exploits->m_need_recharge = false;
			} else
				exploits->m_charged = false;

			exploits->m_dt_shots = 0;
			++exploits->m_charged_ticks;

			int new_command = cs->m_last_outgoing_command + 1;
			auto cmd = interfaces::input->get_user_cmd(new_command);
			auto verified = interfaces::input->get_verified_cmd(new_command);
			std::memcpy(cmd, interfaces::input->get_user_cmd(new_command - 1), sizeof(user_cmd_t));
			cmd->m_command_number = new_command;
			cmd->m_tickcount = INT_MAX;

			std::memcpy(&verified->m_cmd, cmd, sizeof(user_cmd_t));
			verified->m_crc = cmd->get_checksum();

			cs->m_last_outgoing_command = cs->m_net_channel->send_datagram();
			return;
		}

		engine_prediction->update();
		original(accumulated_extra_samples, final_tick);

		if (exploits->m_charged && exploits->m_shift_ticks > 0 && interfaces::client_state->m_choked_commands == 1) {
			exploits->m_in_shift = true;
			const int shift = exploits->m_shift_ticks - 1;

			for (int i = 0; i < shift; ++i) {
				engine_prediction->update();
				original(interfaces::global_vars->m_interval_per_tick, i == shift - 1);
			}

			//fakelag->m_need_reset_shot = true;
			exploits->on_post_shift();
		}
	}

	void __vectorcall read_packets(bool final_tick) {
		static auto original = detour->original(&read_packets);
		if (!exploits->can_read_packets())
			original(final_tick);
	}

	bool __fastcall is_connected(void* ecx) {
		static auto original = vmt::engine->original<decltype(&is_connected)>(XOR32(27));

		if (settings->misc.unlock_inventory && _ReturnAddress() == patterns::is_load_out_allowed.as<void*>())
			return false;

		return original(ecx);
	}

	bool __fastcall is_hltv(void* this_pointer, void* edx) {
		static auto original_is_hltv = vmt::engine->original<decltype(&is_hltv)>(XOR32(93));

		if ((DWORD)_ReturnAddress() == (DWORD)patterns::animstate::setup_velocity)
			return true;

		if ((DWORD)_ReturnAddress() == (DWORD)patterns::accum_layers)
			return true;

		return original_is_hltv(this_pointer, edx);
	}

	bool __fastcall is_paused(void* ecx, void* edx) {
		static auto original_is_paused = vmt::engine->original<decltype(&is_paused)>(XOR32(90));

		static DWORD* return_to_extrapolation = patterns::return_to_extrapolation.as<DWORD*>();

		if (_ReturnAddress() == (void*)return_to_extrapolation)
			return true;

		return original_is_paused(ecx, edx);
	}
} // namespace hooks::engine