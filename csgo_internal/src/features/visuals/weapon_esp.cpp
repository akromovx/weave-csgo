#include "weapon_esp.hpp"
#include "../../game/override_entity_list.hpp"

namespace esp {
	void weapons_t::render() {
		auto& set = settings->weapon_esp;
		if (!set.enable)
			return;

		THREAD_SAFE(entities->m_mutex);

		for (auto& [weapon, entry]: entities->m_weapons) {
			auto& box = entry.m_box;
			box.reset_render_state();
			if (!entry.m_valid || !box.m_got)
				continue;

			if (entry.m_distance_to_local > 500.f)
				continue;

			float alpha = 1.f;
			if (entry.m_distance_to_local >= 400.f)
				alpha = 1.f - (entry.m_distance_to_local - 400.f) / 100.f;

			box.m_alpha = alpha;

			if (set.box)
				box.render_base(set.box_color.get());

			if (set.ammo.value && entry.m_ammo > 0)
				box.bar({ (float)entry.m_max_ammo, (float)entry.m_max_ammo, (float)entry.m_ammo,
						  get_gradient(set.ammo.colors), (bounding_box_t::e_pos_type)set.ammo.position, entry.m_ammo < entry.m_max_ammo });

			if (set.name.value.at(1))
				box.text(entry.m_icon, (bounding_box_t::e_pos_type)set.name.position, get_gradient(set.name.colors), fonts::weapon_icons);

			if (set.name.value.at(0))
				box.text(entry.m_name, (bounding_box_t::e_pos_type)set.name.position, get_gradient(set.name.colors), get_font(set.name.font));
		}
	}
} // namespace esp