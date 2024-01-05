#pragma once
#include "../../base_includes.hpp"

namespace esp {
	struct local_t {
		enum class e_ragebot_rejection {
			none,
			hitchance,
			exploits,

		} m_ragebot_rejection{ e_ragebot_rejection::none };

		void render();
	};

	GLOBAL_DYNPTR(local_t, local);
} // namespace esp