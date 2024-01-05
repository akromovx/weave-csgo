#pragma once
#include "gui.hpp"

namespace NS_GUI {
	class i_renderable {
	protected:
		vec2d m_size{};
		vec2d m_position{};
		e_position_type m_position_type{ e_position_type::relative };
		std::optional<c_style> m_style{};
		bool m_hovered{};
		bool m_low_height{};

	public:
		bool is_low_height() const { return m_low_height; }

		// Returning the size of the element
		// (0.f, 0.f) if undefined
		vec2d get_size() const { return m_size; };

		// Returning the type of position has been set on initialization
		// e_position_type::relative by default
		e_position_type get_position_type() const { return m_position_type; }

		// Setting the type of position
		// enum: e_position_type
		void set_position_type(e_position_type type) { m_position_type = type; }

		// Setting the position for the item
		// If the type of position is "relative" this position will be applied relative
		void set_position(vec2d position) { m_position = position; }

		// Setting the override style for this item
		void set_style(const c_style& style) { m_style = style; }

		// Reset the style to the default values
		void reset_style() { m_style = {}; }
		std::optional<c_style>& get_style() { return m_style; }

		c_style& override_style() {
			if (!m_style.has_value())
				m_style = c_style{};

			return m_style.value();
		}

		bool is_hovered() const { return m_hovered; }

		vec2d calculate_position(const render_context_t& ctx) const {
			switch ((int)m_position_type) {
				case (int)e_position_type::absolute:
					return ctx.m_base_position + m_position;
				case (int)e_position_type::relative:
					return ctx.m_base_position + ctx.m_cursor + m_position;
			}

			return INVALID_POS;
		}

		virtual e_item_type get_item_type() = 0;
		virtual void render(render_context_t& ctx) = 0;
		virtual void* value_pointer() const = 0;
		virtual i_renderable* clone() const = 0;
		virtual ~i_renderable(){};
	};
} // namespace NS_GUI