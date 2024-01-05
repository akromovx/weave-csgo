#pragma once
#include "../renderable.hpp"

namespace NS_GUI {
	namespace shapes {
		class rect_t : public i_renderable {
		private:
			color_t m_color{};
			void* m_texture{};
			float m_alpha{};

		public:
			inline rect_t(vec2d size, color_t color, void* texture = nullptr, e_position_type position_type = e_position_type::relative, vec2d position = { 0.f, 0.f }) {
				m_size = size;
				m_color = color;
				m_position_type = position_type;
				m_position = position;
				m_texture = texture;
			}

			inline void set_override_alpha(float value) { m_alpha = value; }

			HELD_VALUE(nullptr);
			ITEM_TYPE_SHAPE;
			CLONE_METHOD(rect_t);

			void render(render_context_t& ctx) override;
		};
	} // namespace shapes
} // namespace NS_GUI