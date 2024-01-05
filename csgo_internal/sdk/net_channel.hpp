#pragma once
#include "../src/utils/utils.hpp"

namespace sdk {
	struct voice_communication_data_t {
		uint32_t m_xuid_low;
		uint32_t m_xuid_high;
		int32_t m_sequence_bytes;
		uint32_t m_section_number;
		uint32_t m_uncompressed_sample_offset;

		__forceinline uint8_t* data() { return (uint8_t*)this; }
	};

	struct svc_msg_voice_data_t {
		char pad0[0x8];
		int32_t m_client;
		int32_t m_audible_mask;
		uint32_t m_xuid_low;
		uint32_t m_xuid_high;
		std::string* m_voice_data;
		bool m_proximity;
		bool m_caster;
		int32_t m_format;
		int32_t m_sequence_bytes;
		uint32_t m_section_number;
		uint32_t m_uncompressed_sample_offset;

		__forceinline voice_communication_data_t data() const {
			voice_communication_data_t data{};
			data.m_xuid_low = m_xuid_low;
			data.m_xuid_high = m_xuid_high;
			data.m_sequence_bytes = m_sequence_bytes;
			data.m_section_number = m_section_number;
			data.m_uncompressed_sample_offset = m_uncompressed_sample_offset;
			return data;
		}
	};

	struct clc_msg_voice_data_t {
		uint32_t m_vtable;
		uint8_t pad0[4];
		uint32_t m_clc_msg_voice_data_vtable;
		uint8_t pad1[8];
		uintptr_t m_data;
		uint32_t m_xuid_low;
		uint32_t m_xuid_high;
		int32_t m_format;
		int32_t m_sequence_bytes;
		uint32_t m_section_number;
		uint32_t m_uncompressed_sample_offset;
		int32_t m_cached_size;
		uint32_t m_flags;
		uint8_t pad2[0xFF];

		__forceinline void set_data(voice_communication_data_t* data) {
			m_xuid_low = data->m_xuid_low;
			m_xuid_high = data->m_xuid_high;
			m_sequence_bytes = data->m_sequence_bytes;
			m_section_number = data->m_section_number;
			m_uncompressed_sample_offset = data->m_uncompressed_sample_offset;
		}
	};

	struct net_channel_t {
		char pad_0000[0x18];
		uint32_t m_out_sequence_nr;
		uint32_t m_in_sequence_nr;
		uint32_t m_out_sequence_nr_ack;
		uint32_t m_out_reliable_state;
		uint32_t m_in_reliable_state;
		uint32_t m_choked_packets;

		VFUNC(is_loopback(), bool(__thiscall*)(decltype(this)), 6);
		VFUNC(send_net_msg(const uintptr_t msg, bool reliable = false, bool voice = false), bool(__thiscall*)(decltype(this), uintptr_t, bool, bool), 40, msg, reliable, voice);
		VFUNC(send_datagram(), int(__thiscall*)(decltype(this), int), 46, 0);
	};
} // namespace sdk