#pragma once
#include "../../../sdk/entities.hpp"
#include "../../../sdk/model_render.hpp"
#include "../../base_includes.hpp"
#include "../../utils/matrix.hpp"
#include <functional>

struct lag_record_t;

namespace esp {
	struct chams_t {
		bool draw_chams(incheat_vars::chams_settings_t& set, bool pass_xqz = false, matrix3x4_t* matrix = nullptr, float override_alpha = 1.f);
		bool is_working(const sdk::draw_model_state_t& state, const sdk::model_render_info_t& info, std::function<void(matrix3x4_t*)> fn);

		void init();

		void add_hitmatrix(sdk::cs_player_t* player, matrix3x4_t* bones);
		__forceinline void clear_hitmatrices() { m_hits.clear(); }

		void on_post_screen_effects();

	private:
		struct {
			sdk::material_t* m_default{};
			sdk::material_t* m_flat{};
			sdk::material_t* m_metallic{};
			sdk::material_t* m_randomized{};

			sdk::material_t* m_glow_fade{};
			sdk::material_t* m_glow_line{};
			sdk::material_t* m_wireframe{};
			sdk::material_t* m_glass{};
			sdk::material_t* m_animated{};

			__forceinline sdk::material_t* get(int index) {
				switch (index) {
					case 0: return m_default;
					case 1: return m_flat;
					case 2: return m_metallic;
					case 3: return m_randomized;
				}

				return nullptr;
			}

			__forceinline sdk::material_t* get_overlay(int index) {
				switch (index) {
					case 0: return m_glow_fade;
					case 1: return m_glow_line;
					case 2: return m_wireframe;
					case 3: return m_glass;
					case 4: return m_animated;
				}

				return nullptr;
			}
		} m_materials;

		struct hit_matrix_t {
			int ent_index{};
			sdk::model_render_info_t info{};
			sdk::draw_model_state_t state{};
			matrix3x4_t m_bones[sdk::max_bones]{};
			float time{};
			matrix3x4_t model_to_world{};
			bool m_valid{};
		};

		std::vector<hit_matrix_t> m_hits{};

		struct hook_data_t {
			std::function<void(matrix3x4_t*)> m_callback{};
			sdk::draw_model_state_t m_state{};
			sdk::model_render_info_t m_info{};
		} m_hook;
	};

	GLOBAL_DYNPTR(chams_t, chams);
} // namespace esp