#include "logs.hpp"
#include "../../../deps/weave-gui/include.hpp"
#include "../../blur/blur.hpp"
#include "../../interfaces.hpp"
#include "../../render.hpp"
#include "../../utils/easings.hpp"

using namespace gui;

void console_t::clear() {
	interfaces::engine->client_cmd_unrestricted(STRC("clear"));
}

void console_t::print(const std::string& text) {
	interfaces::convar->console_color_printf(color_t{ 160, 160, 160 }, STRC("%s"), text.c_str());
}

void console_t::print_colored_id(const std::string& str) {
	if (interfaces::convar == nullptr)
		return;

	auto get_color = [](int id) -> color_t {
		switch (id) {
			default:
			case 0: return color_t{ 255, 255, 255 }; // white
			case 1: return color_t{ 219, 98, 23 };	 // orange
			case 2: return color_t{ 127, 127, 127 }; // grey
			case 3: return color_t{ 255, 170, 3 };	 // yellow
			case 4: return color_t{ 255, 35, 10 };	 // red
			case 5: return color_t{ 71, 229, 68 };	 // green
		}
	};

	int color = 0 /* default color index */;
	ptrdiff_t start = 0, str_len = str.length();
	for (ptrdiff_t i = 0; i < str_len; ++i) {
		if (str[i] == '$') {
			if (i - start > 0)
				interfaces::convar->console_color_printf(get_color(color), STRC("%s"), str.substr(start, i - start).c_str());

			++i;
			color = str[i] - '0';
			start = i + 1;
		}
	}

	interfaces::convar->console_color_printf(get_color(color), STRC("%s"), str.substr(start).c_str());
}

void console_t::print_gradient_text(std::string text, std::pair<color_t, color_t> colors, int line, int total_lines) {
	int max_line_size = 0;
	if (total_lines == 0) {
		int line_size = 0;
		for (auto c: text) {
			if (c == '\n') {
				++total_lines;

				if (line_size > max_line_size)
					max_line_size = line_size;

				line_size = 0;
			} else
				++line_size;
		}

		line = 0;
	}

	if (max_line_size == 0)
		max_line_size = text.length();

	int current_symbol = 0;
	for (size_t i = 0; i < text.length(); ++i) {
		if (total_lines != 0) {
			const auto lines_lerp = (float)line / (float)total_lines;
			const auto line_ratio = std::clamp((float)current_symbol / (float)max_line_size, 0.f, 1.f);
			interfaces::convar->console_color_printf(colors.first.lerp(colors.second, (line_ratio + lines_lerp) * 0.5f), STRC("%c"), text[i]);

			if (text[i] == '\n') {
				++line;
				current_symbol = 0;
			} else
				++current_symbol;
		} else
			interfaces::convar->console_color_printf(colors.first.lerp(colors.second, std::clamp((float)i / (float)max_line_size, 0.f, 1.f)), STRC("%c"), text[i]);
	}
}

void cheat_logs_t::add(const std::string& message, message_t::e_type type) {
	if (interfaces::global_vars == nullptr)
		return;

	THREAD_SAFE(mtx);
	auto offset = 0.f;
	for (auto& log: logs)
		if (log.m_size > 0.f)
			offset += log.m_size + dpi::scale(4.f);

	logs.emplace_back(message, type, interfaces::global_vars->m_realtime, offset);
}

void cheat_logs_t::render() {
	THREAD_SAFE(mtx);
	const auto start_pos = dpi::scale(vec2d{ 32.f, 32.f });

	constexpr auto get_type_str = [](message_t::e_type type) -> std::string {
		switch (type) {
			case message_t::e_type::miss:
				return STRS("MISS");
			case message_t::e_type::hit:
				return STRS("HIT");
			case message_t::e_type::buy:
				return STRS("BUY");
			case message_t::e_type::error:
				return STRS("ERROR");
			default:
			case message_t::e_type::info:
				return STRS("INFO");
		}
	};

	constexpr auto lerp_offset = [](float value, float target, float& velocity) {
		velocity = interfaces::global_vars->m_frametime * std::abs(target - value) * 37.f;

		if (value > target)
			return std::max<float>(value - velocity, target);
		else if (value < target)
			return std::min<float>(value + velocity, target);

		velocity = 0.f;
		return value;
	};

	float total_size = 0.f;

	for (size_t i = 0u; i < logs.size(); ++i) {
		auto& cur = logs[i];

		if (cur.m_size > 0.f) {
			if (i == 0) {
				cur.m_offset = lerp_offset(cur.m_offset, 0.f, cur.m_velocity);
			} else {
				auto& prev = logs[i - 1];
				if (prev.m_size > 0.f)
					cur.m_offset = lerp_offset(cur.m_offset, prev.m_offset + prev.m_size + dpi::scale(4.f), cur.m_velocity);
			}
		}

		constexpr auto text_wrap_weight = 450.f;

		const auto font = fonts::menu_small_semibold;
		const auto font_bold = fonts::menu_small_bold;

		const auto type_size = dpi::scale(vec2d{ 26.f, 26.f });

		const auto text_size = render::calc_text_size(cur.m_text, font, dpi::scale(text_wrap_weight));
		const auto bg_size = vec2d{ text_size.x + dpi::scale(6.f + 6.f), std::max<float>(dpi::scale(26.f), text_size.y + dpi::scale(12.f)) };

		const auto text_len_fixed = std::count_if(cur.m_text.begin(), cur.m_text.end(), [](unsigned char c) { return !(std::isspace(c) || std::ispunct(c)); });

		const auto showtime = std::clamp(text_len_fixed * showtime_per_symbol, min_showtime, max_showtime);
		const auto time_delta = std::abs(interfaces::global_vars->m_realtime - cur.get_time());
		const auto in_time_ratio = std::min<float>(time_delta / animation_time_in, 1.f);
		const auto time_animation = std::clamp(time_delta / showtime, 0.f, 1.f);

		float out_time_ratio = 0.f;
		if (time_delta > showtime)
			out_time_ratio = std::min<float>((time_delta - showtime) / animation_time_out, 1.f);

		if (out_time_ratio >= 1.f)
			cur.m_outdated = true;

		const auto ease_in = easings::out_circ(in_time_ratio);
		const auto ease_out = easings::in_circ(out_time_ratio);

		const auto current_pos = start_pos + vec2d{ std::round(-(1.f - ease_in * (1.f - ease_out)) * bg_size.x), cur.m_offset };
		const auto alpha = easings::out_sine(in_time_ratio * (1.f - out_time_ratio));

		cur.m_size = bg_size.y;

		// background
		if (bg_size.y > dpi::scale(26.f))
			render::filled_rect(current_pos + vec2d{ type_size.x, 0.f }, bg_size,
								color_t{ 12, 12, 12 }.modify_alpha(alpha), dpi::scale(6.f), ImDrawCornerFlags_Right | ImDrawCornerFlags_BotLeft);
		else
			render::filled_rect(current_pos + vec2d{ type_size.x, 0.f }, bg_size, color_t{ 12, 12, 12 }.modify_alpha(alpha), dpi::scale(6.f), ImDrawCornerFlags_Right);

		const auto type_pos = current_pos;

		std::pair<color_t, color_t> gradient_colors{};

		render::gif_t* type_texture = (render::gif_t*)resources::get_texture(HASH("info"));

		// type
		switch (cur.m_type) {
			case message_t::e_type::miss:
				gradient_colors.first = color_t::lerp({ 241, 144, 90 }, { 249, 233, 149 }, time_animation);
				gradient_colors.second = color_t::lerp({ 245, 80, 80 }, { 255, 115, 166 }, time_animation);
				type_texture = (render::gif_t*)resources::get_texture(HASH("miss"));
				break;
			case message_t::e_type::hit:
				gradient_colors.first = color_t::lerp({ 255, 206, 112 }, { 111, 192, 203 }, time_animation);
				gradient_colors.second = color_t::lerp({ 193, 255, 60 }, { 134, 207, 137 }, time_animation);
				type_texture = (render::gif_t*)resources::get_texture(HASH("hit"));
				break;
			case message_t::e_type::error:
				gradient_colors.first = color_t::lerp({ 252, 151, 78 }, { 248, 116, 251 }, time_animation);
				gradient_colors.second = color_t::lerp({ 226, 52, 84 }, { 255, 213, 62 }, time_animation);
				type_texture = (render::gif_t*)resources::get_texture(HASH("error"));
				break;
			default:
			case message_t::e_type::buy:
				type_texture = (render::gif_t*)resources::get_texture(HASH("buy"));
				__fallthrough;
			case message_t::e_type::info:
				gradient_colors.first = color_t::lerp({ 237, 126, 255 }, { 122, 151, 255 }, time_animation);
				gradient_colors.second = color_t::lerp({ 143, 148, 255 }, { 103, 255, 164 }, time_animation);
				break;
		}

		ImDrawGradient_Linear gradient{
			render::to_imvec2(type_pos),
			render::to_imvec2(type_pos + type_size),
			render::to_imvec4(gradient_colors.first.modify_alpha(alpha)),
			render::to_imvec4(gradient_colors.second.modify_alpha(alpha)),
		};

		render::draw_list->AddRectFilled(render::to_imvec2(type_pos), render::to_imvec2(type_pos + type_size), gradient, dpi::scale(6.f), ImDrawFlags_RoundCornersLeft);

		//render::text(current_pos.x + type_size.x * 0.5f,
		//			 current_pos.y + type_size.y * 0.5f,
		//			 color_t{}.modify_alpha(alpha), render::centered, font_bold, icon);

		const auto icon_size = dpi::scale(vec2d{ 14.f, 14.f });
		render::image(current_pos + dpi::scale(vec2d{ 6.f, 6.f }), icon_size, type_texture->m_textures[0].first, color_t{}.modify_alpha(alpha));

		// text
		if (bg_size.y > dpi::scale(24.f))
			render::text(
					current_pos.x + dpi::scale(6.f) + type_size.x,
					current_pos.y + dpi::scale(6.f),
					color_t{}.modify_alpha(alpha), render::none, font, cur.m_text, dpi::scale(text_wrap_weight));
		else
			render::text(
					current_pos.x + dpi::scale(6.f) + type_size.x,
					current_pos.y + bg_size.y * 0.5f,
					color_t{}.modify_alpha(alpha), render::centered_y, font, cur.m_text, dpi::scale(text_wrap_weight));

		total_size += cur.m_size + dpi::scale(8.f);
	}

	logs.erase(std::remove_if(logs.begin(), logs.end(), [](const message_t& msg) { return msg.m_outdated; }), logs.end());

	if (total_size > render::screen_height * 0.5f)
		logs.erase(logs.begin());
}