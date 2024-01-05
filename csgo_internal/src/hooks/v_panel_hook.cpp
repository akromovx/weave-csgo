#include "hooker.hpp"
#include "hooks.hpp"

#include "../game/players.hpp"
#include "../game/weapons.hpp"
#include "../interfaces.hpp"
//#include "../features/exmenu.hpp"
#include "../features/menu.hpp"

namespace hooks::v_panel {

	static uint32_t verdana{};

	namespace surface_render {
		static void draw_string(uint32_t font, vec2d pos, color_t color, int flags, const char* msg, ...) {
			va_list va_alist;
			char buf[1024];
			va_start(va_alist, msg);
			_vsnprintf(buf, sizeof(buf), msg, va_alist);
			va_end(va_alist);
			wchar_t wbuf[1024];
			MultiByteToWideChar(CP_UTF8, 0, buf, 256, wbuf, 256);

			int width, height;
			interfaces::surface->get_text_size(font, wbuf, width, height);

			if (flags & render::centered_x)
				pos.x -= width * 0.5f;
			if (flags & render::centered_y)
				pos.y -= height * 0.5f;

			interfaces::surface->draw_set_text_font(font);
			interfaces::surface->draw_set_text_color(color.r(), color.g(), color.b(), color.a());
			interfaces::surface->draw_set_text_pos(pos.x, pos.y);
			interfaces::surface->draw_print_text(wbuf, wcslen(wbuf), sdk::e_font_draw_type::font_draw_default);
		}

		static void filled_rect(vec2d pos, vec2d size, color_t color) {
			interfaces::surface->draw_set_color(color);
			interfaces::surface->draw_filled_rect((int)pos.x, (int)pos.y, (int)pos.x + (int)size.x, (int)pos.y + (int)size.y);
		}
	} // namespace surface_render

	void __stdcall paint_traverse(unsigned int current_panel, bool force_repaint, bool allow_force) {
		static auto original = vmt::vpanel->original<void(__thiscall*)(void*, unsigned int, bool, bool)>(XOR32(41));
		static std::array<unsigned int, 3> panels{};

		static bool once = false;
		if (!once) {
			verdana = interfaces::surface->create_font();
			interfaces::surface->set_font_glyph_set(verdana, STRSC("Tahoma"), 18, FW_SEMIBOLD, NULL, NULL, /* FONTFLAG_OUTLINE */ 0x200, 500, 700);
			once = true;
		}

		constexpr hash_t panel_names[] = {
			fnva1("FocusOverlayPanel"),
			fnva1("MatSystemTopPanel"),
			fnva1("HudZoom")
		};

		int cnt = 0;
		for (int i = 0; i < 3; ++i) {
			if (panels[i] > 0) {
				++cnt;
				continue;
			}

			if (!panels[i])
				if (fnva1(interfaces::v_panel->get_name(current_panel)) == panel_names[i])
					panels[i] = current_panel;

			if (cnt > 3)
				break;
		}

		if (current_panel == panels[hud_zoom] && settings->visuals.removals.at(3))
			return;

		if (current_panel == panels[mat_sys_top_panel]) {
			players->on_paint_traverse();
			weapons->on_paint_traverse();
		}

		if (current_panel == panels[focus_overlay_panel]) {
			interfaces::v_panel->set_mouse_input_enabled(current_panel, menu::open);
		}

		original(interfaces::v_panel, current_panel, force_repaint, allow_force);

		if (current_panel == panels[focus_overlay_panel]) {
			animations_pt->update();

			if (!render::can_render || !gui::resources::downloaded) {
				if (!render::current_load_stage_name.empty()) {
					int w{}, h{};
					interfaces::engine->get_screen_size(w, h);
					interfaces::surface->draw_set_color({ 0, 0, 0, 160 });
					interfaces::surface->draw_filled_rect(0, 0, w, h);

					if (render::total_load_stages.load() != 0) {
						surface_render::draw_string(verdana, { w * 0.5f, h * 0.5f }, color_t{}, render::centered,
													dformat(STR("{} ({}/{})"),
															render::current_load_stage_name,
															std::min<int>(render::current_load_stage.load(), render::total_load_stages.load()),
															render::total_load_stages.load())
															.c_str());

						const auto bar_size = vec2d{ 375.f, 12.f };
						animations_pt->ease_lerp(render::current_load_animated, (float)render::current_load_stage.load() / ((float)render::total_load_stages.load() + FLT_EPSILON));
						surface_render::filled_rect({ w * 0.5f - bar_size.x * 0.5f, h * 0.5f - bar_size.y * 0.5f + 32.f }, bar_size, color_t{ 35, 35, 35, 255 });
						surface_render::filled_rect({ w * 0.5f - bar_size.x * 0.5f, h * 0.5f - bar_size.y * 0.5f + 32.f }, { bar_size.x * render::current_load_animated, bar_size.y }, color_t{ 254, 103, 49 });
					} else
						surface_render::draw_string(verdana, { w * 0.5f, h * 0.5f }, color_t{}, render::centered,
													dformat(STR("{}"), render::current_load_stage_name).c_str());
				}
			} else
				surface_render::filled_rect({}, { 1.f, 1.f }, { 0, 0, 0, 1 });
		}
	}
} // namespace hooks::v_panel
