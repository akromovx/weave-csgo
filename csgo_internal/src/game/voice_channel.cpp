#include "voice_channel.hpp"
#include "../interfaces.hpp"
#include "../features/visuals/logs.hpp"

using namespace sdk;

void voice_channel_t::send_data(const utils::bytes_t& data) {
	voice_communication_data_t new_data{};
	std::memcpy(new_data.data(), data.data(), data.size());

	clc_msg_voice_data_t msg{};
	std::memset(&msg, 0, sizeof(msg));

	patterns::construct_voice_data_message.as<uint32_t(__fastcall*)(void*, void*)>()(&msg, nullptr);

	msg.set_data(&new_data);
	comm_string_t comm_str{};
	msg.m_data = (uintptr_t)&comm_str;
	msg.m_format = 0; // VoiceFormat_Steam
	msg.m_flags = 63; // All flags

	auto net_channel = interfaces::client_state->m_net_channel;
	if (net_channel == nullptr || net_channel->is_loopback())
		return;

	net_channel->send_net_msg((uintptr_t)&msg, false, true);

	// TODO: patterns::destruct_voice_data_message
}