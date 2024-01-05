#pragma once
#include "../base_includes.hpp"
#include <atomic>
#include <wtypes.h>
#include <functional>

struct hotkey_t {
	enum class e_type {
		hold,
		toggle,
		always_on
	};

	void translate();

	__forceinline hotkey_t(e_type type = e_type::hold, uint32_t hash = HASH("hotkey-uninitialized"), std::string localize_name = STRS("?"))
		: m_type(type), m_hash(hash), m_localize_name(localize_name) {}

	__forceinline bool is_valid() const { return m_type == e_type::always_on || m_key > 0; }

	uint32_t m_hash{};
	e_type m_type{};
	uint16_t m_key{};
	bool m_active{};

	std::string m_localize_name{};
	std::string m_actual_name{};

	std::function<bool()> m_can_change = []() { return true; };

	bool m_show_in_binds = true;

	void set_active(bool in_game, bool flag);
};

struct hotkeys_t {
	std::atomic<short> m_states[256] = {};
	hotkeys_t();

	struct key_info_t {
		WPARAM m_key = 0;
		bool m_state = false;
	};

	bool m_key_binder_active = false;
	int m_last_key_pressed = 0;

	__forceinline bool at(int index) const {
		return m_states[index];
	}

	bool process_key(int idx, int key, bool state);
	key_info_t get_key_state(UINT msg, WPARAM w_param);
	bool key_updated(int key, UINT msg, WPARAM w_param);
	void on_wnd_proc(UINT msg, WPARAM w_param);

	void update();

	hotkey_t thirdperson{};
	hotkey_t peek_assist{};
	hotkey_t doubletap{};
	hotkey_t override_damage{};

	hotkey_t manual_right{};
	hotkey_t manual_left{};
	hotkey_t manual_back{};
	hotkey_t manual_forward{};

	hotkey_t desync_inverter{};
	hotkey_t slow_walk{};
	hotkey_t fake_duck{};
	hotkey_t freestand{};
	hotkey_t hide_shot{};

	std::vector<hotkey_t*> m_hotkeys{};
};

// global class that contains information about hotkeys state at the moment
GLOBAL_DYNPTR(hotkeys_t, hotkeys);