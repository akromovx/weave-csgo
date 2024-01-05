#include "indicators.hpp"
#include "../../../deps/imgui/imgui.h"
#include "../../blur/blur.hpp"
#include "../../game/override_entity_list.hpp"
#include "../../globals.hpp"
#include "../../utils/hotkeys.hpp"

static gui::instance::window_t* window = nullptr;

auto lerp_float = [](float& value, float target) {
	if (value > target)
		value = std::clamp(value - std::abs(value - target) * gui::globals().time_delta * 9.f, target, FLT_MAX);
	else if (value < target)
		value = std::clamp(value + std::abs(value - target) * gui::globals().time_delta * 9.f, -FLT_MAX, target);
};

struct hotkey_info_t {
	std::string m_name{};
	std::string m_mode{};
};

STFI std::vector<hotkey_info_t> collect_active_hotkeys() {
	std::vector<hotkey_info_t> result{};
	result.reserve(hotkeys->m_hotkeys.size());

	for (size_t i = 0; i < hotkeys->m_hotkeys.size(); ++i) {
		auto hotkey = hotkeys->m_hotkeys[i];
		if (hotkey->is_valid() && hotkey->m_active && hotkey->m_show_in_binds) {
			std::string mode{};
			switch (hotkey->m_type) {
				case hotkey_t::e_type::always_on: mode = STR("[ON]"); break;
				case hotkey_t::e_type::hold: mode = STR("[H]"); break;
				case hotkey_t::e_type::toggle: mode = STR("[T]"); break;
			}

			result.emplace_back(hotkey->m_actual_name, mode);
		}
	}

	return result;
}

static void render_binds(gui::render_context_t& ctx, gui::instance::window_t* window) {
	using namespace gui;

	const auto& active_hotkeys = collect_active_hotkeys();
	window->m_opened = settings->misc.hotkeys_list && (!active_hotkeys.empty() || menu::open);

	constexpr auto header_size = 32.f;

	const auto font = fonts::menu_main;

	vec2d window_size = {};
	static vec2d visual_size = window->get_size();

	shaders::create_blur(ImGui::GetBackgroundDrawList(),
						 to_imvec2(ctx.m_base_position),
						 to_imvec2(ctx.m_base_position + visual_size),
						 color_t{ 127, 127, 127 }.modify_alpha(ctx.m_alpha).abgr(), dpi::scale(5.f));

	// header
	{
		const vec2d pos = ctx.m_base_position;

		render::filled_rect(pos, { visual_size.x, dpi::scale(header_size) },
							styles::get().window_backround.modify_alpha(ctx.m_alpha), dpi::scale(5.f), ImDrawCornerFlags_Top);

		render::image(pos + dpi::scale(vec2d{ 16.f, 9.f }), dpi::scale(vec2d{ 14.f, 14.f }), ((render::gif_t*)resources::get_texture(HASH("hotkeys_icon")))->m_textures[0].first, color_t{}.modify_alpha(ctx.m_alpha));
		render::text(pos.x + dpi::scale(40.f), pos.y + dpi::scale(header_size) * 0.5f, color_t{}.modify_alpha(ctx.m_alpha), render::centered_y, fonts::menu_bold, STR("Hotkeys"));

		auto clr1 = styles::get().accent_color1.modify_alpha(ctx.m_alpha);
		auto clr2 = styles::get().accent_color2.modify_alpha(ctx.m_alpha);

		render::filled_rect_gradient(pos.x, pos.y + dpi::scale(header_size), visual_size.x, dpi::scale(2.f), clr1, clr2, clr2, clr1);

		window_size += dpi::scale(vec2d{ 109.f, header_size + 2.f });
	}

	// bind list
	{
		constexpr auto padding = 6.f;
		float cursor = dpi::scale(header_size + 2.f + padding);
		float height = 6.f;

		render::lock(ctx.m_base_position, visual_size);

		for (const auto& hotkey: active_hotkeys) {
			const auto mode_size = render::calc_text_size(hotkey.m_mode, font);
			const auto name_size = render::calc_text_size(hotkey.m_name, font);

			render::text(
					ctx.m_base_position.x + dpi::scale(14.f),
					ctx.m_base_position.y + cursor,
					color_t{}.modify_alpha(ctx.m_alpha), render::none, font, hotkey.m_name);

			render::text(
					ctx.m_base_position.x + visual_size.x - dpi::scale(14.f) - mode_size.x,
					ctx.m_base_position.y + cursor,
					color_t{}.modify_alpha(ctx.m_alpha * 0.5f), render::none, font, hotkey.m_mode);

			const auto this_line = vec2d{
				dpi::scale(14.f + 14.f) + mode_size.x + name_size.x + dpi::scale(9.f),
				name_size.y + dpi::scale(padding)
			};

			window_size.x = std::max<float>(window_size.x, this_line.x);
			cursor += this_line.y;
			height += this_line.y;
		}

		render::unlock();

		height += dpi::scale(2.f);
		window_size.y += height;
	}

	lerp_float(visual_size.x, window_size.x);
	lerp_float(visual_size.y, window_size.y);
	window->set_size(visual_size);
}

void indicators_t::render() {
	if (window == nullptr) {
		window = new gui::instance::window_t{ { gui::dpi::scale(109.f), 0.f }, { 200.f, 500.f } };
		window->override_render_function(render_binds);
		window->m_opened = true;
		gui::add_instance(window);
	}
}