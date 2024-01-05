#include "hotkeys.hpp"
#include "../globals.hpp"
#include "../interfaces.hpp"
#include "../features/misc.hpp"
#include <thread>
#include "../features/localization.hpp"

#define PUSH_HOTKEY(name, default_type, localization_name)   \
	name = { default_type, HASH(#name), localization_name }; \
	m_hotkeys.emplace_back(&name)

STFI void initialize_thread() {
	constexpr DWORD sleep_time = 1000 / 64;
	std::thread([&] {
		while (!ctx->unload) {
			for (int i = 1; i < 255; ++i)
				hotkeys->m_states[i] = GetAsyncKeyState(i);

			if (globals->m_tickrate > 0.f)
				Sleep(1000 / (int)globals->m_tickrate);
			else
				Sleep(sleep_time);
		}
	}).detach();
}

void hotkey_t::translate() {
	m_actual_name = localization::get(m_localize_name);
}

void hotkey_t::set_active(bool in_game, bool flag) {
	if (!in_game)
		m_active = false;

	m_active = m_can_change() ? flag : false;
}

hotkeys_t::hotkeys_t() {
	PUSH_HOTKEY(thirdperson, hotkey_t::e_type::toggle, STRS("hotkey.thirdperson"));
	PUSH_HOTKEY(doubletap, hotkey_t::e_type::toggle, STRS("hotkey.doubletap"));
	PUSH_HOTKEY(peek_assist, hotkey_t::e_type::hold, STRS("hotkey.peek_assist"));
	PUSH_HOTKEY(override_damage, hotkey_t::e_type::toggle, STRS("hotkey.override_damage"));

	PUSH_HOTKEY(manual_right, hotkey_t::e_type::toggle, STRS("hotkey.manual_right"));
	PUSH_HOTKEY(manual_left, hotkey_t::e_type::toggle, STRS("hotkey.manual_left"));
	PUSH_HOTKEY(manual_back, hotkey_t::e_type::toggle, STRS("hotkey.manual_back"));
	PUSH_HOTKEY(manual_forward, hotkey_t::e_type::toggle, STRS("hotkey.manual_forward"));

	PUSH_HOTKEY(desync_inverter, hotkey_t::e_type::toggle, STRS("hotkey.desync_inverter"));
	PUSH_HOTKEY(slow_walk, hotkey_t::e_type::hold, STRS("hotkey.slow_walk"));
	PUSH_HOTKEY(fake_duck, hotkey_t::e_type::hold, STRS("hotkey.fake_duck"));
	PUSH_HOTKEY(freestand, hotkey_t::e_type::toggle, STRS("hotkey.freestand"));
	PUSH_HOTKEY(hide_shot, hotkey_t::e_type::toggle, STRS("hotkey.hide_shot"));

	const auto ragebot_callback = []() { return settings->ragebot.enable; };
	const auto antiaim_callback = []() { return settings->antiaim.enable; };

	doubletap.m_can_change = ragebot_callback;
	hide_shot.m_can_change = ragebot_callback;
	override_damage.m_can_change = ragebot_callback;

	manual_right.m_can_change = antiaim_callback;
	manual_left.m_can_change = antiaim_callback;
	manual_back.m_can_change = antiaim_callback;
	manual_forward.m_can_change = antiaim_callback;
	desync_inverter.m_can_change = antiaim_callback;
	slow_walk.m_can_change = antiaim_callback;
	fake_duck.m_can_change = antiaim_callback;
	freestand.m_can_change = antiaim_callback;

	initialize_thread();
}

bool hotkeys_t::process_key(int idx, int key, bool state) {
	return idx == key && state;
}

hotkeys_t::key_info_t hotkeys_t::get_key_state(UINT msg, WPARAM w_param) {
	switch (msg) {
		case WM_LBUTTONDOWN:
		case WM_LBUTTONDBLCLK:
			return { VK_LBUTTON, true };
		case WM_LBUTTONUP:
			return { VK_LBUTTON, false };
		case WM_RBUTTONDOWN:
		case WM_RBUTTONDBLCLK:
			return { VK_RBUTTON, true };
		case WM_RBUTTONUP:
			return { VK_RBUTTON, false };
		case WM_MBUTTONDOWN:
		case WM_MBUTTONDBLCLK:
			return { VK_MBUTTON, true };
		case WM_MBUTTONUP:
			return { VK_MBUTTON, false };
		case WM_XBUTTONDOWN:
		case WM_XBUTTONDBLCLK: {
			UINT button = GET_XBUTTON_WPARAM(w_param);
			if (button == XBUTTON1)
				return { VK_XBUTTON1, true };
			else if (button == XBUTTON2)
				return { VK_XBUTTON2, true };

			break;
		}
		case WM_XBUTTONUP: {
			UINT button = GET_XBUTTON_WPARAM(w_param);
			if (button == XBUTTON1)
				return { VK_XBUTTON1, false };
			else if (button == XBUTTON2)
				return { VK_XBUTTON2, false };

			break;
		}
		case WM_KEYDOWN:
			return { w_param, true };
		case WM_KEYUP:
			return { w_param, false };
		case WM_SYSKEYDOWN:
			return { w_param, true };
		case WM_SYSKEYUP:
			return { w_param, false };
	}

	return { 0, false };
}

bool hotkeys_t::key_updated(int key, UINT msg, WPARAM w_param) {
	auto key_state = get_key_state(msg, w_param);
	return key == key_state.m_key && key_state.m_state;
}

void hotkeys_t::on_wnd_proc(UINT msg, WPARAM w_param) {
	if (GetForegroundWindow() != ctx->hwnd)
		return;

	if (misc->is_in_chat())
		return;

	if (interfaces::engine->is_console_open())
		return;

	const auto in_game = interfaces::engine->is_in_game() && interfaces::engine->is_connected();

	if (m_key_binder_active) {
		auto key_state = get_key_state(msg, w_param);
		if (key_state.m_key > 0 && key_state.m_state) {
			m_last_key_pressed = key_state.m_key;
			m_key_binder_active = false;
		}
		return;
	}

	if (m_last_key_pressed > 0)
		return;

	m_last_key_pressed = 0;

	for (auto hotkey: m_hotkeys) {
		if (hotkey->m_key > 0) {
			bool key_changed = key_updated(hotkey->m_key, msg, w_param);

			if (hotkey == &manual_right || hotkey == &manual_left || hotkey == &manual_back || hotkey == &manual_forward) {
				hotkey->m_type = hotkey_t::e_type::toggle;
				if (!settings->antiaim.enable) {
					hotkey->m_active = false;
					continue;
				}

				if (hotkey->m_key > 0 && key_changed) {
					if (hotkey == &manual_left) {
						hotkey->set_active(in_game, !hotkey->m_active);
						if (hotkey->m_active) {
							manual_right.m_active = false;
							manual_back.m_active = false;
							manual_forward.m_active = false;
						}
					} else if (hotkey == &manual_right) {
						hotkey->set_active(in_game, !hotkey->m_active);
						if (hotkey->m_active) {
							manual_back.m_active = false;
							manual_left.m_active = false;
							manual_forward.m_active = false;
						}
					} else if (hotkey == &manual_back) {
						hotkey->set_active(in_game, !hotkey->m_active);
						if (hotkey->m_active) {
							manual_left.m_active = false;
							manual_right.m_active = false;
							manual_forward.m_active = false;
						}
					} else if (hotkey == &manual_forward) {
						hotkey->set_active(in_game, !hotkey->m_active);
						if (hotkey->m_active) {
							manual_left.m_active = false;
							manual_back.m_active = false;
							manual_right.m_active = false;
						}
					}
				}
			} else if (hotkey->m_type == hotkey_t::e_type::toggle) {
				if (key_changed)
					hotkey->set_active(in_game, !hotkey->m_active);
			}
		}
	}
}

void hotkeys_t::update() {
	if (GetForegroundWindow() != ctx->hwnd)
		return;

	if (misc->is_in_chat())
		return;

	if (interfaces::engine->is_console_open())
		return;

	if (m_key_binder_active || m_last_key_pressed > 0)
		return;

	const auto in_game = interfaces::engine->is_in_game() && interfaces::engine->is_connected();
	for (auto& hotkey: m_hotkeys) {
		hotkey->m_key = std::clamp<uint16_t>(hotkey->m_key, 0, 255);
		switch (hotkey->m_type) {
			case hotkey_t::e_type::hold: hotkey->set_active(in_game, m_states[hotkey->m_key]); break;
			case hotkey_t::e_type::always_on: hotkey->set_active(in_game, true); break;
			default:
				if (hotkey->m_key == 0)
					hotkey->set_active(in_game, false);
				break;
		}
	}
}