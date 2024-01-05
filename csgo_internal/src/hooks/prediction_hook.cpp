#include "hooker.hpp"
#include "hooks.hpp"
#include <intrin.h>

#include "../features/hvh/exploits.hpp"
#include "../game/engine_prediction.hpp"
#include "../game/net_data.hpp"

namespace hooks::prediction {
	using namespace sdk;

	bool __fastcall in_prediction(prediction_t* prediction, uint32_t) {
		static auto original = vmt::prediction->original<bool(__thiscall*)(prediction_t*)>(XOR32S(14));

		if (_ReturnAddress() == patterns::return_to_maintain_sequence_transitions.as<void*>())
			return false;

		return original(prediction);
	}

	void __fastcall run_command(void* ecx, void* edx, cs_player_t* player, user_cmd_t* cmd, move_helper_t* move_helper) {
		static auto original = vmt::prediction->original<void(__thiscall*)(void*, cs_player_t*, user_cmd_t*, move_helper_t*)>(XOR32S(19));

		if (player->index() != interfaces::engine->get_local_player())
			return original(ecx, player, cmd, move_helper);

		if (cmd->m_tickcount == INT_MAX) {
			//	player->set_abs_origin(player->origin());
			cmd->m_has_been_predicted = true;
			++player->tickbase();
			return;
		}

		hvh::exploits->fix_tickbase(cmd);

		engine_prediction->on_run_command(cmd, false);
		original(ecx, player, cmd, move_helper);
		engine_prediction->on_run_command(cmd, true);

		net_data->store();
	}
} // namespace hooks::prediction