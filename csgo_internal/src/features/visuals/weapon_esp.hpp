#pragma once
#include "../../base_includes.hpp"

namespace esp {
	struct weapons_t {
		void render();
	};

	GLOBAL_DYNPTR(esp::weapons_t, weapons);
} // namespace esp