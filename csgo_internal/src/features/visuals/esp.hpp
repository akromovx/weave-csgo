#pragma once
#include "local_esp.hpp"
#include "player_esp.hpp"
#include "weapon_esp.hpp"
#include "grenade_esp.hpp"

namespace esp {
	STFI void render() {
		weapons->render();
		projectiles->render();
		players->render();
		local->render();
	}
} // namespace esp