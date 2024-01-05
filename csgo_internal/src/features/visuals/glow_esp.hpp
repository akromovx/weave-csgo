#pragma once
#include "../../base_includes.hpp"
#include "../../../sdk/glow_manager.hpp"

namespace esp {
	struct glow_t {
		void on_post_screen_effects();
	};

	GLOBAL_DYNPTR(glow_t, glow);
} // namespace esp