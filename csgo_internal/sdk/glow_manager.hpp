#pragma once
#include "../src/utils/vector.hpp"
#include "entities.hpp"

namespace sdk {
	struct glow_object_manager_t {
		struct glow_object_definition_t {
			int m_next_free_slot;
			base_entity_t* m_entity;
			vec3d m_glow_color;
			float m_glow_alpha;
			bool m_glow_alpha_capped_by_render_alpha;
			float m_glow_alpha_function_of_max_velocity;
			float m_glow_alpha_max;
			float m_glow_pulse_overdrive;
			bool m_render_when_occluded;
			bool m_render_when_unoccluded;
			bool m_full_bloom_render;
			int m_full_bloom_stencil_test_value;
			int m_render_style;
			int m_split_screen_slot;

			__forceinline bool is_unused() const {
				return m_next_free_slot != glow_object_definition_t::ENTRY_IN_USE;
			}

			static constexpr int END_OF_FREE_LIST = -1;
			static constexpr int ENTRY_IN_USE = -2;
		};

		struct glow_box_definition_t {
			vec3d m_position{};
			vec3d m_orient{};
			vec3d m_mins{}, m_maxs{};
			float m_birth_time{}, m_termination_time{};
			uint32_t m_color{};
		};

		void add_glow_box(const vec3d& origin, const vec3d& orient, const vec3d& mins, const vec3d& maxs, const color_t& clr, float birth_time, float life_time) {
			m_glow_box_definitions.add_to_tail({ origin, orient, mins, maxs, birth_time, birth_time + life_time, clr.u32() });
		}

		utils::utl_vector_t<glow_object_definition_t> m_glow_object_definitions{};
		int32_t m_first_free_slot{};
		utils::utl_vector_t<glow_box_definition_t> m_glow_box_definitions{};
	};
} // namespace sdk