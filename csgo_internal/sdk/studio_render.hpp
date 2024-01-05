#pragma once
#include "../src/utils/utils.hpp"
#include "../src/utils/displacement.hpp"

namespace sdk {
	enum class override_type_t {
		normal = 0,
		buildshadows,
		depthwrite,
		custommaterial,
		ssaodepthwrite
	};

	struct material_t;
	struct draw_model_info_t {
		void* m_studio_hdr;
		void* m_hardware_data;
		void* m_decals;
		int m_skin;
		int m_body;
		int m_hitbox_set;
		void* m_client_entity;
		int m_lod;
		void* m_color_meshes;
		bool m_static_lightning;
		void* m_lightning_state;

		inline draw_model_info_t operator=(const draw_model_info_t& other) {
			std::memcpy(this, &other, sizeof(draw_model_info_t));
			return *this;
		}
	};

	struct studio_render_t {
		bool is_forced_material_override() {
			if (!m_override_material)
				return m_override_type == override_type_t::depthwrite || m_override_type == override_type_t::ssaodepthwrite;

			static auto dev_glow = STR("dev/glow");
			return std::string_view{ m_override_material->get_name() }.starts_with(dev_glow);
		}

	private:
		char pad_0[592];
		material_t* m_override_material;
		char pad_1[12];
		override_type_t m_override_type;
	};
} // namespace sdk