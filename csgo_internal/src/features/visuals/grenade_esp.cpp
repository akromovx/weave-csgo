#include "grenade_esp.hpp"

#include "../../game/engine_prediction.hpp"
#include "../../game/override_entity_list.hpp"

#include "../../globals.hpp"
#include "../../interfaces.hpp"

#include "../../utils/animation_handler.hpp"
#include "../../utils/easings.hpp"

namespace dpi = gui::dpi;
using namespace sdk;

namespace esp {
	STFI void render_projectile(projectile_entry_t& entry, vec2d pos, float size, float pred_detonate_ratio, font_t font, bool mini) {
		const auto damage_warning = easings::out_quad((float)entry.m_predicted_damage / globals->m_local_hp);
		const auto predicted_damage = entry.m_type == projectile_entry_t::e_type::hegrenade ? entry.m_predicted_damage : 0;
		const auto distance_ratio = 1.f - std::clamp((entry.m_distance_to_local - 800.f) / 60.f, 0.f, 1.f);
		const auto alpha = entry.m_thrown_by_local ? 1.f : distance_ratio;
		const auto text_font = mini ? fonts::esp_default : (damage_warning > 0.5f ? fonts::menu_big : fonts::esp_default);
		const auto font_icon = mini ? fonts::weapon_icons : (damage_warning > 0.5f ? fonts::weapon_icons : fonts::weapon_icons_big);

		const auto damage_str = dformat(STR("-{}"), predicted_damage);
		const auto should_show_damage = entry.m_type == projectile_entry_t::e_type::hegrenade && predicted_damage > 0;

		if (should_show_damage)
			animations_direct->lerp(entry.m_he_switch_animation, 1.f, 8.f);
		else
			animations_direct->lerp(entry.m_he_switch_animation, 0.f, 4.f);

		const auto damage_size = render::calc_text_size(damage_str, text_font) * entry.m_he_switch_animation;
		const auto icon_size = render::calc_text_size(entry.m_icon, font_icon);

		render::circle_filled(pos.x, pos.y, size, { 35 + int(damage_warning * 200.f), 35, 35, (uint8_t)(255 * 0.3f * alpha) }, 32);
		render::draw_list->PathArcTo({ pos.x, pos.y }, size, 0.f, (1.f - pred_detonate_ratio) * (float)PI * 2.f, 32);
		render::draw_list->PathStroke(color_t{}.modify_alpha(alpha).abgr(), false, 2);

		render::text(pos.x, pos.y - icon_size.y * 0.5f - damage_size.y * 0.5f, color_t{}.modify_alpha(alpha), render::centered_x, font_icon, entry.m_icon);
		if (damage_size.length_sqr() > 0.f)
			render::text(pos.x, pos.y + icon_size.y * 0.1f, color_t{}.modify_alpha(alpha * entry.m_he_switch_animation), render::centered_x, text_font, damage_str);
	}

	//static int AddGlowBox(vec3d vecOrigin, vec3d angOrientation, vec3d mins, vec3d maxs, color_t colColor, float flLifetime) {
	//	static auto addGlowBox = utils::find_pattern("client.dll", "55 8B EC 53 56 8D 59").as<int(__thiscall*)(sdk::glow_object_manager_t*, vec3d, vec3d, vec3d, vec3d, color_t, float)>();
	//	return addGlowBox(interfaces::glow_manager, vecOrigin, angOrientation, mins, maxs, colColor, flLifetime);
	//}

	STFI void render_grenade_prediction() {
		if (globals->m_local_alive) {
			THREAD_SAFE(::projectiles->m_mutex);

			auto& path = ::projectiles->m_predicted_path;
			if (!path.empty()) {
				std::vector<ImVec2> points{};
				for (size_t i = 1; i < path.size(); ++i) {
					vec2d screen1{}, screen2{};
					if (math::world_to_screen(path[i - 1], screen1) && math::world_to_screen(path[i], screen2)) {
						points.emplace_back(screen1.x, screen1.y);
						points.emplace_back(screen2.x, screen2.y);
					}

					//vec3d start = path[i - 1];
					//vec3d end = path[i];

					//vec3d balls{};
					//math::vector_angles(start - end, balls);

					//AddGlowBox(end, balls, vec3d(0, -0.5, -0.5),
					//		   vec3d((start - end).length(), 0.5, 0.5), color_t{}, interfaces::global_vars->m_frametime);
				}

				render::draw_list->AddPolyline(points.data(), points.size(), color_t{}.abgr(), ImDrawFlags_::ImDrawFlags_None, 1.5f);
			}
		}
	}

	STFI void render_world_grenades() {
		constexpr int arc_size = 18;
		static_assert(arc_size % 2 == 0);

		if (!settings->grenade_esp.enable)
			return;

		THREAD_SAFE(entities->m_mutex);
		SET_AND_RESTORE(render::draw_list->Flags, ImDrawListFlags_AntiAliasedLines | ImDrawListFlags_AntiAliasedFill);

		for (auto& [projectile, entry]: entities->m_projectiles) {
			if (entry.m_predicted_path.size() <= 1 || entry.m_icon.empty())
				continue;

			vec2d origin2d{};
			const auto w2s = math::world_to_screen(entry.m_predicted_path.back(), origin2d);
			const auto pred_detonate_ratio = std::clamp(
					(entry.m_detonate_time - interfaces::global_vars->m_curtime) / (entry.m_detonate_time - entry.m_spawn_time), 0.f, 1.f);

			if (!entry.m_exploded && w2s) {
				render_projectile(entry, origin2d, dpi::scale(28.f), pred_detonate_ratio, fonts::weapon_icons_big, false);
			}

			const auto exploded = entry.m_type != projectile_entry_t::e_type::smokegrenade && entry.m_exploded;

			if (!is_on_screen(origin2d) && !exploded) {
				const auto distance_ratio = 1.f - std::clamp((entry.m_distance_to_local - 800.f) / 60.f, 0.f, 1.f);
				const auto alpha = entry.m_thrown_by_local ? 1.f : distance_ratio;
				SET_AND_RESTORE(render::draw_list->Flags, ImDrawListFlags_AntiAliasedLines | ImDrawListFlags_AntiAliasedFill);

				const auto pos_diff = engine_prediction->m_unpredicted_data.m_origin - entry.m_predicted_path.back();
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

					color_t clr = { 255, 255, 255, 255 };
					if (entry.m_type == projectile_entry_t::e_type::hegrenade && globals->m_local_hp > 0)
						clr = color_t::lerp({ 255, 255, 255 }, { 255, 0, 0 }, easings::out_quad((float)entry.m_predicted_damage / globals->m_local_hp));

					if (entry.m_type == projectile_entry_t::e_type::fire)
						clr = { 255, 0, 0, 255 };

					clr = clr.modify_alpha(alpha);

					if (!last.is_zero() && !last2.is_zero()) {
						render::draw_list->AddQuadFilledMultiColor(
								{ last.x, last.y }, { pos.x, pos.y },
								{ pos2.x, pos2.y }, { last2.x, last2.y },
								clr.modify_alpha(calculate_alpha(i - 1)).abgr(),
								clr.modify_alpha(calculate_alpha(i)).abgr(),
								clr.modify_alpha(0.f).abgr(),
								clr.modify_alpha(0.f).abgr());
					}

					last = pos;
					last2 = pos2;
				}

				vec2d mult = {
					std::cos(local_yaw) * pos_diff.y - std::sin(local_yaw) * pos_diff.x,
					std::cos(local_yaw) * pos_diff.x + std::sin(local_yaw) * pos_diff.y
				};

				if (const auto len = mult.length(); len != 0.0f) {
					mult.x = mult.x / len;
					mult.y = mult.y / len;
				}

				vec2d base_position = {
					center.x + mult.x * center.x * (0.7f - pixel_x * dpi::scale(60.f)),
					center.y + mult.y * center.y * (0.5f - pixel_y * dpi::scale(60.f))
				};

				render_projectile(entry, base_position, dpi::scale(18.f), pred_detonate_ratio, fonts::weapon_icons, true);
			}
		}
	}

	static auto world_circle = [=](vec3d location, float radius) {
		constexpr float step = PI * 2.0f / 60;
		std::vector<ImVec2> points;
		for (float lat = 0.f; lat <= PI * 2.0f; lat += step) {
			const auto point3d = vec3d{ sin(lat), cos(lat), 0.f } * radius;
			vec2d point2d{};
			if (math::world_to_screen(location + point3d, point2d))
				points.emplace_back(point2d.x, point2d.y);
		}

		render::draw_list->AddConvexPolyFilled(points.data(), points.size(), color_t{ 255, 161, 102, 80 }.abgr());
		render::draw_list->AddPolyline(points.data(), points.size(), color_t{ 255, 161, 102, 50 }.abgr(), ImDrawFlags_None, 2.f);
	};

	STFI void render_infernos() {
		THREAD_SAFE(entities->m_mutex);

		for (auto& [inferno, entry]: entities->m_infernos) {
			vec2d origin2d{};

			if (!entry.m_visual_origin.has_value() && !entry.m_origin.is_zero())
				entry.m_visual_origin = entry.m_origin;

			if (!entry.m_visual_origin.has_value())
				continue;

			animations_direct->ease_lerp(entry.m_visual_origin->x, entry.m_origin.x, 10.f);
			animations_direct->ease_lerp(entry.m_visual_origin->y, entry.m_origin.y, 10.f);
			animations_direct->ease_lerp(entry.m_visual_origin->z, entry.m_origin.z, 10.f);

			animations_direct->ease_lerp(entry.m_visual_range, entry.m_range, 10.f);

			if (!math::world_to_screen(entry.m_origin, origin2d))
				continue;

			world_circle(*entry.m_visual_origin, entry.m_visual_range);
		}
	}

	void projectiles_t::render() {
		render_infernos();
		render_world_grenades();
		render_grenade_prediction();
	}
} // namespace esp