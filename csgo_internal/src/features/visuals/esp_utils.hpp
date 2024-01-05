#pragma once
#include "../../render.hpp"

namespace esp {
	struct bounding_box_t {
		float x{}, y{}, w{}, h{};
		float m_alpha{};
		bool m_got{};

		enum class e_pos_type : uint32_t {
			bottom,
			top,
			left,
			right,
			max
		};

		static constexpr float m_padding = 2.f;
		static constexpr float m_bar_size = 2.f;

		struct {
			float m_offsets[(uintptr_t)e_pos_type::max];
		} m_render_state;

		struct bar_value_t {
			float m_max{};
			float m_visual_value{};
			float m_value{};
			std::pair<color_t, color_t> m_colors;
			e_pos_type m_pos{};
			bool m_show_number{};

			bar_value_t(float maximum, float visual_value, float value, std::pair<color_t, color_t> colors = { colors::white, colors::white }, e_pos_type pos = e_pos_type::top, bool show_number = false)
				: m_max(maximum), m_visual_value(visual_value), m_value(value), m_colors(colors), m_pos(pos), m_show_number(show_number){};
		};

		struct text_array_element_t {
			std::string m_text{};
			color_t m_color = { 255, 255, 255, 255 };
			float m_alpha = 1.f;
		};

		using text_array_t = std::vector<text_array_element_t>;

		void render_base(color_t color);
		void bar(bar_value_t&& value);
		void text(std::string text, e_pos_type pos, std::pair<color_t, color_t> colors, font_t font = nullptr);
		void text_array(text_array_t&& text_array, e_pos_type pos = e_pos_type::right, font_t font = nullptr);

		__forceinline void reset_render_state() {
			for (auto& o: m_render_state.m_offsets)
				o = 0.f;
		}

		__forceinline bool out_of_screen() const {
			return x + w < 0 || y + h < 0 || w <= 1 || h <= 1 ||

				   // if 1/4 of the box is below the bottom border of the screen
				   x + w * 0.25f > render::screen_width ||
				   y + h * 0.25f > render::screen_height;
		};

		__forceinline void reset_bounds() {
			x = y = w = h = 0;
		}

		bool get(vec3d render_origin, float duck_amount);
		bool get(vec3d min, vec3d max, const matrix3x4_t& coordinate_frame);
	};

	extern font_t get_font(int id);
	STFI std::pair<color_t, color_t> get_gradient(c_imgui_color* colors) { return { colors[0].get(), colors[1].get() }; }
	STFI bool is_on_screen(const vec2d& p) { return p.x >= 0 && p.y >= 0 && p.x <= render::screen_width && p.y <= render::screen_height; }
} // namespace esp