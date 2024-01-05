#pragma once
#include "../../base_includes.hpp"

namespace esp {
	struct projectiles_t {
		void render();
	};

	GLOBAL_DYNPTR(projectiles_t, projectiles);
} // namespace esp