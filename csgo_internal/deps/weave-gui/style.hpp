#pragma once
#include "../../src/utils/color.hpp"
#include "../../src/utils/vector.hpp"
#include "utils/macros.hpp"

namespace NS_GUI {
	inline color_t accent_color1 = { 255, 139, 59 };
	inline color_t accent_color2 = { 251, 86, 103 };

	class c_style {
	public:
		using callback_t = std::function<void(c_style&)>;

		vec2d window_padding{};
		vec2d container_padding{};
		vec2d controls_text_padding{};
		color_t window_backround{};
		color_t group_background{};

		color_t clickable_color{};
		color_t clickable_hovered{};
		color_t clickable_inactive{};

		color_t border_color{};

		color_t text_color{};
		color_t text_hovered{};
		color_t text_inactive{};

		color_t accent_color1{};
		color_t accent_color2{};

		bool bold_text{};
		bool small_text{};
		bool transparent_clickable{};
		bool invisible_clickable{};
		bool disable_scroll{};
		bool scroll_fade{};
		bool gradient_text_in_button{};

		float rounding{};
		float item_width{};

		std::string tooltip{};

		void* font{};

		// Resets this style to default values
		void reset() {
			window_padding = { 16.f, 16.f };
			container_padding = { 8.f, 0.f };
			controls_text_padding = { 12.f, 8.f };

			window_backround = { 12, 12, 12 };
			group_background = { 20, 20, 20 };

			clickable_color = { 20, 20, 20 };
			clickable_hovered = { 24, 24, 24 };
			clickable_inactive = { 20, 20, 20 };
			border_color = { 27, 27, 27 };

			text_color = { 200, 200, 200 };
			text_hovered = { 255, 255, 255 };
			text_inactive = { 60, 60, 60 };

			this->accent_color1 = NS_GUI::accent_color1;
			this->accent_color2 = NS_GUI::accent_color2;

			bold_text = false;
			small_text = false;
			scroll_fade = false;
			disable_scroll = false;
			transparent_clickable = false;
			gradient_text_in_button = false;

			rounding = 5.f;
			item_width = 248.f;

			m_callback = nullptr;

			tooltip = "";
			font = nullptr;
		}

		c_style() { this->reset(); }

		// Setting callback that will be called every owner's render
		// For global style it's called when is rendering instance (window)
		void set_callback(callback_t callback) { m_callback = callback; }
		void callback() {
			if (m_callback != nullptr)
				m_callback(*this);
		}

	protected:
		callback_t m_callback{};
	};

	class styles {
	private:
		static inline c_style style;

	public:
		static inline const c_style& get() { return style; }

		static inline void push(const c_style& s) { style = s; }
		static inline void reset() { push({}); }

		static inline void call_and_restore(std::function<void()> callback, const c_style& s) {
			c_style style_backup = std::move(style);
			style = s;
			callback();
			style = std::move(style_backup);
		}

		static void* get_font(void* font);

		static inline c_style& value() { return style; }
	};
} // namespace NS_GUI