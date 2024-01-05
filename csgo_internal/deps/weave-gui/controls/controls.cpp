#include "controls.hpp"
#include "../instance.hpp"
#include "../popups/popups.hpp"
#include "../render_wrapper.hpp"

#include "../../../src/features/network.hpp"
#include "../../../src/utils/hotkeys.hpp"

#define FIRST_FRAME(t) ((render::gif_t*)t)->m_textures[0].first

static void to_clipboard(const std::string& text) {
	if (OpenClipboard(0)) {
		EmptyClipboard();
		auto clip_data = (char*)(GlobalAlloc(GMEM_FIXED, MAX_PATH));
		lstrcpyA(clip_data, text.c_str());
		SetClipboardData(CF_TEXT, (HANDLE)(clip_data));
		LCID* lcid = (DWORD*)(GlobalAlloc(GMEM_FIXED, sizeof(DWORD)));
		*lcid = MAKELCID(MAKELANGID(LANG_RUSSIAN, SUBLANG_NEUTRAL), SORT_DEFAULT);
		SetClipboardData(CF_LOCALE, (HANDLE)(lcid));
		CloseClipboard();
	}
}

static std::string from_clipboard() {
	std::string result;

	if (OpenClipboard(0)) {
		HANDLE data = GetClipboardData(CF_TEXT);
		auto buffer = (char*)GlobalLock(data);
		if (buffer == nullptr)
			return "";

		result = buffer;
		GlobalUnlock(data);
		CloseClipboard();
	}

	return result;
}

static void ease_lerp_float(float& value, float target, float scale = 9.f) {
	if (value > target)
		value = std::clamp(value - std::abs(value - target) * gui::globals().time_delta * scale, target, FLT_MAX);
	else if (value < target)
		value = std::clamp(value + std::abs(value - target) * gui::globals().time_delta * scale, -FLT_MAX, target);
};

namespace NS_GUI::controls {
	i_renderable* prompt(const std::string& name, const std::string& value) {
		auto r = new containers::row_t{};
		c_style s{};
		s.container_padding = {};
		r->set_style(s);
		auto name_item = new text_t{ name };
		s.text_color = { 127, 127, 127 };
		name_item->set_style(s);
		r->add(name_item);
		r->add(new text_t{ value });
		return r;
	}

	i_renderable* hotkey(const std::string& name, int* type, uint16_t* key, void* category, std::function<void()> nested_items, std::function<void()> on_bind) {
		auto r = new containers::row_t{};
		r->override_style().container_padding = { 8.f, 0.f };

		r->add(button(name, nested_items, vec2d{ 192.f, 32.f } * dpi::_get_actual_scale(), e_items_align::start, category));

		auto type_selector = dropbox(STRS("Type"), type, { STRS("Hold"), STRS("Toggle"), STRS("Always on") }, false,
									 [on_bind](bool updated) {
										 if (!updated)
											 return;

										 on_bind();
									 });

		type_selector->override_style().item_width = 196.f;
		r->add(type_selector);
		r->add(key_binder(key, on_bind));

		return r;
	}

	void image_t::render(render_context_t& ctx) {
		if (m_texture != nullptr) {
			if (m_rounded) {
				auto pos = this->calculate_position(ctx);

				render::gif((render::gif_t*)m_texture, pos, m_size, color_t{}.modify_alpha(ctx.m_alpha), m_size.x * 0.5f);
			} else
				render::gif((render::gif_t*)m_texture, this->calculate_position(ctx), m_size, color_t{}.modify_alpha(ctx.m_alpha));
		}
	}

	void loading_t::render(render_context_t& ctx) {
		const auto is_loading = m_is_loading != nullptr && m_is_loading();
		m_show_animation.lerp(ctx, is_loading);

		if (m_show_animation == 0.f) {
			m_size = {};
			return;
		}

		m_size = dpi::scale(vec2d{ 12.f, 12.f });

		auto loading_texture = (render::gif_t*)resources::get_texture(HASH("loading"));
		if (loading_texture != nullptr) {
			m_cycle += ctx.m_time_delta;
			while (m_cycle > 1.f)
				m_cycle -= 1.f;

			render::image_rotated(loading_texture->m_textures[0].first, this->calculate_position(ctx) + m_size * 0.5f, m_size, -m_cycle * f2PI,
								  color_t{}.modify_alpha(ctx.m_alpha * m_show_animation));
		}
	}

	void text_t::render(render_context_t& ctx) {
		auto font = styles::get_font(m_bold || styles::get().bold_text ? fonts::menu_bold : fonts::menu_main);
		m_size = render::calc_text_size(m_value, font, ctx.m_item_width) + styles::get().container_padding;
		auto pos = this->calculate_position(ctx);
		render::text(pos.x, pos.y, styles::get().text_color.modify_alpha(ctx.m_alpha), render::none, font, m_value, ctx.m_item_width);
	}

	void checkbox_t::render(render_context_t& ctx) {
		auto enabled = *m_value;

		auto font = styles::get_font(/*enabled || */ styles::get().bold_text ? fonts::menu_bold : fonts::menu_main);
		const auto text_size = render::calc_text_size(m_name, font);

		m_size = { dpi::scale(styles::get().item_width), dpi::scale(16.f) + dpi::scale(styles::get().controls_text_padding.y * 2.f) };
		//m_size = { dpi::scale(styles::get().item_width), dpi::scale(control_height) };

		auto pos = this->calculate_position(ctx);

		m_hovered = globals().is_mouse_in_region(pos, m_size);
		if (m_hovered)
			ctx.set_hovered_element(this);

		m_hovered = m_hovered && ctx.can_update(this);
		if (m_hovered) {
			if (globals().mouse_click[0]) {
				*m_value = !(*m_value);
				enabled = *m_value;

				globals().reset_mouse();

				if (m_on_change != nullptr)
					m_on_change();
			}
		}

		m_hover_animation.lerp(ctx, m_hovered);

		ease_lerp_float(m_switch_animation.m_value, enabled ? 1.f : 0.f);

		pos += vec2d{ 0.f, m_size.y * 0.5f - field_size.y * 0.5f };

		if (!styles::get().transparent_clickable) {
			render::filled_rect(pos, dpi::scale(field_size),
								color_t::lerp(styles::get().clickable_color, styles::get().clickable_hovered, m_hover_animation).modify_alpha(ctx.m_alpha), dpi::scale(8.f));

			render::rect(pos, dpi::scale(field_size), styles::get().border_color.new_alpha(60).modify_alpha(ctx.m_alpha * m_hover_animation), dpi::scale(8.f));

			const auto color1 = color_t::lerp(styles::get().accent_color1, styles::get().accent_color2, m_hover_animation);
			const auto color2 = color_t::lerp(styles::get().accent_color2, styles::get().accent_color1, m_hover_animation);

			ImDrawGradient_Linear gradient{ to_imvec2(pos), to_imvec2(pos + dpi::scale(field_size)),
											to_imvec4(color1.modify_alpha(m_switch_animation * ctx.m_alpha)),
											to_imvec4(color2.modify_alpha(m_switch_animation * ctx.m_alpha)) };

			render::draw_list->AddRectFilled(to_imvec2(pos), to_imvec2(pos + dpi::scale(field_size)), gradient, dpi::scale(8.f));

			const float ease_animation = m_switch_animation /**m_value ? easings::out_cubic(m_switch_animation) : easings::in_cubic(m_switch_animation)*/;
			render::circle_filled(pos.x + dpi::scale(field_padding_x + checkmark_size.x + ease_animation * (field_size.x - checkmark_size.x * 2.f - field_padding_x * 2.f)),
								  pos.y + dpi::scale(field_size.y) * 0.5f, dpi::scale(checkmark_size.x), color_t::lerp({ 60, 60, 60 }, { 255, 255, 255 }, m_switch_animation), 20);
		}

		render::text(
				pos.x + dpi::scale(field_size).x + dpi::scale(styles::get().controls_text_padding.x),
				pos.y + dpi::scale(field_size).y * 0.5f,
				color_t::lerp(styles::get().text_color, styles::get().text_hovered, m_hover_animation).modify_alpha(ctx.m_alpha * std::lerp(0.65f, 1.f, m_switch_animation)),
				render::centered_y, font, m_name);
	}

	void button_t::render(render_context_t& ctx) {
		auto font = styles::get_font(m_colored || styles::get().bold_text ? fonts::menu_bold : fonts::menu_main);

		// if size wasn't defined by user
		if (m_size.x <= 0.f || m_size.y <= 0.f) {
			const auto text_size = render::calc_text_size(m_name, font);
			auto new_size = vec2d{ text_size.x, text_size.y } + dpi::scale(styles::get().controls_text_padding * 2.f);

			if (m_size.x <= 0.f) m_size.x = new_size.x;
			if (m_size.y <= 0.f) m_size.y = new_size.y;
		}

		const auto has_callback = m_callback != nullptr;
		const auto pos = this->calculate_position(ctx) + dpi::scale(vec2d{ 0.f, styles::get().controls_text_padding.y });
		const auto size = m_size - (m_position_type == e_position_type::absolute ? 0.f : dpi::scale(vec2d{ 0.f, styles::get().controls_text_padding.y * 2.f }));

		m_hovered = has_callback && globals().is_mouse_in_region(pos, size);
		if (m_hovered)
			ctx.set_hovered_element(this);

		m_hovered = m_hovered && ctx.can_update(this);
		m_animation.lerp(ctx, m_hovered);

		if (m_hovered) {
			if (globals().mouse_click[0]) {
				m_callback();
				globals().reset_mouse();
			}
		}

		if (!styles::get().transparent_clickable) {
			if (m_colored && has_callback) {
				const auto color1 = color_t::lerp(styles::get().accent_color1, styles::get().accent_color2, m_animation);
				const auto color2 = color_t::lerp(styles::get().accent_color2, styles::get().accent_color1, m_animation);

				ImDrawGradient_Linear gradient{ to_imvec2(pos), to_imvec2(pos + size),
												to_imvec4(color1.modify_alpha(ctx.m_alpha)),
												to_imvec4(color2.modify_alpha(ctx.m_alpha)) };

				render::draw_list->AddRectFilled(to_imvec2(pos), to_imvec2(pos + size), gradient, dpi::scale(styles::get().rounding));
			} else {
				const auto color = has_callback ? color_t::lerp(styles::get().clickable_color, styles::get().clickable_hovered, m_animation) : styles::get().clickable_inactive;
				render::filled_rect(pos, size, color.modify_alpha(ctx.m_alpha), dpi::scale(styles::get().rounding));
				render::rect(pos, size, styles::get().border_color.modify_alpha(m_animation * ctx.m_alpha), dpi::scale(styles::get().rounding));
			}
		} else {
			if (!styles::get().invisible_clickable)
				render::filled_rect(pos, size, styles::get().clickable_hovered.modify_alpha(ctx.m_alpha * 0.1f * m_animation), dpi::scale(styles::get().rounding));
		}

		constexpr vec2d icon_size = { 16.f, 16.f };
		const auto text_color = m_colored ? color_t{ 255, 255, 255 }.modify_alpha(ctx.m_alpha)
										  : color_t::lerp(styles::get().text_color, styles::get().text_hovered, m_animation).modify_alpha(ctx.m_alpha);

		switch (m_text_align) {
			case e_items_align::start: {
				vec2d base_pos = { pos.x + dpi::scale(styles::get().controls_text_padding.x), pos.y + size.y * 0.5f };

				if (m_icon != nullptr) {
					render::gif((render::gif_t*)m_icon, base_pos - vec2d{ 0.f, dpi::scale(icon_size.y) * 0.5f }, dpi::scale(icon_size), text_color);
					base_pos.x += dpi::scale(icon_size.x) + dpi::scale(styles::get().controls_text_padding.x);
				}

				if (styles::get().gradient_text_in_button) {
					const auto text_size = render::calc_text_size(m_name, font);
					const auto text_pos = vec2d{ base_pos.x, base_pos.y - text_size.y * 0.5f };
					ImDrawGradient_Linear gradient{ to_imvec2(text_pos), to_imvec2(text_pos + text_size), to_imvec4(styles::get().accent_color1.modify_alpha(ctx.m_alpha)), to_imvec4(styles::get().accent_color2.modify_alpha(ctx.m_alpha)) };

					auto pfont = (ImFont*)font;
					render::draw_list->PushTextureID(pfont->ContainerAtlas->TexID);
					render::draw_list->AddText(pfont, pfont->FontSize, to_imvec2(text_pos), gradient, m_name.c_str());
					render::draw_list->PopTextureID();
				} else
					render::text(base_pos.x, base_pos.y, text_color, render::centered_y, font, m_name);
			} break;
			case e_items_align::end:
				render::text(pos.x + size.x - dpi::scale(styles::get().controls_text_padding.x), pos.y + size.y * 0.5f, text_color, render::align_left | render::centered_y, font, m_name);
				break;
			default:
			case e_items_align::center:

				if (m_name.empty()) {
					/*render::image({ pos.x + size.x * 0.5f - dpi::scale(icon_size.x) * 0.5f,
									pos.y + size.y * 0.5f - dpi::scale(icon_size.y) * 0.5f },
								  dpi::scale(icon_size), FIRST_FRAME(m_icon), text_color);*/

					render::gif((render::gif_t*)m_icon,
								{ pos.x + size.x * 0.5f - dpi::scale(icon_size.x) * 0.5f,
								  pos.y + size.y * 0.5f - dpi::scale(icon_size.y) * 0.5f },
								dpi::scale(icon_size), text_color);
				} else {
					if (m_icon != nullptr) {
						auto text_size = render::calc_text_size(m_name, font);
						auto width = text_size.x + dpi::scale(icon_size.x + styles::get().controls_text_padding.x);

						render::gif((render::gif_t*)m_icon, { pos.x + size.x * 0.5f + width * 0.5f - dpi::scale(icon_size.x), pos.y + size.y * 0.5f - dpi::scale(icon_size.y * 0.5f) },
									dpi::scale(icon_size), text_color);

						render::text(
								pos.x + size.x * 0.5f - text_size.x * 0.5f - dpi::scale((styles::get().controls_text_padding.x + icon_size.x) * 0.5f),
								pos.y + size.y * 0.5f, text_color,
								render::centered_y, font, m_name);
					} else
						render::text(pos.x + size.x * 0.5f, pos.y + size.y * 0.5f, text_color, render::centered, font, m_name);
				}

				break;
		}
	}

	void collapse_t::render(render_context_t& ctx) {
		auto font = styles::get_font(m_colored || styles::get().bold_text ? fonts::menu_bold : fonts::menu_main);

		// if size wasn't defined by user
		if (m_actual_size.x <= 0.f || m_actual_size.y <= 0.f) {
			const auto text_size = render::calc_text_size(m_name, font);
			auto new_size = vec2d{ text_size.x, text_size.y + dpi::scale(styles::get().controls_text_padding.y) } + dpi::scale(styles::get().controls_text_padding * 2.f);

			if (m_actual_size.x <= 0.f) m_actual_size.x = new_size.x;
			if (m_actual_size.y <= 0.f) m_actual_size.y = new_size.y;
		}

		const auto has_callback = m_callback != nullptr;
		const auto pos = this->calculate_position(ctx) + dpi::scale(vec2d{ 0.f, styles::get().controls_text_padding.y });
		const auto size = m_actual_size - (m_position_type == e_position_type::absolute ? 0.f : dpi::scale(vec2d{ 0.f, styles::get().controls_text_padding.y * 2.f }));

		m_hovered = has_callback && globals().is_mouse_in_region(pos, size);
		if (m_hovered)
			ctx.set_hovered_element(this);

		m_hovered = m_hovered && ctx.can_update(this);

		if (m_hovered) {
			if (globals().mouse_click[0]) {
				m_opened = !m_opened;
				m_callback();
				globals().reset_mouse();
			}
		}

		m_animation.lerp(ctx, m_hovered /*|| m_opened*/);

		ease_lerp_float(m_open_animation.m_value, m_opened ? 1.f : 0.f);

		if (!styles::get().transparent_clickable) {
			const auto color = has_callback ? color_t::lerp(styles::get().clickable_color, styles::get().clickable_hovered, m_animation) : styles::get().clickable_inactive;
			render::filled_rect(pos, size, color.modify_alpha(ctx.m_alpha), dpi::scale(styles::get().rounding));
			render::rect(pos, size, styles::get().border_color.modify_alpha(m_animation * ctx.m_alpha), dpi::scale(styles::get().rounding));
		} else {
			if (!styles::get().invisible_clickable)
				render::filled_rect(pos, size, color_t{ 100, 100, 100 }.modify_alpha(ctx.m_alpha * 0.1f * m_animation), dpi::scale(styles::get().rounding));
		}

		const auto text_color = m_colored ? color_t{ 255, 255, 255 }.modify_alpha(ctx.m_alpha)
										  : color_t::lerp(styles::get().text_color, styles::get().text_hovered, m_animation).modify_alpha(ctx.m_alpha);

		switch (m_text_align) {
			case e_items_align::start: {
				vec2d base_pos = { pos.x + dpi::scale(styles::get().controls_text_padding.x), pos.y + size.y * 0.5f };
				constexpr vec2d icon_size = { 16.f, 16.f };

				if (m_icon != nullptr) {
					render::gif((render::gif_t*)m_icon, base_pos - vec2d{ 0.f, dpi::scale(icon_size.y) * 0.5f }, dpi::scale(icon_size), text_color);
					base_pos.x += dpi::scale(icon_size.x) + dpi::scale(styles::get().controls_text_padding.x);
				}

				render::text(base_pos.x, base_pos.y, text_color, render::centered_y, font, m_name);
			} break;
			case e_items_align::end:
				render::text(pos.x + size.x - dpi::scale(styles::get().controls_text_padding.x), pos.y + size.y * 0.5f, text_color, render::align_left | render::centered_y, font, m_name);
				break;
			case e_items_align::center:
				[[fallthrough]];
			default:
				render::text(pos.x + size.x * 0.5f, pos.y + size.y * 0.5f, text_color, render::centered, font, m_name);
				break;
		}

		auto arrow_texture = (render::gif_t*)resources::get_texture(HASH("arrow_dropbox"));
		if (arrow_texture != nullptr)
			render::image_rotated(arrow_texture->m_textures[0].first, pos + vec2d{ size.x - dpi::scale(16.f), size.y - dpi::scale(16.f) }, dpi::scale(vec2d{ 32.f, 32.f }),
								  m_open_animation * fPI, color_t{}.modify_alpha(ctx.m_alpha));

		m_size = m_actual_size;
		if (m_item != nullptr && m_open_animation > 0.f) {
			const auto items_direction = e_items_direction::vertical;
			const auto direction_backup = ctx.m_direction;
			const auto animation = m_open_animation;
			const auto padding = animation * dpi::scale(8.f);
			const auto this_height = m_actual_size.y * animation + padding;

			ctx.m_direction = items_direction;

			render::lock(ctx.m_base_position + ctx.m_cursor + vec2d{ 0.f, m_actual_size.y + padding },
						 m_item->get_size() + padding);

			SET_AND_RESTORE(ctx.m_alpha, std::min<float>(ctx.m_alpha, animation));
			SET_AND_RESTORE(ctx.m_cursor.y, ctx.m_cursor.y + this_height);
			SET_AND_RESTORE(ctx.m_can_update, ctx.m_alpha >= 0.95f);

			if (ctx.m_alpha > 0.f) {
				render_item(m_item, ctx);
				m_size.y += m_item->get_size().y * animation + padding;
			}

			render::unlock();
			ctx.m_direction = direction_backup;
		}
	}

	void selectables_t::render(render_context_t& ctx) {
		if (m_items.empty())
			return;

		const auto width = m_width != -1.f ? m_width : dpi::scale(styles::get().item_width);
		const auto base_pos = this->calculate_position(ctx);

		bool any_hovered = false;
		float current_height = 0.f;

		for (auto i = 0u; i < m_items.size(); ++i) {
			const auto& item = m_items[i];

			if (m_search_phrase != nullptr) {
				std::string it = item;
				it.erase(std::remove_if(it.begin(), it.end(), [](char c) { return !((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9')); }), it.end());
				std::transform(it.begin(), it.end(), it.begin(), [](char c) -> char { return c >= 'A' && c <= 'Z' ? (c + ('a' - 'A')) : c; });
				if (it.find(*m_search_phrase) == std::string::npos)
					continue;
			}

			auto& anims = m_items_animation[i];
			const auto pos = base_pos + vec2d{ 0.f, current_height };
			const auto flag = 1 << i;

			const bool active = m_multiselect ? *m_value & flag : *m_value == i;

			auto font = styles::get().font != nullptr ? styles::get().font : styles::get_font(/*active || */ styles::get().bold_text ? fonts::menu_bold : fonts::menu_main);
			const auto text_size = render::calc_text_size(item, font);
			const auto height = dpi::scale(16.f) + text_size.y;

			m_hovered = !any_hovered && globals().is_mouse_in_region(pos, vec2d{ width, height });
			if (m_hovered)
				ctx.set_hovered_element(this);

			m_hovered = m_hovered && ctx.can_update(this);

			anims.first.lerp(ctx, m_hovered);
			anims.second.lerp(ctx, active, 4.f);

			if (m_hovered) {
				any_hovered = true;

				if (globals().mouse_click[0]) {
					globals().reset_mouse();

					if (m_multiselect) {
						const bool value_changed = (*m_value & flag) != flag;
						*m_value = *m_value & flag ? (*m_value & ~flag) : (*m_value | flag);

						if (m_on_click != nullptr)
							m_on_click(value_changed);
					} else {
						const bool value_changed = *m_value != i;
						*m_value = i;
						if (m_on_click != nullptr)
							m_on_click(value_changed);
					}
				}
			}

			color_t clr = color_t::lerp(styles::get().clickable_color, styles::get().clickable_hovered, anims.first);
			int rflags = ImDrawCornerFlags_None;
			if (i == 0)
				rflags |= ImDrawCornerFlags_Top;
			if (i == m_items.size() - 1)
				rflags |= ImDrawCornerFlags_Bot;

			if (!styles::get().transparent_clickable)
				render::filled_rect(pos, vec2d{ width, height }, clr.modify_alpha(ctx.m_alpha), dpi::scale(styles::get().rounding), rflags);

			vec2d text_pos = { pos.x + dpi::scale(styles::get().controls_text_padding.x), pos.y + dpi::scale(8.f) };

			render::lock(pos, vec2d{ width, height });

			const auto text_color = styles::get().text_color.lerp(styles::get().text_hovered, anims.first);

			ImDrawGradient_Linear gradient{
				to_imvec2(text_pos),
				{ text_pos.x + text_size.x, text_pos.y + text_size.y },
				to_imvec4(text_color.lerp(styles::get().accent_color1, anims.second).modify_alpha(ctx.m_alpha)),
				to_imvec4(text_color.lerp(styles::get().accent_color2, anims.second).modify_alpha(ctx.m_alpha))
			};

			auto pfont = (ImFont*)font;
			render::draw_list->PushTextureID(pfont->ContainerAtlas->TexID);
			render::draw_list->AddText(pfont, pfont->FontSize, to_imvec2(text_pos), gradient, item.c_str());
			render::draw_list->PopTextureID();

			render::unlock();

			current_height += std::round(height - 0.5f);
		}

		m_size = vec2d{ width, current_height };
	}

	void dropbox_t::render(render_context_t& ctx) {
		m_size = { dpi::scale(styles::get().item_width), dpi::scale(control_height) + dpi::scale(styles::get().controls_text_padding.y * 1.5f) };

		const auto pos = this->calculate_position(ctx) + dpi::scale(vec2d{ 0.f, styles::get().controls_text_padding.y });
		const auto size = m_size - dpi::scale(vec2d{ 0.f, styles::get().controls_text_padding.y * 1.6f });

		m_hovered = globals().is_mouse_in_region(pos, size);
		if (m_hovered)
			ctx.set_hovered_element(this);

		m_hovered = m_hovered && ctx.can_update(this);
		m_hover_animation.lerp(ctx, m_hovered);

		if (m_hovered) {
			if (globals().mouse_click[0]) {
				if (m_popup == nullptr) {
					m_popup = ctx.m_instance->create_popup();

					const auto additional_flag = !m_multiselect ? popup_flag_close_on_click : 0;
					m_popup->m_flags.add(container_flag_is_popup | container_no_padding | popup_flag_animation | additional_flag);

					m_popup->set_position(pos - ctx.m_instance->get_position() + dpi::scale(vec2d{ 0.f, control_height + styles::get().controls_text_padding.y * 0.5f }));

					m_popup->set_size(vec2d{ dpi::scale(styles::get().item_width), m_items.size() < 5 ? 0.f : dpi::scale(160.f) });
					m_popup->on_delete([this]() { m_popup = nullptr; });

					if (m_items.size() > 12) {
						auto g = new containers::group_t{ "", { 0.f, dpi::scale(160.f) } };
						g->override_style().container_padding = {};
						g->override_style().scroll_fade = true;

						//group->override_style().window_padding = {};

						m_search_phrase.clear();
						auto search = text_input(&m_search_phrase, STRS("\xef\x80\x82   Search..."));
						g->add(selectables(m_value, m_items, m_multiselect, dpi::scale(styles::get().item_width), m_on_click, &m_search_phrase));

						m_popup->add(search);
						m_popup->add(g);
						m_popup->m_flags.remove(popup_flag_close_on_click);
						m_popup->m_flags.add(container_flag_visible);
					} else {
						auto g = new containers::group_t{ "", { 0.f, m_items.size() < 5 ? 0.f : dpi::scale(160.f) } };
						g->override_style().container_padding = {};
						g->override_style().window_padding = {};
						g->override_style().scroll_fade = true;

						g->add(new selectables_t{ m_value, m_items, m_multiselect, dpi::scale(styles::get().item_width), m_on_click });
						m_popup->add(g);
					}

					globals().reset_mouse();
				} else {
					m_popup->close();
					m_popup = nullptr;
				}
			}
		}

		const auto is_opened = m_popup != nullptr;
		m_open_animation.lerp(ctx, is_opened, 1.75f);
		const auto this_color = color_t::lerp(styles::get().clickable_color, styles::get().clickable_hovered, m_hover_animation);

		if (!styles::get().transparent_clickable) {
			render::filled_rect(pos, size, this_color.modify_alpha(ctx.m_alpha), dpi::scale(styles::get().rounding));

			render::line(
					pos.x + size.x - dpi::scale(32.f),
					pos.y, pos.x + size.x - dpi::scale(32.f),
					pos.y + size.y,
					styles::get().window_backround.modify_alpha(ctx.m_alpha), dpi::scale(1.f));

			auto arrow_texture = (render::gif_t*)resources::get_texture(HASH("arrow_dropbox"));
			if (arrow_texture != nullptr)
				render::image_rotated(arrow_texture->m_textures[0].first, pos + vec2d{ size.x - dpi::scale(16.f), size.y - dpi::scale(16.f) }, dpi::scale(vec2d{ 32.f, 32.f }),
									  (is_opened ? easings::out_back(m_open_animation) : easings::in_back(m_open_animation)) * fPI, color_t{}.modify_alpha(ctx.m_alpha));
		}

		auto font = styles::get_font(styles::get().bold_text ? fonts::menu_bold : fonts::menu_main);

		std::string format{};
		if (m_multiselect) {
			int t = 0;
			for (size_t i = 0; i < m_items.size(); i++) {
				if (*m_value & 1 << i) {
					if (t++ > 0)
						format += STR(", ");

					format += m_items[i];
				}
			}

			if (format.empty())
				format = STR("...");
		} else
			format = m_items[*m_value];

		render::lock(pos, size - dpi::scale(vec2d{ 33.f, 0.f }));
		render::text(pos.x + dpi::scale(styles::get().controls_text_padding.x), pos.y + size.y * 0.5f,
					 color_t::lerp(styles::get().text_color, styles::get().text_hovered, m_hover_animation).modify_alpha(ctx.m_alpha),
					 render::centered_y, font, m_name + STR(": ") + format);
		render::unlock();

		render::filled_rect_gradient(pos.x + size.x - dpi::scale(32.f + 48.f), pos.y, dpi::scale(48.f), size.y,
									 this_color.modify_alpha(0.f), this_color.modify_alpha(ctx.m_alpha), this_color.modify_alpha(ctx.m_alpha), this_color.modify_alpha(0.f));
	}

	void slider_t::render(render_context_t& ctx) {
		m_size = { dpi::scale(styles::get().item_width), dpi::scale(control_height + styles::get().controls_text_padding.y * 1.5f) };

		const float field_width = dpi::scale(styles::get().item_width - 8.f);
		auto pos = this->calculate_position(ctx) + dpi::scale(vec2d{ 0.f, styles::get().controls_text_padding.y /* * 0.5f*/ });

		const auto size =
				vec2d{ field_width, dpi::scale(control_height + styles::get().controls_text_padding.y * 1.5f) } - vec2d{ dpi::scale(0.f), dpi::scale(styles::get().controls_text_padding.y) };

		auto font = styles::get_font(styles::get().bold_text ? fonts::menu_bold : fonts::menu_main);
		const auto text_size = render::calc_text_size(m_name, font);
		*m_value = std::clamp(*m_value, m_min, m_max);

		render::text(pos.x + dpi::scale(styles::get().controls_text_padding.x * 0.5f) - 1, pos.y,
					 color_t::lerp(styles::get().text_color, styles::get().text_hovered, m_hover_animation).modify_alpha(ctx.m_alpha),
					 render::none, font, m_name);

		render::text(pos.x + size.x, pos.y, color_t{ 100, 100, 100 }.modify_alpha(ctx.m_alpha), render::align_left, font, m_format(*m_value));

		pos += { dpi::scale(4.f), text_size.y + dpi::scale(6.f) };
		const float progress = (float)(*m_value - m_min) / (float)(m_max - m_min);

		auto slider_pos = pos + vec2d{ progress * field_width, dpi::scale(4.f) } - dpi::scale(slider_size * 0.5f);

		const auto slider_hovered = globals().is_mouse_in_region(slider_pos, dpi::scale(slider_size));

		m_hovered = slider_hovered || globals().is_mouse_in_region(pos - dpi::scale(vec2d{ 0.f, 3.f }), { field_width, dpi::scale(8.f + 6.f) });
		if (m_hovered)
			ctx.set_hovered_element(this);

		m_hovered = m_hovered && ctx.can_update(this);

		if (m_hovered) {
			m_hover_animation.lerp(ctx, m_hovered);

			if (globals().mouse_down[0]) {
				if (globals().mouse_click[0]) {
					if (!slider_hovered)
						m_slider_value = std::clamp(globals().mouse_position.x - pos.x, 0.f, field_width);

					m_holding = true;
				}
			} else
				m_holding = false;

			if (m_holding) {
				m_slider_value = std::clamp(m_slider_value + globals().mouse_delta.x, 0.f, field_width);
				*m_value = (int)std::round(std::lerp((float)m_min, (float)m_max, std::clamp(m_slider_value / field_width, 0.f, 1.f)));

				if (m_callback != nullptr)
					m_callback();
			} else if (!globals().mouse_down[0])
				m_slider_value = progress * field_width;
		} else {
			if (!globals().mouse_down[0])
				m_holding = false;

			if (ctx.can_update(this) && m_holding) {
				ctx.set_hovered_element(this);
				ctx.m_active_item = this;
				m_hover_animation.lerp(ctx, 1.f);
				m_slider_value = std::clamp(m_slider_value + globals().mouse_delta.x, 0.f, field_width);
				*m_value = (int)std::round(std::lerp((float)m_min, (float)m_max, std::clamp(m_slider_value / field_width, 0.f, 1.f)));

				if (m_callback != nullptr)
					m_callback();
			} else if (!globals().mouse_down[0]) {
				if (ctx.m_active_item == this)
					ctx.m_active_item = nullptr;

				m_hover_animation.lerp(ctx, 0.f);
				m_slider_value = progress * field_width;
			}
		}

		m_slider_animation.lerp(ctx, slider_hovered || m_holding);
		m_visual_progress.lerp(ctx, progress, std::abs(m_visual_progress - progress) * 18.f);

		if (!styles::get().transparent_clickable) {
			render::filled_rect(pos, { field_width, dpi::scale(8.f) },
								color_t::lerp(styles::get().clickable_color, styles::get().clickable_hovered, m_hover_animation).modify_alpha(ctx.m_alpha), dpi::scale(8.f));

			ImDrawGradient_Linear gradient{ to_imvec2(pos), to_imvec2(pos + vec2d{ m_visual_progress * field_width, dpi::scale(8.f) }),
											to_imvec4(styles::get().accent_color1.modify_alpha(ctx.m_alpha)),
											to_imvec4(styles::get().accent_color2.modify_alpha(ctx.m_alpha)) };

			render::draw_list->AddRectFilled(to_imvec2(pos), to_imvec2(pos + vec2d{ m_visual_progress * field_width, dpi::scale(8.f) }), gradient, dpi::scale(8.f), ImDrawCornerFlags_Left);

			render::filled_rect(pos + vec2d{ m_visual_progress * field_width, dpi::scale(4.f) } - dpi::scale(slider_size * 0.5f), dpi::scale(slider_size),
								color_t::lerp({ 200, 200, 200 }, { 255, 255, 255 }, m_slider_animation).modify_alpha(ctx.m_alpha), dpi::scale(2.5f));
		}
	}

	text_input_t::~text_input_t() {
		//
		auto& io = ImGui::GetIO();
		io.WantTextInput = false;
	}

	float text_input_t::last_time_typed() const { return std::abs(globals().get_time() - m_last_time_typed); }
	void text_input_t::render(render_context_t& ctx) {
		m_size = { dpi::scale(styles::get().item_width), dpi::scale(control_height) + dpi::scale(styles::get().controls_text_padding.y * 2.f) };

		const auto pos = this->calculate_position(ctx) + dpi::scale(vec2d{ 0.f, styles::get().controls_text_padding.y });
		const auto size = m_size - dpi::scale(vec2d{ 0.f, styles::get().controls_text_padding.y * 2.f });

		m_hovered = globals().is_mouse_in_region(pos, size);
		if (m_hovered)
			ctx.set_hovered_element(this);

		m_hovered = m_hovered && ctx.can_update(this);
		m_animation.lerp(ctx, m_hovered);

		if (m_hovered) {
			if (globals().mouse_click[0]) {
				m_active = true;
				ctx.m_active_item = this;
				m_selection.m_begin = m_selection.m_end = m_value->length();
				globals().reset_mouse();

				if (m_on_change_state != nullptr)
					m_on_change_state(true);
			}
		} else {
			if (globals().mouse_click[0] && m_active) {
				if (ctx.m_active_item == this)
					ctx.m_active_item = nullptr;

				m_active = false;
				globals().reset_mouse();

				if (m_on_change_state != nullptr)
					m_on_change_state(false);
			}
		}

		if (m_active) {
			auto& io = ImGui::GetIO();
			io.WantTextInput = true;

			const bool is_ctrl_key_only = io.KeyMods == ImGuiKeyModFlags_Ctrl;
			const bool is_shift_key_only = io.KeyMods == ImGuiKeyModFlags_Shift;
			const bool is_shortcut_key = io.KeyMods == ImGuiKeyModFlags_Ctrl;

			const bool is_cut = is_shortcut_key && ImGui::IsKeyPressed(ImGuiKey_X);
			const bool is_copy = is_shortcut_key && ImGui::IsKeyPressed(ImGuiKey_C);
			const bool is_paste = is_shortcut_key && ImGui::IsKeyPressed(ImGuiKey_V);
			const bool is_select_all = is_shortcut_key && ImGui::IsKeyPressed(ImGuiKey_A);

			if (ImGui::IsKeyPressed(ImGuiKey_LeftArrow)) {
				if (m_selection.m_begin > 0) --m_selection.m_begin;
				if (!is_shift_key_only) m_selection.shrink_to_begin();

				m_last_time_typed = globals().get_time();
			} else if (ImGui::IsKeyPressed(ImGuiKey_RightArrow)) {
				if (m_selection.m_end < m_value->length()) ++m_selection.m_end;
				if (!is_shift_key_only) m_selection.shrink_to_end();

				m_last_time_typed = globals().get_time();
			} else if (is_cut || is_copy) {
				if (m_selection.is_active()) {
					to_clipboard({ m_value->begin() + m_selection.m_begin, m_value->begin() + m_selection.m_end });
					if (is_cut) {
						m_value->erase(m_value->begin() + m_selection.m_begin, m_value->begin() + m_selection.m_end);
						if (m_on_change_value != nullptr) m_on_change_value();
					}
				}
			} else if (is_paste) {
				auto clipboard = from_clipboard();
				if (!clipboard.empty()) {
					if (clipboard.length() > 512)
						clipboard = clipboard.substr(0, 512);

					clipboard.erase(std::remove_if(clipboard.begin(), clipboard.end(), [](char c) { return !std::isalnum(c) && !std::ispunct(c) && !std::isspace(c); }), clipboard.end());

					if (!clipboard.empty()) {
						if (m_selection.is_active())
							m_value->erase(m_value->begin() + m_selection.m_begin, m_value->begin() + m_selection.m_end);

						m_value->insert(m_value->end(), clipboard.begin(), clipboard.end());
						m_selection.m_end = m_value->length();
						m_selection.shrink_to_end();
						if (m_on_change_value != nullptr) m_on_change_value();
					}
				}
			} else if (is_select_all) {
				m_selection.m_begin = 0;
				m_selection.m_end = m_value->length();
			}

			if (ImGui::GetIO().InputQueueCharacters.Size > 0) {

				for (auto c: ImGui::GetIO().InputQueueCharacters) {
					if (c == VK_ESCAPE || c == VK_RETURN) {
						ctx.m_active_item = nullptr;
						m_active = false;

						if (m_on_change_state != nullptr)
							m_on_change_state(false);
					} else if (c == VK_BACK) {
						if (!m_value->empty()) {
							if (m_selection.is_active()) {
								m_selection.clamp(m_value->length());
								m_value->erase(m_value->begin() + m_selection.m_begin, m_value->begin() + m_selection.m_end);
								m_selection.shrink_to_begin();
							} else {
								m_value->pop_back();

								if (m_selection.m_begin > 0) --m_selection.m_begin;
								m_selection.shrink_to_begin();
							}

							if (m_on_change_value != nullptr) m_on_change_value();
						}

						m_last_time_typed = globals().get_time();
					} else if (c != VK_TAB) {
						if (m_value->size() < 512) {
							if (std::isalnum(c) || std::ispunct(c) || std::isspace(c)) {
								if (m_selection.is_active()) {
									m_selection.clamp(m_value->length());
									m_value->erase(m_value->begin() + m_selection.m_begin, m_value->begin() + m_selection.m_end);
								}

								m_selection.clamp(m_value->length());
								m_value->insert(m_value->begin() + m_selection.m_begin, (char)c);

								if (m_selection.m_end < m_value->length()) ++m_selection.m_end;
								m_selection.shrink_to_end();

								if (m_on_change_value != nullptr) m_on_change_value();
								m_last_time_typed = globals().get_time();
							}
						}
					}
#ifdef _DEBUG
					else
						printf("%d\n", (int)c);
#endif
				}

				if (m_value->length() > 512)
					*m_value = m_value->substr(0, 512);

				m_selection.clamp(m_value->length());
			}
		} else {
			if (ctx.m_active_item == this)
				ctx.m_active_item = nullptr;

			m_selection.m_begin = m_selection.m_end = 0;
		}

		m_cursor_pulsating.trigger(ctx, m_active && last_time_typed() > 0.2f);
		m_active_animation.lerp(ctx, m_active);

		if (m_active && m_hovered) {
			ImGui::SetMouseCursor(ImGuiMouseCursor_TextInput);
		}

		if (!styles::get().transparent_clickable) {
			render::filled_rect(pos, size, color_t::lerp(styles::get().clickable_color, styles::get().clickable_hovered, m_active ? 1.f : m_animation).modify_alpha(ctx.m_alpha), dpi::scale(styles::get().rounding));
			render::rect(pos, size, styles::get().border_color.modify_alpha((m_active ? 0.f : m_animation) * ctx.m_alpha), dpi::scale(styles::get().rounding));

			color_t color1 = color_t::lerp(styles::get().accent_color1, styles::get().accent_color2, m_cursor_pulsating);
			color_t color2 = color_t::lerp(styles::get().accent_color2, styles::get().accent_color1, m_cursor_pulsating);

			ImDrawGradient_Linear gradient{ to_imvec2(pos), to_imvec2(pos + size),
											to_imvec4(color1.modify_alpha(ctx.m_alpha)),
											to_imvec4(color2.modify_alpha(ctx.m_alpha)) };

			if (m_active)
				render::draw_list->AddRect(to_imvec2(pos), to_imvec2(pos + size), gradient, dpi::scale(styles::get().rounding), 15, dpi::scale(1.f));
		}

		const bool empty = m_value->empty();
		std::string text = empty ? m_hint : *m_value;

		if (m_password && !empty)
			std::transform(text.begin(), text.end(), text.begin(), [](wchar_t c) { return '*'; });

		color_t clr = color_t::lerp(styles::get().text_color, styles::get().text_hovered, m_active ? 1.f : m_animation);

		if (empty)
			clr = color_t::lerp(styles::get().text_inactive, styles::get().text_inactive.increase(10), m_active_animation);

		auto font = styles::get_font(styles::get().bold_text ? fonts::menu_bold : fonts::menu_main);
		const auto text_size = empty ? render::calc_text_size("", font) : render::calc_text_size(text, font);

		float offset = 0.f;
		if (text_size.x + dpi::scale(24.f) > size.x)
			offset = size.x - text_size.x - dpi::scale(24.f);

		vec2d cursor_position;
		if (m_selection.is_active()) {
			const auto preselection_text = std::string{ m_value->begin(), m_value->begin() + m_selection.m_begin };
			const auto selection_text = std::string{ m_value->begin() + m_selection.m_begin, m_value->begin() + m_selection.m_end };
			const auto preselection_size = render::calc_text_size(preselection_text, font);
			const auto selection_size = render::calc_text_size(selection_text, font);
			cursor_position = pos + vec2d{ selection_size.x + preselection_size.x + dpi::scale(12.f) + offset, size.y * 0.5f - text_size.y * 0.5f };
		} else {
			const auto preselection_text = std::string{ m_value->begin(), m_value->begin() + m_selection.m_begin };
			const auto preselection_size = render::calc_text_size(preselection_text, font);
			cursor_position = pos + vec2d{ preselection_size.x + dpi::scale(12.f) + offset, size.y * 0.5f - text_size.y * 0.5f };
		}

		cursor_position.x = std::max<float>(cursor_position.x, 0.f);

		const auto scroll_animation_scale = std::abs(m_cursor_animated - cursor_position.x) * 30.f;
		if (m_cursor_animated > cursor_position.x)
			m_cursor_animated = std::clamp(m_cursor_animated - ctx.m_time_delta * scroll_animation_scale, cursor_position.x, FLT_MAX);
		else if (m_cursor_animated < cursor_position.x)
			m_cursor_animated = std::clamp(m_cursor_animated + ctx.m_time_delta * scroll_animation_scale, 0.f, cursor_position.x);

		render::lock(pos + dpi::scale(vec2d{ 6.f, 6.f }), size - dpi::scale(vec2d{ 12.f, 12.f }));

		if (m_active) {
			if (m_selection.is_active()) {
				const auto preselection_text = std::string{ m_value->begin(), m_value->begin() + m_selection.m_begin };
				const auto selection_text = std::string{ m_value->begin() + m_selection.m_begin, m_value->begin() + m_selection.m_end };
				const auto preselection_size = render::calc_text_size(preselection_text, font);
				const auto selection_size = render::calc_text_size(selection_text, font);
				render::filled_rect(
						{ pos.x + dpi::scale(12.f) + offset + preselection_size.x,
						  pos.y + size.y * 0.5f - selection_size.y * 0.5f },
						selection_size, color_t{ 138, 99, 68 }.modify_alpha(ctx.m_alpha));
			}
		}

		render::textf(pos.x + dpi::scale(12.f) + offset, pos.y + size.y * 0.5f, clr.modify_alpha(ctx.m_alpha), render::centered_y, font, STRC("%s"), text.c_str());
		render::unlock();

		if (m_active) {
			render::line(
					m_cursor_animated, cursor_position.y,
					m_cursor_animated, cursor_position.y + text_size.y,
					color_t{ 255, 255, 255 }.modify_alpha(ctx.m_alpha * (1.f - m_cursor_pulsating)), dpi::scale(1.f));
		}
	}

	void weapon_selector_t::render(render_context_t& ctx) {
		m_size = { dpi::scale(styles::get().item_width), dpi::scale(control_height) + dpi::scale(styles::get().controls_text_padding.y * 2.f) };

		const auto pos = this->calculate_position(ctx) + dpi::scale(vec2d{ 0.f, styles::get().controls_text_padding.y });
		const auto size = m_size - dpi::scale(vec2d{ 0.f, styles::get().controls_text_padding.y * 2.f });

		m_hovered = globals().is_mouse_in_region(pos, size);
		if (m_hovered)
			ctx.set_hovered_element(this);

		m_hovered = m_hovered && ctx.can_update(this);
		m_hover_animation.lerp(ctx, m_hovered);

		const std::vector<std::string> group_names = {
			STR("General"),
			STR("Snipers"),
			STR("Auto-snipers"),
			STR("Heavy pistols"),
			STR("Pistols"),
			STR("Rifles"),
			STR("Heavies"),
			STR("Shotguns"),
			STR("SMGs")
		};

		const auto get_weapon_list = [](int group_id) -> std::vector<std::string> {
			switch (group_id) {
				default:
				case 0:
					return { (char*)(u8"General") };
				case 1:
					return { (char*)(u8"General"), (char*)(u8"\uE028"), (char*)(u8"\uE009") };
				case 2:
					return { (char*)(u8"General"), (char*)(u8"\uE00B"), (char*)(u8"\uE026") };
				case 3:
					return { (char*)(u8"General"), (char*)(u8"\uE001"), (char*)(u8"\uE040") };

				case 4:
					return { (char*)(u8"General"), (char*)(u8"\uE004"), (char*)(u8"\uE020"),
							 (char*)(u8"\uE03D"), (char*)(u8"\uE002"), (char*)(u8"\uE024"),
							 (char*)(u8"\uE01E"), (char*)(u8"\uE003"), (char*)(u8"\uE03F") };

				case 5:
					return { (char*)(u8"General"), (char*)(u8"\uE00D"), (char*)(u8"\uE00A"),
							 (char*)(u8"\uE007"), (char*)(u8"\uE010"), (char*)(u8"\uE03C"), (char*)(u8"\uE027"), (char*)(u8"\uE008") };
				case 6:
					return { (char*)(u8"General"), (char*)(u8"\uE00E"), (char*)(u8"\uE01C") };

				case 7:
					return { (char*)(u8"General"), (char*)(u8"\uE023"), (char*)(u8"\uE019"),
							 (char*)(u8"\uE01D"), (char*)(u8"\uE01B") };

				case 8:
					return { (char*)(u8"General"), (char*)(u8"\uE011"), (char*)(u8"\uE022"),
							 (char*)(u8"\uE021"), (char*)(u8"\uE017"), (char*)(u8"\uE018"),
							 (char*)(u8"\uE013"), (char*)(u8"\uE01A") };
			}
		};

		const auto get_weapon_list_icon = [](int group_id) -> std::vector<std::string> {
			switch (group_id) {
				default:
				case 0:
					return { (char*)(u8"General") };
				case 1:
					return { (char*)(u8"General"), (char*)(u8"SSG-08\n\uE028"), (char*)(u8"AWP\n\uE009") };
				case 2:
					return { (char*)(u8"General"), (char*)(u8"G3SG1\n\uE00B"), (char*)(u8"SCAR-20\n\uE026") };
				case 3:
					return { (char*)(u8"General"), (char*)(u8"Desert Eagle\n\uE001"), (char*)(u8"Revolver\n\uE040") };

				case 4:
					return { (char*)(u8"General"), (char*)(u8"Glock-18\n\uE004"), (char*)(u8"P2000\n\uE020"),
							 (char*)(u8"USP-S\n\uE03D"), (char*)(u8"Dual Berettas\n\uE002"), (char*)(u8"P250\n\uE024"),
							 (char*)(u8"Tec-9\n\uE01E"), (char*)(u8"Five-Seven\n\uE003"), (char*)(u8"CZ-75\n\uE03F") };

				case 5:
					return { (char*)(u8"General"), (char*)(u8"Galil AR\n\uE00D"), (char*)(u8"Famas\n\uE00A"),
							 (char*)(u8"AK-47\n\uE007"), (char*)(u8"M4A4\n\uE010"), (char*)(u8"M4A1-S\n\uE03C"), (char*)(u8"SG-553\n\uE027"), (char*)(u8"AUG\n\uE008") };
				case 6:
					return { (char*)(u8"General"), (char*)(u8"M249\n\uE00E"), (char*)(u8"Negev\n\uE01C") };

				case 7:
					return { (char*)(u8"General"), (char*)(u8"Nova\n\uE023"), (char*)(u8"XM1014\n\uE019"),
							 (char*)(u8"Sawed-off\n\uE01D"), (char*)(u8"MAG-7\n\uE01B") };

				case 8:
					return { (char*)(u8"General"), (char*)(u8"Mac-10\n\uE011"), (char*)(u8"MP9\n\uE022"),
							 (char*)(u8"MP7\n\uE021"), (char*)(u8"MP5-SD\n\uE017"), (char*)(u8"UMP-45\n\uE018"),
							 (char*)(u8"P90\n\uE013"), (char*)(u8"PP-Bizon\n\uE01A") };
			}
		};

		if (m_hovered) {
			if (globals().mouse_click[0]) {
				if (m_popup == nullptr) {
					m_popup = ctx.m_instance->create_popup();
					m_popup->m_flags.add(container_flag_is_popup | container_no_padding | popup_flag_animation | container_flag_visible);
					m_popup->set_position(pos - ctx.m_instance->get_position() + dpi::scale(vec2d{ 0.f, control_height + styles::get().controls_text_padding.y * 0.5f }));

					const auto popup_size = dpi::scale(vec2d{ styles::get().item_width, 164.f });

					m_popup->set_size(popup_size);
					m_popup->on_delete([this]() { m_popup = nullptr; });

					auto row = new containers::row_t{};

					auto groups = new containers::group_t{ "", vec2d{ dpi::scale(styles::get().item_width) * 0.5f, dpi::scale(164.f) } };
					auto weapons = new containers::group_t{ "", vec2d{ dpi::scale(styles::get().item_width) * 0.5f, dpi::scale(164.f) } };

					const auto update_weapon_id = [=](bool value_changed) {
						*m_weapon_id = m_weapon_array[*m_group_id];

						if (m_on_click != nullptr)
							m_on_click();
					};

					auto configs_selectable = new controls::selectables_t{
						m_group_id, group_names, false, dpi::scale(styles::get().item_width) * 0.5f,
						[=](bool value_changed) {
							if (!value_changed)
								return;

							*m_weapon_id = m_weapon_array[*m_group_id];
							weapons->update_state([=]() {
								auto weapon_selectable = new controls::selectables_t{ &m_weapon_array[*m_group_id], get_weapon_list_icon(*m_group_id), false,
																					  dpi::scale(styles::get().item_width) * 0.5f, update_weapon_id };

								weapon_selectable->override_style().font = fonts::menu_main_weapons;

								weapons->add(weapon_selectable);

								if (m_on_click != nullptr)
									m_on_click();
							});
						}

					};
					configs_selectable->override_style().font = fonts::menu_main_weapons;

					groups->add(configs_selectable);

					auto weapon_selectable = new controls::selectables_t{ &m_weapon_array[*m_group_id], get_weapon_list_icon(*m_group_id), false,
																		  dpi::scale(styles::get().item_width) * 0.5f, update_weapon_id };

					weapon_selectable->override_style().font = fonts::menu_main_weapons;
					weapons->add(weapon_selectable);

					row->add(groups);
					row->add(weapons);

					c_style style{};
					style.container_padding = style.window_padding = {};
					row->set_style(style);

					m_popup->add(row);

					globals().reset_mouse();
				} else {
					m_popup->close();
					m_popup = nullptr;
				}
			}
		}

		const auto is_opened = m_popup != nullptr;
		m_open_animation.lerp(ctx, is_opened, 1.75f);

		if (!styles::get().transparent_clickable) {
			render::filled_rect(pos, size, color_t::lerp(styles::get().clickable_color, styles::get().clickable_hovered, m_hover_animation).modify_alpha(ctx.m_alpha), dpi::scale(styles::get().rounding));

			render::line(
					pos.x + size.x - dpi::scale(32.f),
					pos.y, pos.x + size.x - dpi::scale(32.f),
					pos.y + size.y,
					styles::get().window_backround.modify_alpha(ctx.m_alpha), dpi::scale(1.f));

			auto arrow_texture = (render::gif_t*)resources::get_texture(HASH("arrow_dropbox"));
			if (arrow_texture != nullptr)
				render::image_rotated(arrow_texture->m_textures[0].first, pos + vec2d{ size.x - dpi::scale(16.f), size.y - dpi::scale(16.f) }, dpi::scale(vec2d{ 32.f, 32.f }),
									  (is_opened ? easings::out_back(m_open_animation) : easings::in_back(m_open_animation)) * fPI, color_t{}.modify_alpha(ctx.m_alpha));
		}

		auto font = styles::get_font(styles::get().bold_text ? fonts::menu_bold : fonts::menu_main);
		if (m_weapon_array[*m_group_id] == 0)
			render::textf(pos.x + dpi::scale(styles::get().controls_text_padding.x), pos.y + size.y * 0.5f,
						  color_t::lerp(styles::get().text_color, styles::get().text_hovered, m_hover_animation).modify_alpha(ctx.m_alpha),
						  render::centered_y, fonts::menu_main_weapons, STRC("Setting for: %s"), group_names[*m_group_id].c_str());
		else
			render::textf(pos.x + dpi::scale(styles::get().controls_text_padding.x), pos.y + size.y * 0.5f,
						  color_t::lerp(styles::get().text_color, styles::get().text_hovered, m_hover_animation).modify_alpha(ctx.m_alpha),
						  render::centered_y, fonts::menu_main_weapons, STRC("Setting for: %s"), get_weapon_list(*m_group_id)[*m_weapon_id].c_str());
	}

	void slider_impl_t::update(render_context_t& ctx, i_renderable* parent, const vec2d& position, const vec2d& size, bool& hovered) {
		const float progress = this->get_progress();

		hovered = globals().is_mouse_in_region(position, size);
		if (hovered)
			ctx.set_hovered_element(parent);

		hovered = hovered && ctx.can_update(parent);

		if (hovered) {
			m_hover_animation.lerp(ctx, hovered);

			if (globals().mouse_down[0]) {
				if (globals().mouse_click[0]) {
					m_value = std::clamp(globals().mouse_position.x - position.x, 0.f, size.x);

					m_holding = true;
				}
			} else
				m_holding = false;

			if (m_holding) {
				m_value = std::clamp(m_value + globals().mouse_delta.x, 0.f, size.x);
				*m_pvalue = (int)std::round(std::lerp((float)m_min, (float)m_max, std::clamp(m_value / size.x, 0.f, 1.f)));
			} else if (!globals().mouse_down[0])
				m_value = progress * size.x;
		} else {
			if (!globals().mouse_down[0])
				m_holding = false;

			if (ctx.can_update(parent) && m_holding) {
				ctx.set_hovered_element(parent);
				ctx.m_active_item = parent;
				m_hover_animation.lerp(ctx, 1.f);
				m_value = std::clamp(m_value + globals().mouse_delta.x, 0.f, size.x);
				*m_pvalue = (int)std::round(std::lerp((float)m_min, (float)m_max, std::clamp(m_value / size.x, 0.f, 1.f)));
			} else if (!globals().mouse_down[0]) {
				if (ctx.m_active_item == parent)
					ctx.m_active_item = nullptr;

				m_hover_animation.lerp(ctx, 0.f);
				m_value = progress * size.x;
			}
		}
	}

	void slider2d_impl_t::update(render_context_t& ctx, i_renderable* parent, const vec2d& position, const vec2d& size, bool& hovered) {
		const auto progress_x = this->get_progress_x(), progress_y = this->get_progress_y();

		hovered = globals().is_mouse_in_region(position, size);
		if (hovered)
			ctx.set_hovered_element(parent);

		hovered = hovered && ctx.can_update(parent);

		auto apply_values = [&]() {
			*m_px = (int)std::round(std::lerp((float)m_min, (float)m_max, std::clamp(m_value.x / size.x, 0.f, 1.f)));
			*m_py = (int)std::round(std::lerp((float)m_min, (float)m_max, std::clamp(m_value.y / size.y, 0.f, 1.f)));
		};

		if (hovered) {
			m_hover_animation.lerp(ctx, hovered);

			if (globals().mouse_down[0]) {
				if (globals().mouse_click[0]) {
					m_value.x = std::clamp(globals().mouse_position.x - position.x, 0.f, size.x);
					m_value.y = std::clamp(globals().mouse_position.y - position.y, 0.f, size.y);

					m_holding = true;
				}
			} else
				m_holding = false;

			if (m_holding) {
				m_value.x = std::clamp(m_value.x + globals().mouse_delta.x, 0.f, size.x);
				m_value.y = std::clamp(m_value.y + globals().mouse_delta.y, 0.f, size.y);

				apply_values();
			} else if (!globals().mouse_down[0])
				m_value = { progress_x * size.x, progress_y * size.y };
		} else {
			if (!globals().mouse_down[0])
				m_holding = false;

			if (ctx.can_update(parent) && m_holding) {
				ctx.set_hovered_element(parent);
				ctx.m_active_item = parent;
				m_hover_animation.lerp(ctx, 1.f);

				m_value.x = std::clamp(m_value.x + globals().mouse_delta.x, 0.f, size.x);
				m_value.y = std::clamp(m_value.y + globals().mouse_delta.y, 0.f, size.y);

				apply_values();
			} else if (!globals().mouse_down[0]) {
				if (ctx.m_active_item == parent)
					ctx.m_active_item = nullptr;

				m_hover_animation.lerp(ctx, 0.f);
				m_value = { progress_x * size.x, progress_y * size.y };
			}
		}
	}

	void colorpicker_impl_t::render(render_context_t& ctx) {
		m_size = { dpi::scale(232.f), dpi::scale(224.f - (m_modify_alpha ? 0.f : 24.f)) };
		const auto field_size = dpi::scale(vec2d{ 232.f, 180.f });
		const auto pos = this->calculate_position(ctx) + dpi::scale(vec2d{ 0.f, styles::get().controls_text_padding.y });
		bool hovered = false;

		const auto saturated_color = color_t::hsb(m_hue * 0.001f, 1.f, 1.f);

		// picker area
		{
			if (!m_alpha_slider.m_holding && !m_hue_slider.m_holding)
				m_sb_slider.update(ctx, this, pos, field_size, hovered);

			render::filled_rect_gradient(pos.x, pos.y, field_size.x, field_size.y,
										 color_t{}.modify_alpha(ctx.m_alpha), saturated_color.new_alpha(ctx.m_alpha),
										 saturated_color.new_alpha(ctx.m_alpha), color_t{}.modify_alpha(ctx.m_alpha));

			render::filled_rect_gradient(pos.x, pos.y, field_size.x, field_size.y,
										 color_t{ 0, 0, 0, 0 }, color_t{ 0, 0, 0, 0 },
										 color_t{ 0, 0, 0 }.modify_alpha(ctx.m_alpha), color_t{ 0, 0, 0 }.modify_alpha(ctx.m_alpha));

			render::circle_filled(pos.x + m_sb_slider.get_progress_x() * field_size.x, pos.y + m_sb_slider.get_progress_y() * field_size.y,
								  dpi::scale(3.f), color_t{}.new_alpha(ctx.m_alpha), (int)dpi::scale(12.f));

			render::circle(pos.x + m_sb_slider.get_progress_x() * field_size.x, pos.y + m_sb_slider.get_progress_y() * field_size.y,
						   dpi::scale(3.f), color_t{ 0, 0, 0 }.new_alpha(ctx.m_alpha), (int)dpi::scale(12.f));
		}

		// hue selector
		{
			const auto hue_size = dpi::scale(vec2d{ 232.f, 6.f });
			auto hue_pos = pos + vec2d{ 0.f, field_size.y + dpi::scale(8.f) };

			if (!m_alpha_slider.m_holding && !m_sb_slider.m_holding)
				m_hue_slider.update(ctx, this, hue_pos, hue_size, hovered);

			constexpr int iterations = 25;
			color_t last_color = color_t{ 255, 0, 0 }.modify_alpha(ctx.m_alpha);
			for (auto i = 0; i < iterations; i++) {
				const auto current_color = color_t::hsb((float)i / iterations, 1.f, 1.f).modify_alpha(ctx.m_alpha);

				render::filled_rect_gradient(hue_pos.x + i * hue_size.x / iterations, hue_pos.y, hue_size.x / iterations, hue_size.y,
											 last_color, current_color, current_color, last_color);

				last_color = current_color;
			}

			const auto progress = m_hue_slider.get_progress();

			render::circle_filled(hue_pos.x + progress * hue_size.x, hue_pos.y + hue_size.y * 0.5f, dpi::scale(6.f), saturated_color.new_alpha(ctx.m_alpha), (int)dpi::scale(18.f));
			render::circle(hue_pos.x + progress * hue_size.x, hue_pos.y + hue_size.y * 0.5f, dpi::scale(6.f), color_t{}.modify_alpha(ctx.m_alpha), (int)dpi::scale(18.f), dpi::scale(1.f));
		}

		if (m_modify_alpha) {
			const auto alpha_size = dpi::scale(vec2d{ 232.f, 12.f });
			auto alpha_pos = pos + vec2d{ 0.f, field_size.y + dpi::scale(24.f) };

			if (!m_hue_slider.m_holding && !m_sb_slider.m_holding)
				m_alpha_slider.update(ctx, this, alpha_pos, alpha_size, hovered);

			if (auto bg_texture = (render::gif_t*)resources::get_texture(HASH("colorpicker_bg")); bg_texture != nullptr)
				render::image(alpha_pos, alpha_size, bg_texture->m_textures[0].first, color_t{}.modify_alpha(ctx.m_alpha));

			render::filled_rect_gradient(alpha_pos.x, alpha_pos.y, alpha_size.x, alpha_size.y,
										 m_value->new_alpha(0.f), m_value->new_alpha(ctx.m_alpha), m_value->new_alpha(ctx.m_alpha), m_value->new_alpha(0.f));

			render::filled_rect({ alpha_pos.x + m_alpha_slider.get_progress() * alpha_size.x - dpi::scale(1.f), alpha_pos.y }, { dpi::scale(2.f), alpha_size.y }, color_t{}.modify_alpha(ctx.m_alpha));
		} else
			m_alpha = 1000;

		if (m_on_change != nullptr && (m_sb_slider.m_holding || m_hue_slider.m_holding || m_alpha_slider.m_holding))
			m_on_change();

		*m_value = color_t::hsb(m_hue * 0.001f, m_saturation * 0.001f, 1.f - m_brightness * 0.001f).modify_alpha(m_alpha * 0.001f);
	}

	void colorpicker_t::render(render_context_t& ctx) {
		m_size = { dpi::scale(styles::get().item_width), dpi::scale(16.f) + dpi::scale(styles::get().controls_text_padding.y * 2.f) };

		const auto pos = this->calculate_position(ctx) + dpi::scale(vec2d{ 0.f, styles::get().controls_text_padding.y });
		const auto size = m_size - dpi::scale(vec2d{ 0.f, styles::get().controls_text_padding.y * 2.f });

		m_hovered = globals().is_mouse_in_region(pos, size);
		if (m_hovered)
			ctx.set_hovered_element(this);

		m_hovered = m_hovered && ctx.can_update(this);
		m_hover_animation.lerp(ctx, m_hovered);

		if (m_hovered) {
			if (globals().mouse_click[0]) {
				if (m_popup == nullptr) {
					m_popup = ctx.m_instance->create_popup();
					m_popup->m_flags.add(container_flag_is_popup | popup_flag_animation | container_flag_visible);
					m_popup->set_position(pos - ctx.m_instance->get_position() + dpi::scale(vec2d{ 0.f, 16.f + 8.f }));
					m_popup->set_size(vec2d{ dpi::scale(248.f), dpi::scale(272.f - (m_modify_alpha ? 0.f : 24.f)) });
					m_popup->on_delete([this]() { m_popup = nullptr; });

					auto colorpicker_impl = new colorpicker_impl_t{ m_value, m_modify_alpha };
					auto text_input = new text_input_t{ &m_color_text, "" };
					const auto max_size = m_modify_alpha ? 8 : 6;
					std::string* color_text = &m_color_text;

					auto update_color_text = [=]() {
						if (m_modify_alpha)
							*color_text = std::vformat(STR("{:08X}"), std::make_format_args(std::byteswap(m_value->u32()))).substr(0, max_size);
						else
							*color_text = std::vformat(STR("{:06X}"), std::make_format_args(std::byteswap(m_value->u32() & 0x00FFFFFF))).substr(0, max_size);
					};

					colorpicker_impl->on_change(update_color_text);
					update_color_text();

					m_popup->add(colorpicker_impl);

					text_input->on_change_value([=]() {
						color_text->erase(std::remove_if(color_text->begin(), color_text->end(), [](char c) {
											  return !(c >= '0' && c <= '9' || c >= 'A' && c <= 'F' || c >= 'a' && c <= 'f');
										  }),
										  color_text->end());

						*color_text = color_text->substr(0, max_size);

						if (!color_text->empty()) {
							m_value->u32() = std::byteswap(std::stoul(*color_text, nullptr, 16));
							if (!m_modify_alpha)
								m_value->u32() = (m_value->u32() >> 8) | 0xFF000000;
						}

						colorpicker_impl->update_cache();
					});

					c_style s{};
					s.clickable_color = { 12, 12, 12 };
					s.clickable_hovered = { 16, 16, 16 };

					s.item_width = 232.f;
					text_input->set_style(s);

					m_popup->add(text_input);

					globals().reset_mouse();
				} else {
					m_popup->close();
					m_popup = nullptr;
				}
			}
		}

		const auto is_opened = m_popup != nullptr;
		m_open_animation.lerp(ctx, is_opened, 1.75f);

		auto font = styles::get_font(styles::get().bold_text ? fonts::menu_bold : fonts::menu_main);
		render::text(pos.x, pos.y + size.y * 0.5f,
					 color_t::lerp(styles::get().text_color, styles::get().text_hovered, m_hover_animation).modify_alpha(ctx.m_alpha),
					 render::centered_y, font, m_label);

		{
			const auto indicator_size = dpi::scale(vec2d{ 16.f, 16.f });
			const auto indicator_pos = vec2d{ pos.x + dpi::scale(styles::get().item_width) - indicator_size.x,
											  pos.y + size.y * 0.5f - indicator_size.x * 0.5f };

			if (m_modify_alpha) {
				if (auto bg_texture = (render::gif_t*)resources::get_texture(HASH("colorpicker_bg2")); bg_texture != nullptr)
					render::image(indicator_pos, indicator_size, bg_texture->m_textures[0].first, color_t{}.modify_alpha(ctx.m_alpha), dpi::scale(2.5f));
			}

			render::filled_rect(indicator_pos, indicator_size, m_value->modify_alpha(ctx.m_alpha), dpi::scale(2.5f));
		}
	}

	void key_binder_t::render(render_context_t& ctx) {
		m_size = { dpi::scale(128.f), dpi::scale(control_height) + dpi::scale(styles::get().controls_text_padding.y * 2.f) };

		const auto pos = this->calculate_position(ctx) + dpi::scale(vec2d{ 0.f, styles::get().controls_text_padding.y });
		const auto size = m_size - dpi::scale(vec2d{ 0.f, styles::get().controls_text_padding.y * 2.f });

		m_hovered = globals().is_mouse_in_region(pos, size);
		if (m_hovered)
			ctx.set_hovered_element(this);

		m_hovered = m_hovered && ctx.can_update(this);
		m_hover_animation.lerp(ctx, m_hovered);

		if (m_hovered) {
			if (globals().mouse_click[0]) {
				m_active = true;
				ctx.m_active_item = this;
				globals().reset_mouse();
			}
		}

		m_open_animation.lerp(ctx, m_active, 1.75f);
		const auto this_color = color_t::lerp(styles::get().clickable_color, styles::get().clickable_hovered, m_hover_animation);

		if (!styles::get().transparent_clickable) {
			render::filled_rect(pos, size, this_color.modify_alpha(ctx.m_alpha), dpi::scale(styles::get().rounding));

			render::line(
					pos.x + size.x - dpi::scale(32.f),
					pos.y, pos.x + size.x - dpi::scale(32.f),
					pos.y + size.y,
					styles::get().window_backround.modify_alpha(ctx.m_alpha), dpi::scale(1.f));

			auto hotkeys_icon = (render::gif_t*)resources::get_texture(HASH("hotkeys_icon"));
			if (hotkeys_icon != nullptr) {
				const auto color = color_t::lerp({ 64, 64, 64 }, { 255, 255, 255 }, m_open_animation);
				render::image_rotated(hotkeys_icon->m_textures[0].first, pos + vec2d{ size.x - dpi::scale(16.f), size.y - dpi::scale(16.f) }, dpi::scale(vec2d{ 16.f, 16.f }),
									  (m_active ? easings::out_back(m_open_animation) : easings::in_back(m_open_animation)) * fPI * 0.25f, color.modify_alpha(ctx.m_alpha));
			}
		}

		auto font = styles::get_font(styles::get().bold_text ? fonts::menu_bold : fonts::menu_main);

		static const std::vector<std::string> key_strings = {
			STRS("None"), STRS("m1"), STRS("m2"), STRS("c+b"), STRS("m3"), STRS("m4"), STRS("m5"),
			STRS("unk"), STRS("bkspc"), STRS("tab"), STRS("unk"), STRS("unk"), STRS("unk"),
			STRS("enter"), STRS("unk"), STRS("unk"), STRS("shift"), STRS("ctrl"), STRS("alt"), STRS("pause"),
			STRS("caps"), STRS("unk"), STRS("unk"), STRS("unk"), STRS("unk"), STRS("unk"), STRS("unk"),
			STRS("esc"), STRS("unk"), STRS("unk"), STRS("unk"), STRS("unk"), STRS("space"), STRS("pg up"),
			STRS("pg down"), STRS("end"), STRS("home"), STRS("left"), STRS("up"), STRS("right"), STRS("down"),
			STRS("unk"), STRS("print"), STRS("unk"), STRS("ps"), STRS("ins"), STRS("del"), STRS("unk"),
			STRS("0"), STRS("1"), STRS("2"), STRS("3"), STRS("4"), STRS("5"), STRS("6"), STRS("7"),
			STRS("8"), STRS("9"), STRS("unk"), STRS("unk"), STRS("unk"), STRS("unk"), STRS("unk"), STRS("unk"),
			STRS("unk"), STRS("a"), STRS("b"), STRS("c"), STRS("d"), STRS("e"), STRS("f"), STRS("g"),
			STRS("h"), STRS("i"), STRS("j"), STRS("k"), STRS("l"), STRS("m"), STRS("n"), STRS("o"),
			STRS("p"), STRS("q"), STRS("r"), STRS("s"), STRS("t"), STRS("u"), STRS("v"), STRS("w"), STRS("x"),
			STRS("y"), STRS("z"), STRS("l-win"), STRS("r-win"), STRS("unk"), STRS("unk"), STRS("unk"),
			STRS("num0"), STRS("num1"), STRS("num2"), STRS("num 3"), STRS("num4"), STRS("num5"), STRS("num6"),
			STRS("num7"), STRS("num8"), STRS("num9"), STRS("*"), STRS("+"), STRS("_"), STRS("-"), STRS("."),
			STRS("/"), STRS("f1"), STRS("f2"), STRS("f3"), STRS("f4"), STRS("f5"), STRS("f6"), STRS("f7"),
			STRS("f8"), STRS("f9"), STRS("f10"), STRS("f11"), STRS("f12"), STRS("f13"), STRS("f14"), STRS("f15"),
			STRS("f16"), STRS("f17"), STRS("f18"), STRS("f19"), STRS("f20"), STRS("f21"), STRS("f22"), STRS("f23"),
			STRS("f24"), STRS("unk"), STRS("unk"), STRS("unk"), STRS("unk"), STRS("unk"), STRS("unk"), STRS("unk"),
			STRS("unk"), STRS("num lock"), STRS("slock"), STRS("unk"), STRS("unk"), STRS("unk"), STRS("unk"), STRS("unk"),
			STRS("unk"), STRS("unk"), STRS("unk"), STRS("unk"), STRS("unk"), STRS("unk"), STRS("unk"), STRS("unk"),
			STRS("unk"), STRS("lshift"), STRS("rshift"), STRS("lctrl"), STRS("rctrl"), STRS("lmenu"), STRS("rmenu"),
			STRS("unk"), STRS("unk"), STRS("unk"), STRS("unk"), STRS("unk"), STRS("unk"), STRS("unk"), STRS("unk"),
			STRS("unk"), STRS("unk"), STRS("nxt"), STRS("prv"), STRS("stop"), STRS("play"), STRS("unk"), STRS("unk"),
			STRS("unk"), STRS("unk"), STRS("unk"), STRS("unk"), STRS(";"), STRS("+"), STRS(","), STRS("-"), STRS("."),
			STRS("/?"), STRS("~"), STRS("unk"), STRS("unk"), STRS("unk"), STRS("unk"), STRS("unk"), STRS("unk"),
			STRS("unk"), STRS("unk"), STRS("unk"), STRS("unk"), STRS("unk"), STRS("unk"), STRS("unk"), STRS("unk"),
			STRS("unk"), STRS("unk"), STRS("unk"), STRS("unk"), STRS("unk"), STRS("unk"), STRS("unk"), STRS("unk"),
			STRS("unk"), STRS("unk"), STRS("unk"), STRS("unk"), STRS("[{"), STRS("\\|"), STRS("}]"), STRS("'\""),
			STRS("unk"), STRS("unk"), STRS("unk"), STRS("unk"), STRS("unk"), STRS("unk"), STRS("unk"), STRS("unk"),
			STRS("unk"), STRS("unk"), STRS("unk"), STRS("unk"), STRS("unk"), STRS("unk"), STRS("unk"), STRS("unk"),
			("unk"), ("unk"), ("unk"), ("unk"), ("unk"), ("unk"), ("unk"), ("unk"),
			STRS("unk"), STRS("unk"), STRS("unk"), STRS("unk"), STRS("unk"), STRS("unk"), STRS("unk"), STRS("unk"), STRS("unk")
		};

		render::textf(pos.x + dpi::scale(styles::get().controls_text_padding.x), pos.y + size.y * 0.5f,
					  color_t::lerp(styles::get().text_color, styles::get().text_hovered, m_hover_animation).modify_alpha(ctx.m_alpha),
					  render::centered_y, font, STRC("Key: %s"), key_strings[*m_value].c_str());

		if (m_active) {
			ImGui::GetIO().WantTextInput = true;
			hotkeys->m_key_binder_active = true;

			if (hotkeys->m_last_key_pressed > 0) {
				if (hotkeys->m_last_key_pressed == VK_ESCAPE) {
					*m_value = 0;
					if (m_on_bind != nullptr) m_on_bind();
					hotkeys->m_last_key_pressed = 0;
					hotkeys->m_key_binder_active = false;
				} else {
					*m_value = hotkeys->m_last_key_pressed;
					if (m_on_bind != nullptr) m_on_bind();
					hotkeys->m_last_key_pressed = 0;
					hotkeys->m_key_binder_active = false;
				}

				ctx.m_active_item = nullptr;
				m_active = false;
			}
		} else {
			if (ctx.m_active_item == this)
				ctx.m_active_item = nullptr;
		}
	}

	web_image_t::web_image_t(std::function<std::vector<uint8_t>()> download, float scale, std::optional<vec2d> force_size, bool rounded) {
		m_force_size = force_size;
		m_size = m_force_size.value_or(dpi::scale(vec2d{ 32.f, 32.f }));
		m_rounded = rounded;

		std::thread(
				[this, download, scale]() {
					m_gif = create_gif_from_buffer(download()).get();
					m_size = m_force_size.value_or(((render::gif_t*)m_gif)->get_original_size() * scale);
					m_downloaded = true;
				})
				.detach();
	}

	void web_image_t::render(render_context_t& ctx) {
		if (!m_downloaded.load()) {
			if (!m_locked) {
				m_locked = true;
				ctx.m_instance->lock();
			}

			m_show_animation.lerp(ctx, 0.f);
			m_cycle += ctx.m_time_delta;
			while (m_cycle > 1.f)
				m_cycle -= 1.f;

			auto loading_texture = (render::gif_t*)resources::get_texture(HASH("loading"));
			render::image_rotated(loading_texture->m_textures[0].first, this->calculate_position(ctx) + m_size * 0.5f, dpi::scale(vec2d{ 16.f, 16.f }), -m_cycle * f2PI,
								  color_t{}.modify_alpha(ctx.m_alpha));
		} else {
			m_show_animation.lerp(ctx, 1.f);
			if (m_locked) {
				m_locked = false;
				ctx.m_instance->unlock();
			}

			((render::gif_t*)m_gif)->render(render::draw_list, this->calculate_position(ctx), m_size, color_t{}.modify_alpha(ctx.m_alpha * m_show_animation), m_rounded ? m_size.x * 0.5f : 0.f);
		}
	}

	void skin_selector_t::render(render_context_t& ctx) {
		m_size = dpi::scale(vec2d{ 512.f, 384.f }) / 4.1f;

		auto pos = this->calculate_position(ctx);

		if (!m_loaded) {
			m_cycle += ctx.m_time_delta;
			while (m_cycle > 1.f)
				m_cycle -= 1.f;

			ImDrawGradient_Linear gradient{ to_imvec2(pos), to_imvec2(pos + m_size),
											to_imvec4(color_t{ 4, 11, 14 }.modify_alpha(ctx.m_alpha)),
											to_imvec4(color_t{ 30, 32, 35 }.modify_alpha(ctx.m_alpha)) };

			render::draw_list->AddRectFilled(to_imvec2(pos), to_imvec2(pos + m_size), gradient, dpi::scale(5.f));

			auto loading_texture = (render::gif_t*)resources::get_texture(HASH("loading"));
			constexpr auto loading_size = vec2d{ 16.f, 16.f };
			render::image_rotated(loading_texture->m_textures[0].first, pos + m_size * 0.5f, dpi::scale(loading_size), -m_cycle * f2PI,
								  color_t{}.modify_alpha(ctx.m_alpha));
		} else {
			m_hovered = globals().is_mouse_in_region(pos, m_size);
			if (m_hovered)
				ctx.set_hovered_element(this);

			m_hovered = m_hovered && ctx.can_update(this);
			m_hover_animation.lerp(ctx, m_hovered);

			if (m_hovered) {
				if (globals().mouse_click[0]) {
					m_callback();
					globals().reset_mouse();
				}
			}

			std::pair<color_t, color_t> colors{};

			switch (m_rarity) {
				default:
				case -1:
					colors = { { 4, 11, 14 }, { 30, 32, 35 } };
					break;
				case 2:
					colors = { { 12, 12, 12 }, { 0, 47, 82 } };
					break;
				case 3:
					colors = { { 12, 12, 12 }, { 30, 0, 79 } };
					break;
				case 4:
					colors = { { 12, 12, 12 }, { 75, 2, 72 } };
					break;
				case 5:
					colors = { { 12, 12, 12 }, { 62, 0, 0 } };
					break;
				case 6:
					colors = { { 12, 12, 12 }, { 106, 85, 13 } };
					break;
			}

			ImDrawGradient_Linear gradient{ to_imvec2(pos), to_imvec2(pos + m_size),
											to_imvec4(colors.first.modify_alpha(ctx.m_alpha)),
											to_imvec4(colors.second.modify_alpha(ctx.m_alpha)) };

			render::draw_list->AddRectFilled(to_imvec2(pos), to_imvec2(pos + m_size), gradient, dpi::scale(5.f));

			const auto pic_padding = m_size * 0.08f * m_hover_animation;
			const auto pic_size = m_size - pic_padding;

			render::image(pos + pic_padding * 0.5f, pic_size, FIRST_FRAME(m_texture), color_t{}.modify_alpha(ctx.m_alpha));
		}
	}

	void gif_t::render(render_context_t& ctx) {
		if (m_gif != nullptr) {
			auto pos = this->calculate_position(ctx);

			m_hovered = m_on_click != nullptr && globals().is_mouse_in_region(pos, m_size);
			if (m_hovered)
				ctx.set_hovered_element(this);

			m_hovered = m_hovered && ctx.can_update(this);

			if (m_hovered) {
				if (globals().mouse_click[0]) {
					m_on_click();
					globals().reset_mouse();
				}
			}

			((render::gif_t*)m_gif)->render(render::draw_list, pos, m_size, color_t{}.modify_alpha(ctx.m_alpha), m_rounded ? m_size.x * 0.5f : 0.f);
		}
	}
} // namespace NS_GUI::controls