#include "local_esp.hpp"

#include "../../globals.hpp"
#include "../../interfaces.hpp"
#include "../../render.hpp"

#include "../../utils/hotkeys.hpp"
#include "../../utils/easings.hpp"
#include "../../utils/animation_handler.hpp"

#include "../../game/players.hpp"

#include "../../features/movement.hpp"
#include "../../features/hvh/hvh.hpp"

namespace esp {
	STFI void render_scope() {
		static float scope_animation = 0.f;
		if (!settings->visuals.removals.at(3) || !globals->m_local_alive || !globals->m_is_connected) {
			animations_direct->lerp(scope_animation, 0.f);
			return;
		}

		bool scoped = globals->m_local_alive && globals->m_zoom_level > 0;
		animations_direct->lerp(scope_animation, scoped ? 1.f : 0.f);

		if (settings->visuals.override_scope == 1) {
			if (scoped) {
				const vec2d screen_size = { render::screen_width, render::screen_height };
				render::line(0.f, screen_size.y * 0.5f, screen_size.x, screen_size.y * 0.5f, { 0, 0, 0, 255 });
				render::line(screen_size.x * 0.5f, 0.f, screen_size.x * 0.5f, screen_size.y, { 0, 0, 0, 255 });
			}
		} else {
			if (scope_animation > 0.f) {
				switch (settings->visuals.override_scope) {
					case 2: {
						const auto arrow_size = settings->visuals.scope_size * (scoped ? easings::out_circ(scope_animation) : 1.f - easings::out_circ(1.f - scope_animation));
						const auto offset = settings->visuals.scope_thickness / 2 + 1 + settings->visuals.scope_gap;

						const auto screen_center = vec2d{ render::screen_width / 2.f, render::screen_height / 2.f };
						const auto arrow_thickness = settings->visuals.scope_thickness / 2.f;
						const auto color_in = settings->visuals.scope_color[0].get().abgr(), color_out = settings->visuals.scope_color[1].get().abgr();

						render::draw_list->AddRectFilledMultiColor(
								{ screen_center.x - offset - arrow_size, screen_center.y - arrow_thickness },
								{ screen_center.x - offset, screen_center.y + arrow_thickness }, color_out, color_in, color_in, color_out);

						render::draw_list->AddRectFilledMultiColor(
								{ screen_center.x + offset + arrow_size, screen_center.y - arrow_thickness },
								{ screen_center.x + offset, screen_center.y + arrow_thickness }, color_out, color_in, color_in, color_out);

						render::draw_list->AddRectFilledMultiColor(
								{ screen_center.x - arrow_thickness, screen_center.y - arrow_size - offset },
								{ screen_center.x + arrow_thickness, screen_center.y - offset }, color_out, color_out, color_in, color_in);

						render::draw_list->AddRectFilledMultiColor(
								{ screen_center.x - arrow_thickness, screen_center.y + arrow_size + offset },
								{ screen_center.x + arrow_thickness, screen_center.y + offset }, color_out, color_out, color_in, color_in);
					} break;
				}
			}
		}
	}

	STFI void render_peek_assist() {
		auto& animation = movement->m_peek_state.m_animation;

		if (!hotkeys->peek_assist.is_valid() || !hotkeys->peek_assist.m_active || !globals->m_local_alive || !globals->m_is_connected) {
			animations_direct->lerp(animation, 0.f);
			return;
		}

		const auto& source = movement->m_peek_state.m_source;
		if (source != vec3d{ 0, 0, 0 }) {
			constexpr int points_count = 60;
			std::vector<ImVec2> points;
			vec2d center2d{};
			if (math::world_to_screen(source, center2d)) {
				constexpr float step = f2PI / (float)(points_count);
				for (float angle = 0.f; angle <= f2PI; angle += step) {
					auto point3d = vec3d{ sin(angle), cos(angle), 0.f } * 17.5f * easings::out_quint(animation);
					vec2d point2d{};
					if (math::world_to_screen(source + point3d, point2d))
						points.emplace_back(point2d.x, point2d.y);
				}

				auto base_color = movement->m_peek_state.m_going_back ? settings->movement.peek_assist_colors[1] : settings->movement.peek_assist_colors[0];
				if (!points.empty()) {
					for (size_t i = 0; i < points.size() - 1; ++i) {
						render::triangle_filled_multicolor(
								points[i].x, points[i].y,
								points[i + 1].x, points[i + 1].y,
								center2d.x, center2d.y,
								base_color.get().new_alpha(0), base_color.get().new_alpha(0), base_color.get().new_alpha((int)(80.f * animation)));
					}
				}
			}

			animations_direct->lerp(animation, 1.f);
		} else
			animation = 0.f;
	}

	static void draw_angle_line(vec3d origin, float angle, color_t color) {
		vec3d dst, forward;
		vec2d sc1, sc2;

		math::angle_vectors(vec3d(0, angle, 0), forward);
		if (math::world_to_screen(origin, sc1) && math::world_to_screen(origin + (forward * 28), sc2))
			render::line(sc1.x, sc1.y, sc2.x, sc2.y, color);
	}

	void local_t::render() {
		render_peek_assist();
		render_scope();
	}
} // namespace esp