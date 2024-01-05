#include "style.hpp"
#include "render_wrapper.hpp"

namespace NS_GUI {
	void* styles::get_font(void* font) {
		if (style.small_text)
			return style.bold_text ? fonts::menu_small_bold : fonts::menu_small;

		return font;
	}
} // namespace NS_GUI