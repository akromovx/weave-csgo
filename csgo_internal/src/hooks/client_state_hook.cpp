#include "hooks.hpp"
#include "hooker.hpp"

#include "../globals.hpp"
#include "../features/hvh/exploits.hpp"

namespace hooks::client_state {
	using namespace sdk;

	bool __fastcall send_netmsg(sdk::net_channel_t* channel, uint32_t edx, sdk::net_message_t* message, bool reliable, bool voice) {
		auto original = detour->original(&send_netmsg);

		if (message->get_type() == 14)
			return false;

		if (message->get_group() == 9)
			voice = true;

		return original(channel, edx, message, reliable, voice);
	}

	void __fastcall packet_start(client_state_t* state, uint32_t edx, int32_t incoming, int32_t outgoing) {
		static auto original = vmt::client_state->original<void(__thiscall*)(void*, int, int)>(XOR32(5));

		if (globals->m_local == nullptr || !globals->m_local_alive || hvh::exploits->m_in_shift) {
			globals->m_cmds.clear();
			return original(state, incoming, outgoing);
		}

		for (auto it = globals->m_cmds.rbegin(); it != globals->m_cmds.rend(); ++it) {
			if (!it->is_outgoing)
				continue;

			if (it->cmd_number == outgoing || outgoing > it->cmd_number && (!it->is_used || it->previous_command_number == outgoing)) {
				it->previous_command_number = outgoing;
				it->is_used = true;
				original(state, incoming, outgoing);
				break;
			}
		}

		auto result = false;
		for (auto it = globals->m_cmds.begin(); it != globals->m_cmds.end();) {
			if (outgoing == it->cmd_number || outgoing == it->previous_command_number)
				result = true;

			if (outgoing > it->cmd_number && outgoing > it->previous_command_number)
				it = globals->m_cmds.erase(it);
			else
				++it;
		}

		if (!result)
			original(state, incoming, outgoing);
	}

	//void __fastcall packet_start(c_client_state* state, uint32_t edx, int32_t incoming, int32_t outgoing) {
	//    static auto original = vmt::client_state->original<decltype(&packet_start)>(XOR32(5));

	//    if (!globals->m_local || !globals->m_local->is_alive())
	//        return original(state, edx, incoming, outgoing);

	//    // update only last sent commands
	//    for (auto it = globals->m_commands.begin(); it != globals->m_commands.end(); ++it) {
	//        if (*it == outgoing) {
	//            globals->m_commands.erase(it);
	//            original(state, edx, incoming, outgoing);
	//            break;
	//        }
	//    }
	//}

	void __fastcall packet_end(client_state_t* state, uint32_t edx) {
		static auto original = vmt::client_state->original<void(__thiscall*)(void*)>(XOR32(6));
		return original(state);
	}
}