#include "player_esp.hpp"

#include "../../globals.hpp"
#include "../../interfaces.hpp"
#include "../../utils/easings.hpp"

#include "../../game/dormant.hpp"
#include "../../game/engine_prediction.hpp"
#include "../../game/override_entity_list.hpp"
#include "../../game/players.hpp"

#include "../../../deps/weave-gui/include.hpp"

using namespace gui;

namespace esp {
	STFI void render_box(const player_entry_t& entry, bounding_box_t& box) {
		auto& set = settings->player_esp;

		if (set.box)
			box.render_base(set.box_color.get());

		if (set.name.value)
			box.text(entry.m_name, (bounding_box_t::e_pos_type)set.name.position, get_gradient(set.name.colors), get_font(set.name.font));

		const auto dormant_color = color_t{ 255, 255, 255, (int)(box.m_alpha * 255) };
		const std::pair<color_t, color_t> dormant_gradient = { dormant_color, dormant_color.decrease(25) };

		if (set.health.value)
			box.bar({ 100.f, entry.m_visual_health, (float)entry.m_health,
					  entry.m_dormant ? dormant_gradient : esp::players->get_health_color(entry.m_health, box.m_alpha),
					  (bounding_box_t::e_pos_type)set.health.position, entry.m_health < 100 });

		if (entry.m_weapon.m_valid) {
			if (set.ammo.value && entry.m_weapon.m_ammo > 0)
				box.bar({ (float)entry.m_weapon.m_max_ammo, (float)entry.m_weapon.m_max_ammo, (float)entry.m_weapon.m_ammo, entry.m_dormant ? dormant_gradient : get_gradient(set.ammo.colors),
						  (bounding_box_t::e_pos_type)set.ammo.position, entry.m_weapon.m_ammo < entry.m_weapon.m_max_ammo });

			if (set.weapon.value.at(1) && !entry.m_weapon.m_icon.empty())
				box.text(entry.m_weapon.m_icon, (bounding_box_t::e_pos_type)set.weapon.position, get_gradient(set.weapon.colors), fonts::weapon_icons);

			if (set.weapon.value.at(0) && !entry.m_weapon.m_name.empty())
				box.text(entry.m_weapon.m_name, (bounding_box_t::e_pos_type)set.weapon.position, get_gradient(set.weapon.colors), get_font(set.weapon.font));
		}

		const auto& flags = set.flags.value;
		bounding_box_t::text_array_t text_arr{};

		auto flags_color = set.flags.colors[0].get().modify_alpha(box.m_alpha);

		if (flags.get() != 0) {
			if (entry.m_friend_cheat == 1)
				text_arr.emplace_back(STR("Weave"), color_t{ 254, 103, 49 }.modify_alpha(box.m_alpha));
			else if (entry.m_friend_cheat == 2)
				text_arr.emplace_back(STR("Airflow"), color_t{ 150, 113, 220 }.modify_alpha(box.m_alpha));
			else if (entry.m_friend_cheat == 3)
				text_arr.emplace_back(STR("Boss"), color_t{ 0, 255, 163 }.modify_alpha(box.m_alpha));
			else if (entry.m_friend_cheat == 4)
				text_arr.emplace_back(STR("Furcore"), color_t{ 115, 180, 255 }.modify_alpha(box.m_alpha));
		}

		if (flags.at(0)) {
			if (entry.m_has_armor && entry.m_has_helmet)
				text_arr.emplace_back(STR("HK"), flags_color);
			else if (entry.m_has_armor)
				text_arr.emplace_back(STR("K"), flags_color);
			else if (entry.m_has_helmet)
				text_arr.emplace_back(STR("H"), flags_color);
		}

		if (flags.at(1) && entry.m_scoped)
			text_arr.emplace_back(STR("Zoom"), flags_color);

		if (flags.at(2) && entry.m_flashed)
			text_arr.emplace_back(STR("Flash"), flags_color);

		if (flags.at(3) && entry.m_has_kit)
			text_arr.emplace_back(STR("Kit"), flags_color);

		if (flags.at(4) && entry.m_distance > 0.f)
			text_arr.emplace_back(dformat(STR("{}ft"), (int)std::round((entry.m_distance / 100.f * 3.28084f) / 5) * 5), flags_color);

		if (flags.at(5) && entry.m_ping > 0)
			text_arr.emplace_back(dformat(STR("{}ms"), entry.m_ping), flags_color);

		box.text_array(std::move(text_arr), (bounding_box_t::e_pos_type)set.flags.position, get_font(set.flags.font));
	}

	STFI void out_of_fov(const player_entry_t& entry) {
		SET_AND_RESTORE(render::draw_list->Flags, ImDrawListFlags_AntiAliasedLines | ImDrawListFlags_AntiAliasedFill);

		const auto pos_diff = engine_prediction->m_unpredicted_data.m_origin - entry.m_render_origin;
		const auto local_yaw = DEG2RAD(interfaces::engine->get_view_angles().y);

		constexpr float box_size = 30, box_ratio = 1.f;
		constexpr int arc_size = 13;
		static_assert(arc_size % 2 != 0);

		const vec2d center = { render::screen_width * 0.5f, render::screen_height * 0.5f };

		const float pixel_x = 1.f / render::screen_width;
		const float pixel_y = 1.f / render::screen_height;

		vec2d last{}, last2{};

		constexpr float div = arc_size * 0.5f + 0.5f;
		static const auto calculate_alpha = [arc_size](int i) constexpr {
			const float ratio = (float)i / (arc_size - 1);
			if (ratio <= 0.5f)
				return ratio;

			return 1.f - ratio;
		};

		for (int i = 0; i < arc_size; ++i) {
			const auto ang = i - div;

			vec2d m = {
				std::cos(local_yaw + DEG2RAD(ang)) * pos_diff.y - std::sin(local_yaw + DEG2RAD(ang)) * pos_diff.x,
				std::cos(local_yaw + DEG2RAD(ang)) * pos_diff.x + std::sin(local_yaw + DEG2RAD(ang)) * pos_diff.y
			};

			if (const auto len = m.length(); len != 0.0f) {
				m.x = m.x / len * center.x;
				m.y = m.y / len * center.y;
			}

			const vec2d pos = {
				center.x + m.x * (0.7f + pixel_x * dpi::scale(50.f)),
				center.y + m.y * (0.5f + pixel_y * dpi::scale(50.f))
			};

			const vec2d pos2 = {
				center.x + m.x * (0.7f),
				center.y + m.y * (0.5f)
			};

			const auto clr = color_t{ 255, 255, 255, 255 }.modify_alpha(entry.m_visual_alpha);

			if (!last.is_zero() && !last2.is_zero()) {
				render::draw_list->AddQuadFilledMultiColor(
						{ last.x, last.y }, { pos.x, pos.y },
						{ pos2.x, pos2.y }, { last2.x, last2.y },
						clr.modify_alpha(calculate_alpha(i - 1)).abgr(),
						clr.modify_alpha(calculate_alpha(i)).abgr(),
						clr.modify_alpha(0.f).abgr(),
						clr.modify_alpha(0.f).abgr());

				render::draw_list->AddLine({ last.x, last.y }, { pos.x, pos.y }, clr.new_alpha((int)(255 * entry.m_visual_alpha)).abgr(), dpi::scale(1.f));
			}

			last = pos;
			last2 = pos2;
		}

		bounding_box_t box{};
		box.m_got = true;

		vec2d mult = {
			std::cos(local_yaw) * pos_diff.y - std::sin(local_yaw) * pos_diff.x,
			std::cos(local_yaw) * pos_diff.x + std::sin(local_yaw) * pos_diff.y
		};

		if (const auto len = mult.length(); len != 0.0f) {
			mult.x = mult.x / len;
			mult.y = mult.y / len;
		}

		vec2d box_position = {
			center.x + mult.x * center.x * (0.7f - pixel_x * dpi::scale(60.f)),
			center.y + mult.y * center.y * (0.5f - pixel_y * dpi::scale(120.f))
		};

		box.x = box_position.x - box_size / 2.f;
		box.y = box_position.y - box_size / 2.f * box_ratio;
		box.w = box_size;
		box.h = box_size * box_ratio;
		box.m_alpha = entry.m_visual_alpha;

		render_box(entry, box);
	}

	void players_t::render() {
		if (!settings->player_esp.enable)
			return;

		THREAD_SAFE(entities->m_mutex);

		for (auto& [player, entry]: entities->m_players) {
			if (!entry.m_valid_for_esp)
				continue;

			entry.m_box.reset_render_state();
			entry.m_box.m_alpha = entry.m_visual_alpha;

			if (entry.m_visual_alpha <= 0.001f)
				continue;

			if (entry.m_box.out_of_screen())
				out_of_fov(entry);

			if (!entry.m_box.m_got)
				continue;

			entry.m_box.reset_render_state();
			render_box(entry, entry.m_box);
		};
	}
} // namespace esp