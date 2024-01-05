#include "popups.hpp"
#include "../render_wrapper.hpp"

namespace NS_GUI::popups {
	void popup_t::render(render_context_t& ctx) {
		auto pos = this->calculate_position(ctx);

		const auto hovered = globals().is_mouse_in_region(pos, m_size);
		const auto click = globals().mouse_click[0];
		const auto active = this == ctx.m_instance->active_popup();

		if (active && (click || globals().mouse_click[1]) && !hovered)
			this->close();

		//	SET_AND_RESTORE(ctx.m_alpha, m_show_animation);
		SET_AND_RESTORE(ctx.m_can_update, active);

		// lerp visual size to real one
		{
			m_visual_size.x = m_size.x / dpi::_scale;
			//m_visual_size.y = m_size.y * (m_closed ? easings::in_cubic(m_show_animation) : (easings::out_cubic(m_show_animation)));
			animations_direct->ease_lerp(m_visual_size.y, m_closed ? 0.f : m_size.y / dpi::_scale, 10.f);
		}

		if (m_flags.has(container_flag_visible))
			render::filled_rect(pos, dpi::scale(m_visual_size), styles::get().group_background.modify_alpha(ctx.m_alpha), dpi::scale(styles::get().rounding));

		// set direction to vertical
		const vec2d base_position_backup = ctx.m_base_position;
		const auto direction_backup = ctx.m_direction;
		ctx.m_direction = e_items_direction::vertical;
		vec2d new_size{};
		// apply container size
		ctx.m_size = dpi::scale(m_visual_size - styles::get().container_padding);
		ctx.m_base_position = pos;
		ctx.m_in_popup = true;

		// store cursor position first
		vec2d backup_cursor = ctx.m_cursor;

		// apply padding & header height to cursor position
		if (!m_flags.has(container_no_padding))
			ctx.m_cursor += dpi::scale(styles::get().container_padding);

		// our "auto size calculation" give us size without padding in the end, so we need to add it
		// and don't forget that we have header

		render::lock(pos, dpi::scale(m_visual_size));
		new_size = containers::render_items(ctx, m_items, e_items_direction::vertical) + (m_flags.has(container_no_padding) ? vec2d{} : dpi::scale(styles::get().container_padding) * 2.f);
		render::unlock();

		if (active && hovered && ctx.m_hovered_item == nullptr)
			ctx.set_hovered_element(this);

		//if (active && ctx.m_hovered_item != nullptr)
		//	render::text(100, 100, {}, render::outline, fonts::menu_big, std::format("{:X}", (uintptr_t)ctx.m_hovered_item));

		ctx.m_in_popup = false;

		// restore cursor position then
		ctx.m_cursor = backup_cursor;
		ctx.m_direction = direction_backup;
		ctx.m_base_position = base_position_backup;

		if (m_size.x <= 0) m_size.x = new_size.x;
		if (m_size.y <= 0) m_size.y = new_size.y;

		if (active && m_flags & popup_flag_close_on_click && click)
			this->close();

		if (m_closed) {
			if (m_flags & popup_flag_close_animation) {
				m_show_animation.lerp(ctx, 0.f, 4.f);
				if (m_show_animation <= 0.f && m_visual_size.y < m_size.y * 0.5f)
					this->remove();
			} else
				this->remove();
		} else {
			if (m_flags & popup_flag_open_animation)
				m_show_animation.lerp(ctx, 1.f, 4.f);
			else
				m_show_animation.m_value = 1.f;
		}
	}

	void popup_t::remove() {
		if (m_on_delete != nullptr)
			m_on_delete();

		m_instance->delete_popup(this);
	}

	void popup_t::close() {
		m_closed = true;
	}
} // namespace NS_GUI::popups