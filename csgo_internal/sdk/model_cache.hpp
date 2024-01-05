#pragma once
#include "model_info.hpp"

namespace sdk {
	struct model_cache_t {
		VFUNC(get_hardware_data(uint16_t handle), studiohwdata_t*(__thiscall*)(decltype(this), uint16_t), 15, handle);

		VFUNC(begin_lock(), void(__thiscall*)(decltype(this)), 33);
		VFUNC(end_lock(), void(__thiscall*)(decltype(this)), 34);
	};
} // namespace sdk