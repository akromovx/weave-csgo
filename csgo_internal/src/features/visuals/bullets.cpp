#include "../bullets.hpp"
#include "../../globals.hpp"
#include "../../interfaces.hpp"
#include "../../render.hpp"
#include "../../utils/easings.hpp"

#include "../../../deps/imgui/imgui.h"

using namespace sdk;

static constexpr float impact_animation = 0.4f;

void bullet_tracer_t::on_bullet_impact(game_event_t* e) {
	const auto player = (cs_player_t*)interfaces::entity_list->get_client_entity(interfaces::engine->get_player_for_user_id(e->get_int(STRC("userid"))));
	if (player == globals->m_local) {
		m_last_impact_recieved = { e->get_float(STRC("x")), e->get_float(STRC("y")), e->get_float(STRC("z")) };

		auto& impact = m_server_impacts.emplace_back();
		impact.m_position = *m_last_impact_recieved;
		impact.m_time = interfaces::global_vars->m_realtime;
		impact.m_valid = true;

		if (m_local_tracers.empty())
			return;

		auto& tracer = m_local_tracers.front();
		tracer.m_weapon_fired = true;
		tracer.m_impacts.emplace_back(*m_last_impact_recieved);
	}
	//else if (!player->is_teammate()) {
	//	auto& tracer = m_enemy_tracers.front();
	//	tracer.m_weapon_fired = true;
	//	tracer.m_impacts.emplace_back(e->get_float(STRC("x")), e->get_float(STRC("y")), e->get_float(STRC("z")));
	//}
}

void bullet_tracer_t::on_weapon_fire(game_event_t* e) {
	if (m_local_tracers.empty())
		return;

	auto& tracer = m_local_tracers.front();
	tracer.m_weapon_fired = true;
}

void bullet_tracer_t::store_client_impacts() {
	struct client_impact_t {
		vec3d m_position{};
		float m_time{};
		float m_expires{};
	};

	auto& impacts_list = *(utils::utl_vector_t<client_impact_t>*)((uintptr_t)globals->m_local + XOR32(0x11C50));
	for (auto i = impacts_list.count(); i > 0; i--) {
		auto& impact = m_client_impacts.emplace_back();
		impact.m_position = impacts_list[i - 1].m_position;
		impact.m_time = interfaces::global_vars->m_realtime;
		impact.m_valid = true;
	}

	impacts_list.remove_all();
}

void bullet_tracer_t::erase_outdated_impacts(std::vector<impact_info_t>& list, float impact_time) {
	if (list.empty())
		return;

	list.erase(std::remove_if(list.begin(), list.end(),
							  [impact_time](impact_info_t& impact) {
								  const float time_delta = std::abs(interfaces::global_vars->m_realtime - impact.m_time);
								  if (time_delta > impact_time + impact_animation)
									  impact.m_valid = false;

								  return !impact.m_valid;
							  }),
			   list.end());
}

void bullet_tracer_t::draw_tracers(std::vector<shot_record_t>& tracers, color_t color, int type, float duration) {
	if (type == 0) {
		if (!tracers.empty()) tracers.clear();
		return;
	}

	if (tracers.empty())
		return;

	for (const auto& trace: tracers) {
		auto end = trace.get_farthest_impact();
		if (!end.has_value())
			continue;

		if (type == 1) {
			interfaces::debug_overlay->add_line_overlay(trace.m_eye_position, end.value(), color.r(), color.g(), color.b(), true, 4.f);
		} else if (type == 2) {

			beam_info_t beam_info{};
			beam_info.m_type = beam_normal;
			beam_info.m_model_index = -1;
			beam_info.m_start = trace.m_eye_position;
			beam_info.m_end = *end;
			beam_info.m_life = 4.f;
			beam_info.m_fade_lenght = .1f;
			beam_info.m_halo_scale = 0.f;
			beam_info.m_amplitude = 1.f;
			beam_info.m_segments = 2;
			beam_info.m_renderable = true;
			beam_info.m_brightness = 255.f;
			beam_info.m_red = color.r();
			beam_info.m_green = color.g();
			beam_info.m_blue = color.b();
			beam_info.m_speed = 1.f;
			beam_info.m_start_frame = 0;
			beam_info.m_frame_rate = 0.f;
			beam_info.m_width = 2.f;
			beam_info.m_end_width = 2.f;
			beam_info.m_flags = beam_only_no_is_once | beam_notile;

			auto model_name = STRS("sprites/purplelaser1.vmt");
			beam_info.m_model_name = model_name.c_str();

			auto beam = interfaces::beams->create_beam_points(beam_info);
			if (beam != nullptr)
				interfaces::beams->draw_beam(beam);
		}
		//else if (type == 3) {
		//	const auto thickness = 0.3f;

		//	vec3d trajectory{};
		//	vec3d delta_pos(*end - trace.m_eye_position);
		//	math::vector_angles(delta_pos, trajectory);

		//	interfaces::glow_manager->add_glow_box(
		//		trace.m_eye_position, trajectory,
		//		vec3d{ 0.f, -thickness, -thickness }, vec3d{ (trace.m_eye_position - *end).length(), thickness, thickness },
		//		color, interfaces::global_vars->m_curtime, 4.f
		//	);
		//}
	}

	tracers.erase(std::remove_if(tracers.begin(), tracers.end(), [](shot_record_t& record) {
					  return record.get_farthest_impact().has_value() || std::abs(record.m_time - interfaces::global_vars->m_curtime) > 0.5f;
				  }),
				  tracers.end());
}

void bullet_tracer_t::draw_tracers() {
	draw_tracers(m_local_tracers, settings->bullets.tracer_color.get(), settings->bullets.tracer);
}

void bullet_tracer_t::on_render_start() {
	if (globals->m_local == nullptr)
		return this->clear();

	store_client_impacts();
	erase_outdated_impacts(m_client_impacts);
	erase_outdated_impacts(m_server_impacts);

	draw_tracers();
}

float bullet_tracer_t::impact_info_t::calculate_alpha(float impact_time) const {
	const float time_delta = std::abs(interfaces::global_vars->m_realtime - m_time);
	float alpha = 1.f;
	if (time_delta > impact_time)
		alpha = 1.f - (time_delta - impact_time) / impact_animation;

	return std::clamp(alpha, 0.f, 1.f);
}

void bullet_tracer_t::draw_impacts(const std::vector<impact_info_t>& list, c_imgui_color colors[2]) {
	for (const auto& impact: list) {
		const auto alpha = impact.calculate_alpha();
		const auto size_lerp = ((float)settings->bullets.impacts_size * 0.1f) * easings::out_quad(alpha);
		render::filled_cube(impact.m_position, size_lerp, colors[0].get().modify_alpha(alpha));
		render::cube(impact.m_position, size_lerp, colors[1].get().modify_alpha(alpha), 1.5f);
	}
}

void bullet_tracer_t::render() {
	if (globals->m_local == nullptr)
		return this->clear();

	SET_AND_RESTORE(render::draw_list->Flags, ImDrawListFlags_None);

	if (settings->bullets.server_impacts)
		draw_impacts(m_server_impacts, settings->bullets.server_impact_colors);

	if (settings->bullets.client_impacts)
		draw_impacts(m_client_impacts, settings->bullets.client_impact_colors);
}