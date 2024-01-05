#pragma once
#include "../src/utils/displacement.hpp"

namespace sdk {
	struct render_view_t {
		VFUNC(set_blend(float value), void(__thiscall*)(decltype(this), float), 4, value);
		VFUNC(get_blend(), float(__thiscall*)(decltype(this)), 5);
		VFUNC(set_color_modulation(float const* blend), void(__thiscall*)(decltype(this), float const*), 6, blend);
		VFUNC(get_color_modulation(float* blend), void(__thiscall*)(decltype(this), float*), 7, blend);
	};
} // namespace sdk