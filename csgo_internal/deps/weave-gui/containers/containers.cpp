#include "containers.hpp"
#include "../instance/window.hpp"
#include "../render_wrapper.hpp"
#include "../shapes/shapes.hpp"

namespace NS_GUI::containers {
	vec2d render_items(render_context_t& ctx, const std::vector<i_renderable*>& items, e_items_direction items_direction) {
		const auto direction_backup = ctx.m_direction;
		ctx.m_direction = items_direction;

		if (items.empty())
			return { 0.f, 0.f };

		vec2d new_size{};
		for (auto item: items) {
			render_item(item, ctx);

			const auto a = ctx.apply(item, dpi::scale(styles::get().container_padding));
			switch (items_direction) {
				case e_items_direction::horizontal:
					if (a.y > new_size.y) new_size.y = a.y;
					new_size.x += a.x;
					break;
				case e_items_direction::vertical:
					if (a.x > new_size.x) new_size.x = a.x;
					new_size.y += a.y;
					break;
			}
		}

		ctx.m_direction = direction_backup;

		if (new_size.x > 0.f || new_size.y > 0.f) {
			// remove last added padding
			switch (items_direction) {
				case e_items_direction::horizontal:
					return new_size - vec2d{ dpi::scale(styles::get().container_padding.x), 0.f };
				case e_items_direction::vertical:
					return new_size - vec2d{ 0.f, dpi::scale(styles::get().container_padding.y) };
			}
		} else
			return new_size;

		return INVALID_POS;
	}

	template<>
	i_renderable* recursive_find(const std::vector<i_renderable*>& items, std::function<bool(i_renderable*)> cond) {
		for (auto item: items) {
			if (cond(item))
				return item;

			if (item->get_item_type() == e_item_type::container) {
				auto result = ((i_container*)item)->find(cond);
				if (result != nullptr)
					return result;
			}
		}

		return nullptr;
	}

	template<>
	i_renderable* recursive_find(const std::vector<popups::popup_t*>& popups, std::function<bool(i_renderable*)> cond) {
		for (auto popup: popups) {
			for (auto item: popup->get_items()) {
				if (cond(item))
					return item;

				if (item->get_item_type() == e_item_type::container) {
					auto result = ((i_container*)item)->find(cond);
					if (result != nullptr)
						return result;
				}
			}
		}

		return nullptr;
	}

	void recursive_delete(const std::vector<i_renderable*>& items) {
		for (auto item: items) {
			if (item->get_item_type() == e_item_type::container)
				((i_container*)item)->clear();

			delete item;
		}
	}

	void row_t::render(render_context_t& ctx) {
		vec2d backup_cursor = ctx.m_cursor, backup_size = ctx.m_size;
		m_hovered = globals().is_mouse_in_region(this->calculate_position(ctx), m_size);

		//render::rect(ctx.m_base_position + ctx.m_cursor, m_size, {});

		SET_AND_RESTORE(ctx.m_parent, this);

		if (!m_size.is_zero()) {
			if (m_items_align == e_items_align::center) {
				ctx.m_cursor.x += ctx.m_size.x * 0.5f - m_size.x * 0.5f;
				ctx.m_size.x = m_size.x;
				m_size = render_items(ctx, m_items, e_items_direction::horizontal);
			} else if (m_items_align == e_items_align::end) {
				ctx.m_cursor.x += ctx.m_size.x - m_size.x;
				ctx.m_size.x = m_size.x;
				m_size = render_items(ctx, m_items, e_items_direction::horizontal);
			} else if (m_items_align == e_items_align::start) {
				ctx.m_size.x = m_size.x;
				m_size = render_items(ctx, m_items, e_items_direction::horizontal);
			}
		} else
			m_size = render_items(ctx, m_items, e_items_direction::horizontal);

		ctx.m_size = backup_size;
		ctx.m_cursor.x = backup_cursor.x;
	}

	void column_t::render(render_context_t& ctx) {
		vec2d new_size{}, backup_cursor = ctx.m_cursor, backup_size = ctx.m_size;
		m_hovered = globals().is_mouse_in_region(this->calculate_position(ctx), m_size);

		// render::rect(ctx.m_base_position + ctx.m_cursor, m_size, {});

		SET_AND_RESTORE(ctx.m_parent, this);

		if (!m_size.is_zero()) {
			if (m_items_align == e_items_align::center) {
				if (ctx.m_size.y > m_size.y)
					ctx.m_cursor.y += ctx.m_size.y * 0.5f - m_size.y * 0.5f;

				ctx.m_size = m_size;
				// auto cursor_backup = ctx.m_base_position + ctx.m_cursor;
				new_size = render_items(ctx, m_items, e_items_direction::vertical);
			} else if (m_items_align == e_items_align::end) {
				ctx.m_cursor.y += ctx.m_size.y - m_size.y;
				ctx.m_size = m_size;
				new_size = render_items(ctx, m_items, e_items_direction::vertical);
			} else if (m_items_align == e_items_align::start) {
				ctx.m_size = m_size;
				new_size = render_items(ctx, m_items, e_items_direction::vertical);
			}
		} else
			new_size = render_items(ctx, m_items, e_items_direction::vertical);

		ctx.m_size = backup_size;
		ctx.m_cursor.y = backup_cursor.y;
		m_size = new_size;

		if (m_update_callback != nullptr) {
			containers::recursive_delete(m_items);
			m_items.clear();
			m_update_callback();

			ctx.m_hovered_item = nullptr;
			m_update_callback = nullptr;
		}
	}

	group_t::group_t(std::string title, vec2d size) {
		m_size = size;
		m_title = title;

		if (m_size.x <= 0.f)
			m_auto_size_x = true;
		if (m_size.y <= 0.f)
			m_auto_size_y = true;
	}

	void group_t::render(render_context_t& ctx) {
		const auto pos = this->calculate_position(ctx);
		const auto text_size = render::calc_text_size(m_title, fonts::menu_big);
		const auto header_height = m_title.empty() ? 0.f : text_size.y + dpi::scale(6.f);

		m_hovered = globals().is_mouse_in_region(this->calculate_position(ctx), m_size);
		SET_AND_RESTORE(ctx.m_parent, this);

		vec2d new_size{};
		const vec2d backup_size = ctx.m_size;

		vec2d size = m_size;
		if (!m_auto_size_x)
			size.x = m_size.x;

		if (!m_auto_size_y)
			size.y = m_size.y;

		ctx.m_size = size - dpi::scale(styles::get().container_padding) - vec2d{ 0.f, header_height };

		m_show_animation.lerp(ctx, m_loading ? 0.f : 1.f);
		m_skeleton_pulsating.trigger(ctx, m_loading, 1.5f);

		if (!m_title.empty() && m_show_animation > 0.f) {
			render::text(pos.x + dpi::scale(styles::get().container_padding.x), pos.y + dpi::scale(styles::get().container_padding.y) + header_height * 0.5f,
						 styles::get().text_inactive.modify_alpha(ctx.m_alpha), render::centered_y, fonts::menu_big, m_title.c_str());
		}

		const auto scroll_animation_scale = std::abs(m_scroll.m_offset_animated.y - m_scroll.m_offset.y) * 14.f;
		if (m_scroll.m_offset_animated.y > m_scroll.m_offset.y)
			m_scroll.m_offset_animated.y = std::clamp(m_scroll.m_offset_animated.y - ctx.m_time_delta * scroll_animation_scale, m_scroll.m_offset.y, FLT_MAX);
		else if (m_scroll.m_offset_animated.y < m_scroll.m_offset.y)
			m_scroll.m_offset_animated.y = std::clamp(m_scroll.m_offset_animated.y + ctx.m_time_delta * scroll_animation_scale, 0.f, m_scroll.m_offset.y);

		// store cursor position first
		vec2d backup_cursor = ctx.m_cursor;
		if (m_position_type == e_position_type::absolute) {
			ctx.m_cursor = pos - ctx.m_base_position;
			ctx.m_cursor += dpi::scale(styles::get().container_padding) + vec2d{ 0.f, header_height };
			ctx.m_cursor.y -= (int)m_scroll.m_offset_animated.y;
			//vec2d old_cursor = ctx.m_base_position + ctx.m_cursor;

			render::lock(pos, size);
			new_size = render_items(ctx, m_items, e_items_direction::vertical) +
					   vec2d{ dpi::scale(styles::get().container_padding.x), header_height + dpi::scale(styles::get().window_padding.y * 0.5f) };

			render::unlock();

			//render::rect(old_cursor, new_size, {});
		} else {
			if (m_show_animation < 1.f && m_loading) {
				auto skeleton_color = color_t{ 100, 100, 100 }.modify_alpha((1.f - m_show_animation) * 0.1f + m_skeleton_pulsating * 0.25f);
				auto skeleton_pos = pos + dpi::scale(styles::get().container_padding) + vec2d{ 0.f, header_height };

				if (m_size.x > dpi::scale(styles::get().item_width + 32.f)) {
					{
						float offset = 0.f;

						// group name
						render::filled_rect(skeleton_pos, dpi::scale(vec2d{ 83.f, 18.f }), skeleton_color, dpi::scale(5.f));

						offset += 32.f;

						render::filled_rect(skeleton_pos + dpi::scale(vec2d{ 0.f, offset }), dpi::scale(vec2d{ 248.f, 32.f }), skeleton_color, dpi::scale(5.f));

						offset += 40.f;

						{

							for (int i = 1; i <= 3; ++i) {
								render::filled_rect(skeleton_pos + dpi::scale(vec2d{ 0.f, offset }),
													dpi::scale(vec2d{ 100.f, 16.f }), skeleton_color, dpi::scale(5.f));

								offset += 24.f;

								render::filled_rect(skeleton_pos + dpi::scale(vec2d{ 0.f, offset }),
													dpi::scale(vec2d{ 248.f, 32.f }), skeleton_color, dpi::scale(5.f));

								offset += 40.f;
							}
						}
					}

					// now render right side
					skeleton_pos.x += dpi::scale(styles::get().item_width + 32.f);

					{
						float offset = 0.f;

						// group name
						render::filled_rect(skeleton_pos, dpi::scale(vec2d{ 83.f, 18.f }), skeleton_color, dpi::scale(5.f));

						offset += 32.f;

						render::filled_rect(skeleton_pos + dpi::scale(vec2d{ 0.f, offset }), dpi::scale(vec2d{ 248.f, 32.f }), skeleton_color, dpi::scale(5.f));

						offset += 40.f;
						for (int i = 1; i <= 4; ++i) {
							render::filled_rect(skeleton_pos + dpi::scale(vec2d{ 0.f, offset }),
												dpi::scale(vec2d{ 248.f, 32.f }), skeleton_color, dpi::scale(5.f));

							offset += 40.f;
							render::filled_rect(skeleton_pos + dpi::scale(vec2d{ 0.f, offset }),
												dpi::scale(vec2d{ 100.f, 16.f }), skeleton_color, dpi::scale(5.f));

							offset += 24.f;
						}

						render::filled_rect(skeleton_pos + dpi::scale(vec2d{ 0.f, offset }),
											dpi::scale(vec2d{ 100.f, 16.f }), skeleton_color, dpi::scale(5.f));
					}
				} else {
					render::ignore_lock([&]() {
						float offset = 8.f;

						if (!m_title.empty()) {
							// group name
							render::filled_rect(skeleton_pos, dpi::scale(vec2d{ 83.f, 18.f }), skeleton_color, dpi::scale(5.f));

							offset += 32.f;
						}

						render::filled_rect(skeleton_pos + dpi::scale(vec2d{ 0.f, offset }), dpi::scale(vec2d{ 248.f, 32.f }), skeleton_color, dpi::scale(5.f));

						offset += 40.f;

						{

							for (int i = 1; i <= 3; ++i) {
								render::filled_rect(skeleton_pos + dpi::scale(vec2d{ 0.f, offset }),
													dpi::scale(vec2d{ 100.f, 16.f }), skeleton_color, dpi::scale(5.f));

								offset += 24.f;

								render::filled_rect(skeleton_pos + dpi::scale(vec2d{ 0.f, offset }),
													dpi::scale(vec2d{ 248.f, 32.f }), skeleton_color, dpi::scale(5.f));

								offset += 40.f;
							}
						}
					});
				}
			}

			//render::rect(pos, size, {});

			if (!m_items.empty() && !m_loading && m_show_animation > 0.f) {
				// apply padding & header height to cursor position
				ctx.m_cursor += dpi::scale(styles::get().container_padding) + vec2d{ 0.f, header_height };
				ctx.m_cursor.y -= (int)m_scroll.m_offset_animated.y;

				render::lock(pos, size);

				// our "auto size calculation" give us size without padding in the end, so we need to add it
				// and don't forget that we have header
				new_size = render_items(ctx, m_items, e_items_direction::vertical) + vec2d{ dpi::scale(styles::get().container_padding.x), header_height + dpi::scale(styles::get().window_padding.y * 0.5f) };

				render::unlock();
			}
		}

		if (!m_auto_size_y) {
			const auto scroll_max = new_size.y - ctx.m_size.y;
			const auto scroll_unclamped = m_scroll.m_offset.y;
			const auto can_scroll = (ctx.m_size.x >= 0.f || ctx.m_size.y >= 0.f) && new_size.y > ctx.m_size.y && !styles::get().disable_scroll;

			if (can_scroll) {
				if (styles::get().scroll_fade) {
					if (scroll_unclamped < scroll_max) {
						render::filled_rect_gradient(pos.x, pos.y + size.y - dpi::scale(30.f), size.x, dpi::scale(30.f),
													 styles::get().window_backround.modify_alpha(0.f), styles::get().window_backround.modify_alpha(0.f),
													 styles::get().window_backround.modify_alpha(ctx.m_alpha), styles::get().window_backround.modify_alpha(ctx.m_alpha));
					}

					if (scroll_unclamped > 0.f) {
						render::filled_rect_gradient(pos.x, pos.y - 1, size.x, dpi::scale(30.f),
													 styles::get().window_backround.modify_alpha(ctx.m_alpha), styles::get().window_backround.modify_alpha(ctx.m_alpha),
													 styles::get().window_backround.modify_alpha(0.f), styles::get().window_backround.modify_alpha(0.f));
					}
				}

				if (!ctx.m_scrolled_this_frame && globals().is_mouse_in_region(pos, new_size) && (!ctx.m_instance->has_popups() || ctx.m_in_popup)) {
					const auto mouse_wheel_delta = dpi::scale(vec2d{ 0.f, globals().mouse_wheel * 30.f });

					if (mouse_wheel_delta.length_sqr() > 0) {
						m_scroll.m_offset -= mouse_wheel_delta;
						m_scroll.m_last_scroll_time = globals().get_time();
					}
					ctx.m_scrolled_this_frame = true;
					m_scroll.m_offset.y = std::clamp(m_scroll.m_offset.y, 0.f, scroll_max);

					//if (styles::get().scroll_fade) {
					//	const auto scroll_pos = m_scroll.m_offset_animated.y / new_size.y * size.y;
					//	const auto scroll_pos_max = scroll_max / (float)new_size.y * size.y;
					//	const auto scroll_alpha = 1.f - std::clamp((globals().get_time() - m_scroll.m_last_scroll_time) / 10.f, 0.f, 1.f);

					//	render::filled_rect(pos.x + size.x - dpi::scale(6.f), pos.y + scroll_pos, dpi::scale(3.f), size.y - scroll_pos_max,
					//		styles::get().border_color.modify_alpha(ctx.m_alpha));
					//}

					/*if (scroll_unclamped > scroll_max && std::abs(m_scroll.m_offset_animated.y - m_scroll.m_offset.y) <= 5.f)
						m_scroll.m_limit_animation = 1.f;
					else if (scroll_unclamped < 0.f && std::abs(m_scroll.m_offset_animated.y - m_scroll.m_offset.y) <= 5.f)
						m_scroll.m_limit_animation = 1.f;

					m_scroll.m_limit_animation = std::max<float>(m_scroll.m_limit_animation - ctx.m_time_delta, 0.f);
					const auto limit_color = color_t{ 28, 28, 28 }.modify_alpha(m_scroll.m_limit_animation * ctx.m_alpha);

					if (m_scroll.m_offset.y == scroll_max) {
						render::filled_rect_gradient(pos.x, pos.y + size.y - dpi::scale(30.f), size.x, dpi::scale(30.f),
							limit_color.modify_alpha(0.f), limit_color.modify_alpha(0.f), limit_color, limit_color);
					}
					else if (m_scroll.m_offset.y == 0.f) {
						render::filled_rect_gradient(pos.x, pos.y, size.x, dpi::scale(30.f),
							limit_color, limit_color, limit_color.modify_alpha(0.f), limit_color.modify_alpha(0.f));
					}*/
				}
			} else
				m_scroll.m_offset = {};
		}

		// restore cursor position then
		ctx.m_cursor = backup_cursor;
		ctx.m_size = backup_size;

		if (m_auto_size_x)
			m_size.x = new_size.x;
		if (m_auto_size_y)
			m_size.y = new_size.y;

		if (m_update_callback != nullptr) {
			containers::recursive_delete(m_items);
			m_items.clear();
			m_update_callback();

			ctx.m_hovered_item = nullptr;
			m_update_callback = nullptr;

			m_scroll.reset();
		}
	}
} // namespace NS_GUI::containers