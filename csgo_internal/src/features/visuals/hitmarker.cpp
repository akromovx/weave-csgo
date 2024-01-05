#include "hitmarker.hpp"

#include "../../interfaces.hpp"
#include "../../globals.hpp"
#include "../../render.hpp"

#include "../../../deps/weave-gui/include.hpp"

constexpr auto showtime = 4.f;
constexpr auto animtime = 0.25f;

STFI void render_mark(vec2d pos, color_t color, float size, float gap = 2.f) {
	render::line(pos.x - gap, pos.y - gap, pos.x - gap - size, pos.y - gap - size, color, 2.f);
	render::line(pos.x - gap, pos.y + gap, pos.x - gap - size, pos.y + gap + size, color, 2.f);
	render::line(pos.x + gap, pos.y - gap, pos.x + gap + size, pos.y - gap - size, color, 2.f);
	render::line(pos.x + gap, pos.y + gap, pos.x + gap + size, pos.y + gap + size, color, 2.f);
}

STFI void render_sreen_hitmarker() {
	const auto time_delta = std::abs(interfaces::global_vars->m_realtime - hitmarker->m_last_hit_time);
	if (time_delta < animtime)
		render_mark({ render::screen_width * 0.5f, render::screen_height * 0.5f }, color_t{}.modify_alpha(1.f - time_delta / animtime), 4.f);
}

float hitmarker_t::log_t::calculate_alpha() const {
	const auto time_delta = interfaces::global_vars->m_realtime - m_time;
	if (time_delta < animtime)
		return time_delta / animtime;

	return 1.f - std::clamp((time_delta - showtime - animtime) / animtime, 0.f, 1.f);
}

void hitmarker_t::render() {
	THREAD_SAFE(m_mutex);

	for (auto& hit: m_hits) {
		vec2d origin2d{};
		if (!math::world_to_screen(hit.m_position, origin2d))
			continue;

		if (settings->visuals.hitmarker.at(0))
			render_mark(origin2d, color_t{}.modify_alpha(hit.calculate_alpha()), gui::dpi::scale(4.f), gui::dpi::scale(2.f));

		if (settings->visuals.hitmarker.at(2)) {
			const auto damage_alpha = 1.f - std::clamp((interfaces::global_vars->m_realtime - hit.m_time - 2.f) / animtime, 0.f, 1.f);
			if (damage_alpha > 0.f) {
				const auto color = hit.m_headshot ? color_t{ 255, 124, 124 } : color_t{};
				render::text(
						origin2d.x, origin2d.y - gui::dpi::scale(18.f), color.modify_alpha(damage_alpha),
						render::outline | render::centered_x, fonts::menu_desc, std::format("{}", hit.m_damage));
			}
		}
	}

	m_hits.erase(std::remove_if(m_hits.begin(), m_hits.end(),
								[](const auto& hit) {
									return hit.calculate_alpha() <= 0.f && std::abs(interfaces::global_vars->m_realtime - hit.m_time) > animtime;
								}),
				 m_hits.end());

	if (settings->visuals.hitmarker.at(1))
		render_sreen_hitmarker();
}