#include "hooks.hpp"
#include "hooker.hpp"

namespace hooks::movement {
	using namespace sdk;

	void __fastcall process_movement(void* ecx, void* edx, cs_player_t* player, move_data_t* move_data) {
		static auto original = vmt::movement->original<void(__thiscall*)(void*, cs_player_t*, move_data_t*)>(XOR32(1));
		move_data->m_game_code_moved_player = false;
		original(ecx, player, move_data);
	}
}