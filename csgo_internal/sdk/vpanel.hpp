#pragma once
#include "../src/utils/utils.hpp"

namespace sdk {
	struct vpanel_t {
		VFUNC(set_mouse_input_enabled(uint32_t panel, bool state), void(__thiscall*)(decltype(this), uint32_t, bool), 32, panel, state);
		VFUNC(get_name(int index), const char*(__thiscall*)(decltype(this), int), 36, index);
	};

	struct ui_panel_t {
		VFUNC(child_count(), int(__thiscall*)(decltype(this)), 48);
		VFUNC(child(int n), ui_panel_t*(__thiscall*)(decltype(this), int), 49, n);
		VFUNC(has_class(const char* name), bool(__thiscall*)(decltype(this), const char*), 139, name);
		VFUNC(set_attribute_float(const char* name, float value), bool(__thiscall*)(decltype(this), const char*, float), 288, name, value);
	};
} // namespace sdk