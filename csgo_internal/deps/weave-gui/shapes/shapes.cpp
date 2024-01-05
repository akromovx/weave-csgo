#include "shapes.hpp"
#include "../render_wrapper.hpp"

namespace NS_GUI::shapes {
	void rect_t::render(render_context_t& ctx) {
		const auto pos = this->calculate_position(ctx) + dpi::scale(vec2d{ 0.f, styles::get().controls_text_padding.y * 2.f });
		const auto size = this->get_size();

		if (m_texture != nullptr)
			render::image(pos, size, ((render::gif_t*)m_texture)->m_textures[0].first);
		else
			render::filled_rect(pos, size, m_color.modify_alpha(ctx.m_alpha));
	}
}// namespace NS_GUI::shapes