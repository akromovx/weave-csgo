#pragma once
#include "../base_includes.hpp"

namespace sdk {
	struct csgo_hud_radar_t;
	struct base_combat_weapon_t;

	extern base_combat_weapon_t* get_local_weapon();
} // namespace sdk

class misc_t {
public:
	void on_create_move();
	void on_override_view();
	void on_render_start();

	void preserve_kill_feed(bool round_start);

	void buy_items();
	const char8_t* get_weapon_icon(short item_definition_index);
	void enable_hidden_convars();
	sdk::csgo_hud_radar_t* get_hud_radar();

	bool is_in_chat();
};

GLOBAL_DYNPTR(misc_t, misc);