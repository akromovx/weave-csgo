#include "peek_state.hpp"
#include "../../game/dormant.hpp"
#include "../../game/engine_prediction.hpp"
#include "../../game/override_entity_list.hpp"
#include "../../globals.hpp"
#include "../../interfaces.hpp"
#include "../../utils/threading.hpp"
#include "autowall.hpp"
#include "exploits.hpp"
#include "hvh.hpp"

using namespace sdk;
using namespace hvh;

STFI bool can_hit_floating_point(const vec3d& start, const vec3d& end, cs_player_t* override_player = nullptr, float* out_dist = nullptr) {
	const auto info = autowall->run(start, end, override_player == nullptr ? globals->m_weapon : override_player->active_weapon(), nullptr, override_player, true);
	const auto ret = info.m_did_hit || start.dist_sqr(info.m_end) > start.dist_sqr(end);

	if (out_dist != nullptr)
		*out_dist = start.dist(info.m_end);

	//#ifdef _DEBUG
	//	if (ret)
	//		interfaces::debug_overlay->add_line_overlay(start, end, 255, 0, 0, true, interfaces::global_vars->m_interval_per_tick * 2.f);
	//	else
	//		interfaces::debug_overlay->add_line_overlay(start, end, 255, 255, 255, true, interfaces::global_vars->m_interval_per_tick * 2.f);
	//#endif

	return ret;
}

STFI vec3d get_dormant_origin(player_entry_t* entry) {
	const auto idx = entry->m_player->index();
	const auto& shared_player = players->m_shared[idx];
	vec3d origin = entry->m_render_origin;
	if (entry->m_player->dormant()) {
		if (!shared_player.is_expired())
			origin = vec3d{ (float)shared_player.m_data.m_origin_x, (float)shared_player.m_data.m_origin_y, (float)shared_player.m_data.m_origin_z };
	}

	return origin;
}

STFI vec3d get_eye_position(cs_player_t* player, const vec3d& origin) {
	const auto view_vectors = interfaces::game_rules->get_view_vectors();
	const auto view_offset = vec3d{ 0.f, 0.f, view_vectors->m_duck_view.z + (view_vectors->m_view - view_vectors->m_duck_view).z * (1.f - player->duck_amount()) };
	return origin + view_offset;
}

STFI int calculate_freestand(player_entry_t* entry, const vec3d& eye_position) {
	if (globals->m_airtick > 0)
		return 0;

	const auto trace_length = 30.f;
	const auto target_yaw = math::calculate_angle(globals->m_local->origin(), entry->m_player->origin()).y;

	const auto eyepos_right = aimbot->get_advanced_point(globals->m_local, globals->m_eye_position, target_yaw + 90.f, trace_length),
			   eyepos_left = aimbot->get_advanced_point(globals->m_local, globals->m_eye_position, target_yaw - 90.f, trace_length);

	const auto player_eye_position = get_dormant_origin(entry) + vec3d{ 0.f, 0.f, entry->m_player->view_offset().z /* * 0.5f*/ };

	const auto enemy_eyepos_right = aimbot->get_advanced_point(entry->m_player, player_eye_position, target_yaw + 90.f, trace_length);
	const auto enemy_eyepos_left = aimbot->get_advanced_point(entry->m_player, player_eye_position, target_yaw - 90.f, trace_length);

	const auto damage_right = can_hit_floating_point(eyepos_right, enemy_eyepos_right) || can_hit_floating_point(eyepos_right, enemy_eyepos_left);
	const auto damage_left = can_hit_floating_point(eyepos_left, enemy_eyepos_right) || can_hit_floating_point(eyepos_left, enemy_eyepos_left);

	float dist_right{}, dist_left{};

	const auto center_damage_right = can_hit_floating_point(globals->m_eye_position, enemy_eyepos_right, nullptr, &dist_right);
	const auto center_damage_left = can_hit_floating_point(globals->m_eye_position, enemy_eyepos_left, nullptr, &dist_left);

	if (damage_right && !damage_left && !center_damage_left && dist_left < 150.f)
		return 1;

	if (damage_left && !damage_right && !center_damage_right && dist_right < 150.f)
		return -1;

	return 0;
}

STFI void proceed_player(cs_player_t* player, player_entry_t* entry, const vec3d& eye_position) {
	const auto idx = player->index();
	auto& info = peek_state->m_players[idx];

	if (player->dormant()) {
		if (!players->m_shared[idx].is_expired() && std::abs(interfaces::global_vars->m_curtime - esp::dormant->m_sound_players[idx].m_last_seen_time) <= 8.f)
			info.m_freestand_side = calculate_freestand(entry, eye_position);
		else
			info.m_freestand_side = 0;
	} else
		info.m_freestand_side = calculate_freestand(entry, eye_position);

	// skip scan if we're on almost the same position
	if ((globals->m_eye_position - eye_position).length_sqr() <= 10.f)
		return;

	aimbot->m_eye_position = eye_position;

	if (player->dormant()) {
		if (std::abs(interfaces::global_vars->m_curtime - esp::dormant->m_sound_players[idx].m_last_seen_time) <= 8.f) {
			const auto player_eye_position = get_eye_position(player, get_dormant_origin(entry));
			const float target_yaw = math::calculate_angle(globals->m_local->origin(), player_eye_position).y;
			const auto eyepos_right = aimbot->get_advanced_point(player, player_eye_position, target_yaw + 90.f, 30.f);
			const auto eyepos_left = aimbot->get_advanced_point(player, player_eye_position, target_yaw - 90.f, 30.f);

			if (can_hit_floating_point(globals->m_eye_position, eyepos_right) || can_hit_floating_point(globals->m_eye_position, eyepos_left))
				info.m_has_damage = true;
		}

		return;
	}

	auto record = players->find_record_if(entry, [](lag_record_t* record) { return record->is_valid(); });
	if (record == nullptr)
		return;

	// only scan a few points
	std::vector<aimbot_t::point_t> points{};
	record->apply(record->m_bones);
	aimbot->collect_points(record, record->m_bones, aimbot->get_hitboxes_to_scan(true), points, true);
	aimbot->scan_points(record, points);

	if (ranges::none_of(points, [=](const auto& point) { return point.m_damage > 0; }))
		return;

	info.m_has_damage = true;

	const auto health = player->health();
	if (ranges::none_of(points, [health](const auto& point) { return is_baim_hitbox((e_hitbox)point.m_hitbox) && aimbot->correct_damage(point.m_damage) > health; }))
		return;

	info.m_has_lethal_point = true;
}

STFI int32_t lookup_bone(base_entity_t* entity, const char* name) {
	return patterns::lookup_bone.as<int32_t(__thiscall*)(base_entity_t*, const char*)>()(entity, name);
}

STFI vec3d matrix_get_origin(const matrix3x4_t& src) {
	return { src[0][3], src[1][3], src[2][3] };
}

STFI void calculate_defensive() {
	peek_state->m_defensive_lag = false;

	if (globals->m_local->velocity().length2d() <= 8.f)
		return;

	const auto predicted_origin = predict_local_player_origin(globals->m_local->origin(), globals->m_local->velocity(), globals->m_old_velocity, XOR32(6));
	const auto view_offset = globals->m_local->view_offset();
	const auto local_body_pos = predicted_origin + vec3d{ 0.f, 0.f, view_offset.z * 0.5f };

	const auto local_head_position = aimbot->get_advanced_point(globals->m_local, globals->m_eye_position, engine_prediction->m_last_sent_data.m_cmd.m_viewangles.y, 20.f);
	//std::vector<c_aimbot::point_t> local_points{};

	//math::change_bones_position(players->m_local_player.m_bones, sdk::max_bones, { 0.f, 0.f, 0.f }, globals->m_local->origin());
	//aimbot->collect_points(globals->m_local, players->m_local_player.m_bones, { e_hitbox::head/*, e_hitbox::stomach */}, local_points, true);
	//math::change_bones_position(players->m_local_player.m_bones, sdk::max_bones, globals->m_local->origin(), { 0.f, 0.f, 0.f });

	//c_threading::callbacks_t callbacks{};
	for (auto& [player, entry]: entities->m_players) {
		if (player == globals->m_local || player->is_teammate() || !player->is_alive() || player->gungame_immunity())
			continue;

		vec3d player_eyepos{};

		if (player->dormant()) {
			auto idx = player->index();
			if (players->m_shared[idx].is_expired() || std::abs(interfaces::global_vars->m_curtime - esp::dormant->m_sound_players[idx].m_last_seen_time) > 8.f)
				continue;

			player_eyepos = get_eye_position(player, get_dormant_origin(&entry));
		} else
			player_eyepos = predict_player_origin(player, XOR32(6)) + player->view_offset();

		const float target_yaw = math::calculate_angle(globals->m_local->origin(), player_eyepos).y;

		std::vector<vec3d> points = {
			aimbot->get_advanced_point(globals->m_local, local_body_pos, target_yaw + 90.f, 10.f),
			aimbot->get_advanced_point(globals->m_local, local_body_pos, target_yaw - 90.f, 10.f),
			local_head_position
		};

		const auto player_eyepos_right = aimbot->get_advanced_point(player, player_eyepos, target_yaw + 90.f, 16.f);
		const auto player_eyepos_left = aimbot->get_advanced_point(player, player_eyepos, target_yaw - 90.f, 16.f);

		for (const auto& point: points) {
			if (can_hit_floating_point(player_eyepos_right, point, player) || can_hit_floating_point(player_eyepos_left, point, player)) {
				peek_state->m_defensive_lag = true;
				break;
			}
		}

		if (peek_state->m_defensive_lag)
			break;
	}

	//threading->run(callbacks);
}

STI void calculate_freestand_side() {
	int right = 0, left = 0;
	for (const auto& p: peek_state->m_players) {
		if (p.m_freestand_side > 0)
			++right;
		else if (p.m_freestand_side < 0)
			++left;
	}

	if (right > 0 && left == 0)
		antiaim->m_freestand_side = 1;
	else if (left > 0 && right == 0)
		antiaim->m_freestand_side = -1;
}

void peek_state_t::on_create_move() {
	antiaim->m_freestand_side = 0;
	this->reset();

	if (!globals->m_local->is_alive() || globals->m_weapon == nullptr)
		return;

	if (!aimbot->m_settings.has_value())
		return;

	threading_t::callbacks_t callbacks{};
	std::vector<cs_player_t*> players{};

	const auto extra_ticks = MAX_FAKELAG - get_ticks_to_standing_accuracy() - 1;

	const auto predicted_eye_pos = predict_local_player_origin(globals->m_local->origin(), globals->m_local->velocity(), globals->m_old_velocity, extra_ticks) + globals->m_local->view_offset();

	for (auto& [player, entry]: entities->m_players) {
		if (player == globals->m_local || player->is_teammate() || !player->is_alive() || player->gungame_immunity())
			continue;

		backup_player(player);
		players.emplace_back(player);

		auto pentry = &entry;
		callbacks.emplace_back([=]() { proceed_player(player, pentry, predicted_eye_pos); });
	}

	threading->run(callbacks);

	calculate_freestand_side();
	calculate_defensive();

	for (auto player: players)
		restore_player(player);
}

void peek_state_t::on_post_move() {
	for (auto& p: m_players)
		p.m_has_damage_old = p.m_has_damage;
}