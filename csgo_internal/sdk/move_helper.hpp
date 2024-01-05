#pragma once
#include "../src/utils/utils.hpp"
#include "entities.hpp"

namespace sdk {
	struct move_helper_t {
		VFUNC(set_host(base_entity_t* host), void(__thiscall*)(decltype(this), base_entity_t*), 1, host);
		VFUNC(process_impacts(), void(__thiscall*)(decltype(this)), 4);
	};
} // namespace sdk