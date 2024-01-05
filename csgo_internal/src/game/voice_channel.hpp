#pragma once
#include "../base_includes.hpp"

struct voice_channel_t {
	enum class e_msg_type : uint8_t {
		// used to share player info between hack users
		// sends every tick and every player we're seeing on ESP
		player_info = 0xF1,
	};

	static constexpr uint16_t base_id = 0xDEAD;

	enum e_friend_cheat {
		weave_id = 0x99,
		airflow_id = 0xFA,
		airflow_old_id = 0xAF,
		airflow_boss_id = 0xBA,
		boss_id = 0xBB,
		furcore_id = 0xDC,
		floridahook_id = 0x66,
		karnazity = 0xB4,
	};

#ifdef _DEBUG
	static constexpr uint32_t unique_id = (base_id << 16) | (boss_id << 8);
#else
	static constexpr uint32_t unique_id = (base_id << 16) | (weave_id << 8);
#endif

	static __forceinline auto get_id(uint32_t n) {
		return (n & 0xFF00) >> 8;
	}

	static constexpr uint32_t get_id(e_msg_type type) {
		switch (type) {
		case e_msg_type::player_info: return unique_id | ((uint8_t)e_msg_type::player_info);
		}

		return 0;
	}

	struct comm_string_t {
		char m_data[16]{};
		uint32_t m_current_length = 0;
		uint32_t m_max_length = 15;
	};

	void send_data(const utils::bytes_t& data);
};

GLOBAL_DYNPTR(voice_channel_t, voice_channel);