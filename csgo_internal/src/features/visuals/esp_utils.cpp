#include "esp_utils.hpp"
#include "../../../deps/weave-gui/include.hpp"
#include "../../game/engine_prediction.hpp"
#include "../../interfaces.hpp"
#include "../../utils/math.hpp"

using namespace gui;

namespace esp {
	bool bounding_box_t::get(vec3d min, vec3d max, const matrix3x4_t& coordinate_frame) {
		vec3d points[8] = {
			{ min.x, min.y, min.z },
			{ min.x, max.y, min.z },
			{ max.x, max.y, min.z },
			{ max.x, min.y, min.z },
			{ max.x, max.y, max.z },
			{ min.x, max.y, max.z },
			{ min.x, min.y, max.z },
			{ max.x, min.y, max.z }
		};

		int valid_bounds = 0;
		vec2d points_to_screen[8] = {};
		for (int i = 0; i < 8; i++) {
			vec3d v1{};
			math::vector_transform(points[i], coordinate_frame, v1);
			if (!math::world_to_screen(v1, points_to_screen[i]))
				continue;

			valid_bounds++;
		}

		if (valid_bounds == 0)
			return false;

		float left = points_to_screen[3].x;
		float top = points_to_screen[3].y;
		float right = points_to_screen[3].x;
		float bottom = points_to_screen[3].y;

		for (auto i = 1; i < 8; i++) {
			if (left > points_to_screen[i].x)
				left = points_to_screen[i].x;
			if (top < points_to_screen[i].y)
				top = points_to_screen[i].y;
			if (right < points_to_screen[i].x)
				right = points_to_screen[i].x;
			if (bottom > points_to_screen[i].y)
				bottom = points_to_screen[i].y;
		}

		x = std::round(left);
		y = std::round(bottom);
		w = std::round(right - left);
		h = std::round(top - bottom);
		return true;
	}

	/*
		TODO:

		bool c_player_esp::get_bounding_box( valve::c_base_player* entity, box_t& box ) const {
		const auto adjust = math::c_vec3( 0, 0, -18 ) * entity->get_duck_amount( );
		const auto air = !( entity->get_flags( ) & valve::fl_on_ground ) && ( entity->get_move_type( ) != valve::move_type_ladder ) ?
			math::c_vec3( 0, 0, 10 ) : math::c_vec3( 0, 0, 0 );

		const auto down = entity->get_origin( ) + air;
		const auto top = down + math::c_vec3( 0, 0, 72 ) + adjust;

		math::c_vec3 s[ 2 ];
		if ( g_render->world_to_screen( top, s[ 1 ] ) && g_render->world_to_screen( down, s[ 0 ] ) ) {
			const math::c_vec3 delta = s[ 1 ] - s[ 0 ];

			box.h = fabsf( delta.y );
			box.w = box.h / 2.0f;

			box.x = s[ 1 ].x - ( box.w / 2 );
			box.y = s[ 1 ].y;

			return true;
		}

		return false;
	}

	*/

	bool bounding_box_t::get(vec3d render_origin, float duck_amount) {
		auto adjust = vec3d{ 0.f, 0.f, -16.f } * duck_amount;

		auto down = render_origin - vec3d{ 0.f, 0.f, 1.f };
		auto top = down + vec3d{ 0.f, 0.f, 72.f } + adjust;

		vec2d s[2]{};
		if (math::world_to_screen(top, s[1]) && math::world_to_screen(down, s[0])) {
			h = std::abs((s[1] - s[0]).y);
			w = h / 2.0f;

			x = s[1].x - w / 2.f;
			y = s[1].y;
			return true;
		}

		return false;
	}

	void bounding_box_t::render_base(color_t color) {
		constexpr auto thickness = 3.f;
		if (w <= thickness * 2.f || h <= thickness * 2.f)
			return;

		SET_AND_RESTORE(render::draw_list->Flags, ImDrawListFlags_None);

		auto outline_color = color_t{ 12, 12, 12, (uint8_t)(m_alpha * m_alpha * 255.f) };
		render::rect(x, y, w, h, outline_color, 0.f, 0, dpi::scale(1.f));
		render::rect(std::round(x + dpi::scale(1.f)), std::round(y + dpi::scale(1.f)), std::round(w - dpi::scale(2.f)), std::round(h - dpi::scale(2.f)), color.modify_alpha(m_alpha), 0.f, 0, std::round(dpi::scale(1.f)));
		render::rect(std::round(x + dpi::scale(2.f)), std::round(y + dpi::scale(2.f)), std::round(w - dpi::scale(4.f)), std::round(h - dpi::scale(4.f)), outline_color, 0.f, 0, std::round(dpi::scale(1.f)));
	}

	void bounding_box_t::bar(bar_value_t&& value) {
		SET_AND_RESTORE(render::draw_list->Flags, ImDrawListFlags_None);

		vec2d pos{}, size{};
		bool horizontal = false;
		auto& current_offset = m_render_state.m_offsets[(uintptr_t)value.m_pos];

		switch (value.m_pos) {
			case e_pos_type::bottom:
				size = { w, m_bar_size };
				pos = { 0.f, h + dpi::scale(m_padding) + current_offset };
				horizontal = true;
				break;
			case e_pos_type::top:
				size = { w, m_bar_size };
				pos = { 0.f, -dpi::scale(m_padding) - dpi::scale(m_bar_size) - current_offset };
				horizontal = true;
				break;
			case e_pos_type::left:
				size = { m_bar_size, h };
				pos = { -dpi::scale(m_padding) - dpi::scale(m_bar_size) - current_offset, 0.f };
				break;
			case e_pos_type::right:
				size = { m_bar_size, h };
				pos = { w + dpi::scale(m_padding) + current_offset, 0.f };
				break;
			default:
				return;
		}

		pos.round();
		size.round();

		const float bar_fill = std::clamp(value.m_visual_value / value.m_max, 0.f, 1.f);

		if (horizontal) {
			render::filled_rect(x + pos.x, y + pos.y - dpi::scale(1.f), size.x, size.y + dpi::scale(2.f), color_t{ 35, 35, 35, (uint8_t)(255 * m_alpha) }, 0.f);

			pos.x += dpi::scale(1.f);
			size.x -= dpi::scale(2.f);

			pos.round();
			size.round();

			render::filled_rect_gradient(x + pos.x, y + pos.y, size.x * bar_fill, size.y,
										 value.m_colors.first.modify_alpha(m_alpha), value.m_colors.second.modify_alpha(m_alpha),
										 value.m_colors.second.modify_alpha(m_alpha), value.m_colors.first.modify_alpha(m_alpha));

			if (value.m_show_number)
				render::textf(x + pos.x + size.x * bar_fill, y + pos.y, color_t{ 255, 255, 255, (uint8_t)(255.F * m_alpha) },
							  render::centered_x | render::centered_y | render::outline, fonts::esp_small, STRC("%d"), (int)value.m_value);
		} else {
			render::filled_rect(x + pos.x - dpi::scale(1.f), y + pos.y, size.x + dpi::scale(2.f), size.y, color_t{ 35, 35, 35, (uint8_t)(255.f * m_alpha) }, 0.f);

			pos.y += dpi::scale(1.f);
			size.y -= dpi::scale(2.f);

			pos.round();
			size.round();

			render::filled_rect_gradient(x + pos.x, y + pos.y + size.y * (1.f - bar_fill), size.x, size.y * bar_fill,
										 value.m_colors.first.modify_alpha(m_alpha), value.m_colors.first.modify_alpha(m_alpha),
										 value.m_colors.second.modify_alpha(m_alpha), value.m_colors.second.modify_alpha(m_alpha));

			if (value.m_show_number)
				render::textf(x + pos.x, y + pos.y + size.y * (1.f - bar_fill), color_t{ 255, 255, 255, (uint8_t)(255.f * m_alpha) },
							  render::centered_x | render::centered_y | render::outline, fonts::esp_small, STRC("%d"), (int)value.m_value);
		}

		current_offset += m_bar_size + 1 /* outline */ + m_padding;
	}

	void bounding_box_t::text(std::string text, e_pos_type _pos, std::pair<color_t, color_t> colors, font_t font) {
		if (text.empty())
			return;

		vec2d pos{};
		font = font == nullptr ? fonts::esp_default : font;
		auto size = render::calc_text_size(text.c_str(), font);
		auto& current_offset = m_render_state.m_offsets[(uintptr_t)_pos];

		current_offset += dpi::scale(m_padding);

		switch (_pos) {
			case e_pos_type::bottom:
				pos = { w / 2.f - size.x / 2.f, h + current_offset };
				current_offset += size.y;
				break;
			case e_pos_type::top:
				pos = { w / 2.f - size.x / 2.f, -current_offset - size.y };
				current_offset += size.y;
				break;
			case e_pos_type::left:
				pos = { -size.x - current_offset, 0.f };
				current_offset += size.x;
				break;
			case e_pos_type::right:
				pos = { w + current_offset, 0.f };
				current_offset += size.x;
				break;
			default:
				return;
		}

		pos += { x, y };
		pos = pos.round();

		ImDrawGradient_Linear gradient{ render::to_imvec2(pos), render::to_imvec2(pos + size),
										render::to_imvec4(colors.first.modify_alpha(m_alpha)),
										render::to_imvec4(colors.second.modify_alpha(m_alpha)) };

		const auto outline_clr = color_t{ 0, 0, 0, int(m_alpha * 255.f * 0.3f) };

		render::draw_list->PushTextureID(font->ContainerAtlas->TexID);

		render::draw_list->AddText(font, font->FontSize, { pos.x + 1, pos.y - 1 }, outline_clr.abgr(), text.c_str());
		render::draw_list->AddText(font, font->FontSize, { pos.x - 1, pos.y + 1 }, outline_clr.abgr(), text.c_str());
		render::draw_list->AddText(font, font->FontSize, { pos.x - 1, pos.y - 1 }, outline_clr.abgr(), text.c_str());
		render::draw_list->AddText(font, font->FontSize, { pos.x + 1, pos.y + 1 }, outline_clr.abgr(), text.c_str());

		render::draw_list->AddText(font, font->FontSize, { pos.x, pos.y + 1 }, outline_clr.abgr(), text.c_str());
		render::draw_list->AddText(font, font->FontSize, { pos.x, pos.y - 1 }, outline_clr.abgr(), text.c_str());
		render::draw_list->AddText(font, font->FontSize, { pos.x + 1, pos.y }, outline_clr.abgr(), text.c_str());
		render::draw_list->AddText(font, font->FontSize, { pos.x - 1, pos.y }, outline_clr.abgr(), text.c_str());

		render::draw_list->AddText(font, font->FontSize, render::to_imvec2(pos), gradient, text.c_str());

		render::draw_list->PopTextureID();
	}

	void bounding_box_t::text_array(text_array_t&& text_array, e_pos_type _pos, font_t font) {
		if (text_array.empty())
			return;

		font = font == nullptr ? fonts::esp_small : font;

		auto& current_offset = m_render_state.m_offsets[(uintptr_t)_pos];
		float max_x_size{};
		vec2d pos{};

		switch (_pos) {
			case e_pos_type::bottom:
				pos = { 0.f, h + current_offset };
				break;
			case e_pos_type::top:
				pos = { 0.f, -current_offset };
				break;
			case e_pos_type::left:
				pos = { -current_offset, 0.f };
				break;
			case e_pos_type::right:
				pos = { w + current_offset, 0.f };
				break;
			default:
				return;
		}

		pos = pos.round();

		float total_size = (render::calc_text_size(text_array[0].m_text, font).y + dpi::scale(m_padding)) * text_array.size();
		float cursor_pos = _pos == e_pos_type::top ? -total_size : 0.f;

		for (size_t i = 0u; i < text_array.size(); ++i) {
			const auto& it = text_array[i];
			const auto it_size = render::calc_text_size(it.m_text.c_str(), font);

			if (_pos == e_pos_type::left)
				render::text(x + pos.x - dpi::scale(m_padding) - it_size.x, y + pos.y + cursor_pos, it.m_color, render::outline, font, it.m_text.c_str());
			else
				render::text(x + pos.x + dpi::scale(m_padding), y + pos.y + cursor_pos, it.m_color, render::outline, font, it.m_text.c_str());

			if (it_size.x > max_x_size)
				max_x_size = it_size.x;

			cursor_pos += it_size.y + dpi::scale(m_padding);
		}

		switch (_pos) {
			case e_pos_type::bottom:
			case e_pos_type::top:
				current_offset += total_size + dpi::scale(m_padding);
				break;
			case e_pos_type::left:
			case e_pos_type::right:
				current_offset += max_x_size + dpi::scale(m_padding);
				break;
			default:
				return;
		}
	}

	__forceinline font_t get_font(int id) {
		switch (id) {
			case 0:
				return fonts::esp_default;
			case 1:
				return fonts::menu_desc;
			case 2:
				return fonts::esp_small;

			case 3:
				return fonts::menu_main;
			case 4:
				return fonts::menu_bold;
			case 5:
				return fonts::menu_big;
			case 6:
				return fonts::menu_small;
			case 7:
				return fonts::menu_small_semibold;
			case 8:
				return fonts::menu_small_bold;

			default:
				return fonts::esp_default;
		}
	}
} // namespace esp