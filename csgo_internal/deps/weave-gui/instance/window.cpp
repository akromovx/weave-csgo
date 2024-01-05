#include "window.hpp"
#include "../controls/controls.hpp"
#include "../render_wrapper.hpp"

#include <format>

#include "../../../src/blur/blur.hpp"

#include "../../../src/features/network.hpp"
#include "../../../src/features/visuals/logs.hpp"

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

namespace NS_GUI::instance {
	static void render_gradient_line(vec2d pos, vec2d size, float alpha) {
		color_t color = { 255, 255, 255, 127 };
		render::filled_rect_gradient(pos.x, std::round(pos.y), size.x * 0.5f, std::round(size.y),
									 color.modify_alpha(0.f), color.modify_alpha(alpha), color.modify_alpha(alpha), color.modify_alpha(0.f));

		render::filled_rect_gradient(pos.x + size.x * 0.5f, std::round(pos.y), size.x * 0.5f, std::round(size.y),
									 color.modify_alpha(alpha), color.modify_alpha(0.f), color.modify_alpha(0.f), color.modify_alpha(alpha));
	}

	bool window_t::update() {
		bool result = false;
		auto& ctx = m_render_context;
		ctx.m_instance = this;

		m_can_drag = false;
		if (hovered() && ctx.m_hovered_item == nullptr && ctx.m_active_item == nullptr) {
			if (globals().mouse_down[0] && globals().mouse_click[0]) {
				m_dragging = true;
				result = true;
			}

			m_can_drag = true;
		} else
			m_dragging = false;

		if (ctx.m_hovered_item != nullptr || ctx.m_active_item != nullptr)
			result = true;

		if (!globals().mouse_down[0])
			m_dragging = false;

		if (this->has_popups()) {
			m_dragging = false;
			result = true;
		}

		if (m_dragging)
			m_position += globals().mouse_delta;

		return result;
	}

	static void copy_varname(void* ptr) {
		//auto buf = network::simple_get(dformat(STRS("menu/var?var={}"), (int)ptr), CLOUD);
		//if (buf.empty()) {
		//	cheat_logs->add_info(STRS("Server error!"));
		//} else {
		//	const auto j = json_t::parse(buf);
		//	if (j.contains(STRS("name"))) {
		//		const auto var_name = j[STRS("name")].get<std::string>();
		//		if (!var_name.empty()) {
		//			to_clipboard(dformat(STRS("vars.find('{}')"), var_name));
		//			cheat_logs->add_info(STRS("Reference has been copied!"));
		//		} else
		//			cheat_logs->add_error(STRS("This var is not included in config!"));
		//	} else
		//		cheat_logs->add_error(STRS("This var is not included in config!"));
		//}
	}

	void window_t::render() {
		styles::reset();

		if (dpi::_new_scale.has_value() && *dpi::_new_scale != dpi::_scale)
			return;

		auto& ctx = m_render_context;
		ctx.m_instance = this;
		ctx.m_time_delta = globals().time_delta;

		m_alpha = std::clamp(m_alpha + (m_opened ? ctx.m_time_delta : -ctx.m_time_delta) * 8.f, 0.f, 1.f);

		ctx.m_base_position = m_position;
		ctx.reset();

		ctx.m_can_update = !m_dragging;
		ctx.m_item_width = dpi::scale(styles::get().item_width);

		const auto render_overrided = m_override_render_function != nullptr;

		if (render_overrided) {
			ctx.m_alpha = std::min<float>(std::clamp(ctx.m_alpha + ctx.m_time_delta * 4.f, 0.f, 1.f), m_alpha);
			ctx.m_size = m_size;

			m_override_render_function(ctx, this);
			ctx.m_active_item = nullptr;
			ctx.m_hovered_item = nullptr;
		} else
			ctx.m_size = m_size - dpi::scale(styles::get().window_padding * 2.f);

		if (ctx.m_size.x < 0.f || ctx.m_size.y < 0.f)
			ctx.m_size.set();

		if (m_alpha <= 0.01f || render_overrided)
			return;

		SET_AND_RESTORE(ctx.m_cursor, dpi::scale(vec2d{ ctx.m_cursor + styles::get().window_padding }));

		shaders::create_blur(render::draw_list, to_imvec2(m_position), to_imvec2(m_position + vec2d{ dpi::scale(sidebar_width), m_size.y }), color_t{}.modify_alpha(m_alpha).abgr());

		render::filled_rect(m_position, vec2d{ dpi::scale(sidebar_width), m_size.y },
							color_t{ 0, 0, 0 }.modify_alpha(m_alpha * 0.5f), dpi::scale(5.f), ImDrawCornerFlags_Left);

		render::filled_rect(m_position + vec2d{ dpi::scale(sidebar_width), 0.f }, m_size - vec2d{ dpi::scale(sidebar_width), 0.f },
							styles::get().window_backround.modify_alpha(m_alpha), dpi::scale(5.f), ImDrawCornerFlags_Right);

		render::rect(m_position + vec2d{ dpi::scale(sidebar_width) - 1, 0.f }, vec2d{ dpi::scale(1.f), m_size.y }, color_t{}.modify_alpha(m_alpha * 0.1f));

		render_gradient_line(m_position + dpi::scale(vec2d{ 16.f, 73.f }), dpi::scale(vec2d{ 164.f, 1.f }), m_alpha);
		render_gradient_line(m_position + dpi::scale(vec2d{ 16.f, 414.f }), dpi::scale(vec2d{ 164.f, 1.f }), m_alpha);

		render::gif_t* current_texture = nullptr;

		switch ((int)(dpi::_get_actual_scale() * 100.f)) {
			case 100: current_texture = (render::gif_t*)resources::get_texture(HASH("wlogo_100")); break;
			case 125: current_texture = (render::gif_t*)resources::get_texture(HASH("wlogo_125")); break;
			case 150: current_texture = (render::gif_t*)resources::get_texture(HASH("wlogo_150")); break;
			case 175: current_texture = (render::gif_t*)resources::get_texture(HASH("wlogo_175")); break;
			case 200: current_texture = (render::gif_t*)resources::get_texture(HASH("wlogo_200")); break;
		}

		if (current_texture != nullptr) {
			const auto mins = m_position + dpi::scale(vec2d{ 65.f, 24.f });
			const auto size = dpi::scale(vec2d{ 61.f, 28.f });
			ImDrawGradient_Linear gradient{
				to_imvec2({ mins.x + size.x * 0.5f, mins.y }),
				to_imvec2({ mins.x + size.x * 0.5f, mins.y + size.y }),
				to_imvec4(styles::get().accent_color1.modify_alpha(ctx.m_alpha)),
				to_imvec4(styles::get().accent_color2.modify_alpha(ctx.m_alpha)),
			};

			render::draw_list->AddImage(current_texture->m_textures[0].first, to_imvec2(mins), to_imvec2(mins + size), { 0.f, 0.f }, { 1.f, 1.f }, gradient);
			//render::image(m_position + dpi::scale(vec2d{ 65.f, 24.f }), dpi::scale(vec2d{ 61.f, 28.f }), current_texture, color_t{}.modify_alpha(m_alpha));
		} else
			render::filled_rect(m_position + dpi::scale(vec2d{ 65.f, 24.f }), dpi::scale(vec2d{ 61.f, 28.f }),
								color_t{ 100, 100, 100 }.modify_alpha((1.f - m_show_animation) * 0.1f + m_skeleton_pulsating * 0.25f), dpi::scale(5.f));

		if (m_show_animation < 1.f && m_loading) {
			m_loading_cycle += ctx.m_time_delta * 0.5f;
			while (m_loading_cycle > 1.f)
				m_loading_cycle -= 1.f;

			const auto loading_pos = m_position + vec2d{ dpi::scale(sidebar_width), 0.f } + (m_size - vec2d{ dpi::scale(sidebar_width), 0.f }) * 0.5f;
			containers::render_loading(loading_pos, m_loading_cycle, 1.f - m_show_animation, m_loading_text);
		}

		// render skeleton if we're on load
		{
			auto skeleton_color = color_t{ 100, 100, 100 }.modify_alpha((1.f - m_show_animation) * 0.1f + m_skeleton_pulsating * 0.25f);

			// tabs
			for (int i = 0; i < 6; ++i)
				render::filled_rect(m_position + dpi::scale(vec2d{ 16.f, 89.f + 40.f * i }), dpi::scale(vec2d{ 164.f, 32.f }), skeleton_color, dpi::scale(5.f));

			// footer
			render::filled_rect(m_position + dpi::scale(vec2d{ 20.f, 432.f }), dpi::scale(vec2d{ 152.f, 40.f }), skeleton_color, dpi::scale(5.f));

			// search field
			render::filled_rect(m_position + dpi::scale(vec2d{ 216.f, 38.f }), dpi::scale(vec2d{ 532.f, 32.f }), skeleton_color, dpi::scale(5.f));

			// active tab & subtab
			render::filled_rect(m_position + dpi::scale(vec2d{ 221.f, 17.f }), dpi::scale(vec2d{ 120.f, 14.f }), skeleton_color, dpi::scale(5.f));
		}

		m_skeleton_pulsating.trigger(ctx, m_loading, 1.5f);
		m_show_animation.lerp(ctx, m_loading ? 0.f : 1.f);
		if (m_show_animation < 1.f)
			return;

		ctx.m_alpha = std::min<float>(std::clamp(ctx.m_alpha + ctx.m_time_delta * 4.f, 0.f, 1.f), m_alpha);
		m_loading_cycle = 0.5f;

		ctx.m_cursor.x += dpi::scale(sidebar_width);
		ctx.m_can_update = this->m_can_update;

		vec2d items_size{};
		for (auto item: m_items) {
			render_item(item, ctx);
			items_size += ctx.apply(item, dpi::scale(styles::get().container_padding));
		}

		auto backup_hovered_item = ctx.m_hovered_item;
		ctx.reset();

		for (auto popup: m_popups)
			render_item(popup, ctx);

		if (!m_popups_to_delete.empty()) {
			for (auto popup: m_popups_to_delete)
				delete popup;

			m_popups_to_delete.clear();
		}

		if (m_update_callback != nullptr) {
			containers::recursive_delete(m_items);
			for (auto popup: m_popups) {
				containers::recursive_delete(popup->get_items());
				delete popup;
			}

			m_popups.clear();
			m_items.clear();
			m_update_callback();

			if (!m_silent_update)
				ctx.m_alpha = 0.f;

			ctx.m_hovered_item = nullptr;
			m_update_callback = nullptr;
		} else {
			if (ctx.m_hovered_item == nullptr)
				ctx.m_hovered_item = backup_hovered_item;

			if (m_can_update && ctx.m_hovered_item != nullptr && globals().mouse_click[1]) {
				const auto is_exist =
						containers::recursive_find(m_items, [hovered = ctx.m_hovered_item](auto* item) { return item == hovered; }) != nullptr ||
						containers::recursive_find(m_popups, [hovered = ctx.m_hovered_item](auto* item) { return item == hovered; }) != nullptr;

				if (is_exist) {
					auto value_ptr = ctx.m_hovered_item->value_pointer();
					if (value_ptr != nullptr) {
						auto popup = create_popup();
						popup->m_flags.add(container_flag_is_popup | popup_flag_animation | popup_flag_close_on_click);
						popup->set_position(globals().mouse_position - ctx.m_base_position);

						static int current_action = -1;
						auto actions = controls::selectables(&current_action, { STRS("Copy reference") }, false, dpi::scale(160.f), [=](bool) {
							switch (current_action) {
								case 0: std::thread(copy_varname, value_ptr).detach(); break;
							}

							current_action = -1;
						});

						popup->add(actions);
					}
				}
			}
		}
	}
} // namespace NS_GUI::instance