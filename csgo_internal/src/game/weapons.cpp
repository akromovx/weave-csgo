#include "weapons.hpp"
#include "override_entity_list.hpp"

#include "../globals.hpp"
#include "../interfaces.hpp"

#include "../features/misc.hpp"

void weapons_t::on_paint_traverse() {
	if (!settings->weapon_esp.enable)
		return;

	THREAD_SAFE(entities->m_mutex);

	for (auto& [weapon, entry]: entities->m_weapons) {
		if (weapon->dormant() || weapon->owner().value() != -1) {
			entry.m_valid = false;
			continue;
		}

		entry.m_valid = true;
		entry.m_distance_to_local = (globals->m_local != nullptr && globals->m_local->is_alive()) ? entry.m_origin.dist(globals->m_local->origin()) : 0.f;

		auto collideable = weapon->get_model();
		entry.m_box.m_got = entry.m_distance_to_local < 1000.f ? entry.m_box.get(collideable->m_mins, collideable->m_maxs, weapon->coordinate_frame()) : false;
		entry.m_origin = weapon->render_origin();
		entry.m_ammo = weapon->ammo();

		auto weapon_info = weapon->get_cs_weapon_info();
		if (weapon_info != nullptr) {
			entry.m_max_ammo = weapon_info->m_max_ammo_1;
			entry.m_name = interfaces::localize->find_utf8(weapon_info->m_hud_name);
			entry.m_icon = (char*)misc->get_weapon_icon(weapon->item_definition_index());
		} else
			entry.m_ammo = entry.m_max_ammo = 0;
	}
}