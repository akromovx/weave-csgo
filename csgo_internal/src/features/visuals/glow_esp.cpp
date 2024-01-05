#include "glow_esp.hpp"
#include "../../interfaces.hpp"
#include "../../globals.hpp"

namespace esp {
	using namespace sdk;
	STFI void apply_glow(glow_object_manager_t::glow_object_definition_t* object, incheat_vars::glow_esp_settings_t& set) {
		if (!set.enable) {
			object->m_render_when_occluded = false;
			return;
		}

		object->m_glow_color = vec3d{ set.color.r() / 255.f, set.color.g() / 255.f, set.color.b() / 255.f };
		object->m_glow_alpha = set.color.a() / 255.f;
		object->m_render_when_occluded = true;
		object->m_render_when_unoccluded = false;
		object->m_full_bloom_render = false;
	}

	void glow_t::on_post_screen_effects() {
		for (auto i = 0; i < interfaces::glow_manager->m_glow_object_definitions.m_size; ++i) {
			auto object = &interfaces::glow_manager->m_glow_object_definitions[i];

			base_entity_t* entity = object->m_entity;

			if (object->m_entity == nullptr || object->is_unused())
				continue;

			if (entity->dormant())
				continue;

			if (entity->is_player()) {
				auto player = (cs_player_t*)entity;
				if (!player->is_teammate() && player != globals->m_local)
					apply_glow(object, settings->player_esp.glow);
				else if (player == globals->m_local)
					apply_glow(object, settings->player_esp.local_glow);
			} else if (entity->is_weapon() && entity->owner().value() == -1)
				apply_glow(object, settings->weapon_esp.glow);
		}
	}
} // namespace esp