#include "bullets.hpp"

#include "../globals.hpp"
#include "../interfaces.hpp"

#include "visuals/hitmarker.hpp"
#include "visuals/logs.hpp"

#include "hvh/autowall.hpp"
#include "hvh/hvh.hpp"

#include "../utils/sounds.hpp"

#include "../../deps/weave-gui/include.hpp"

#include "../../bytes/hitsound/hit1.h"
#include "../../bytes/hitsound/hit2.h"
#include "../../bytes/hitsound/hit3.h"
#include "../../bytes/hitsound/hit4.h"

using namespace sdk;

void bullet_impacts_t::on_weapon_fire(game_event_t* e) {
	const auto userid = e->get_int(STRC("userid"));
	const auto index = interfaces::engine->get_player_for_user_id(userid);

	if (index != interfaces::engine->get_local_player())
		return;

	if (m_shots.empty())
		return;

	shot_record_rage_t* last_unconfirmed = nullptr;

	for (auto it = m_shots.rbegin(); it != m_shots.rend(); it = next(it)) {
		if (!it->m_confirmed) {
			last_unconfirmed = &*it;
			break;
		}

		it->m_skip = true;
	}

	if (last_unconfirmed == nullptr)
		return;

	last_unconfirmed->m_confirmed = true;
}

void bullet_impacts_t::on_bullet_impact(game_event_t* e) {
	const auto userid = e->get_int(STRC("userid"));
	const auto index = interfaces::engine->get_player_for_user_id(userid);

	if (index != interfaces::engine->get_local_player())
		return;

	if (m_shots.empty())
		return;

	shot_record_rage_t* last_confirmed = nullptr;

	for (auto it = m_shots.rbegin(); it != m_shots.rend(); it = next(it)) {
		if (it->m_confirmed && !it->m_skip) {
			last_confirmed = &*it;
			break;
		}
	}

	if (last_confirmed == nullptr)
		return;

	auto& impact = last_confirmed->m_impacts.emplace_back();
	impact.x = e->get_float(STRC("x"));
	impact.y = e->get_float(STRC("y"));
	impact.z = e->get_float(STRC("z"));
}

STFI void play_hitsound() {
	static int hit_counter{};
	float volume = std::clamp((float)settings->misc.hitsound_volume * 0.01f, 0.f, 1.f);

	switch (hit_counter++ & 3) {
		case 0: utils::play_sound_from_memory(hitsound_raw_1, sizeof(hitsound_raw_1), volume); break;
		case 1: utils::play_sound_from_memory(hitsound_raw_2, sizeof(hitsound_raw_2), volume); break;
		case 2: utils::play_sound_from_memory(hitsound_raw_3, sizeof(hitsound_raw_3), volume); break;
		case 3: utils::play_sound_from_memory(hitsound_raw_4, sizeof(hitsound_raw_4), volume); break;
	}
}

STFI std::string hitgroup_to_string(e_hitgroup hitgroup) {
	switch (hitgroup) {
		case e_hitgroup::generic: return STR("generic");
		case e_hitgroup::head: return STR("head");
		case e_hitgroup::chest: return STR("chest");
		case e_hitgroup::stomach: return STR("stomach");
		case e_hitgroup::leftarm: return STR("left arm");
		case e_hitgroup::rightarm: return STR("right arm");
		case e_hitgroup::leftleg: return STR("left leg");
		case e_hitgroup::rightleg: return STR("right leg");
		case e_hitgroup::neck: return STR("neck");
		case e_hitgroup::gear: return STR("gear");
		default: return STR("hitgroup_") + std::to_string((int)hitgroup);
	}
}

STFI void print_damage_dealt(std::string name, int damage, int health_left, e_hitgroup hitgroup) {
	game_console->print_colored_id(dformat(STR("$2Hit $3{} $2for $3{}hp $2in $3{} $2({}hp left)\n"), name, damage, hitgroup_to_string(hitgroup), health_left));
	cheat_logs->add(dformat(STR("Hit {} for {}hp in {} ({}hp left)"), name, damage, hitgroup_to_string(hitgroup), health_left), cheat_logs_t::message_t::e_type::hit);
}

STFI void print_damage_received(std::string name, int damage, int health_left, e_hitgroup hitgroup) {
	game_console->print_colored_id(dformat(STR("$2Hurt from $3{} $2for $3{}hp $2in $3{} $2({}hp left)\n"), name, damage, hitgroup_to_string(hitgroup), health_left));
	cheat_logs->add(dformat(STR("Hurt from {} for {}hp in {} ({}hp left)"), name, damage, hitgroup_to_string(hitgroup), health_left), cheat_logs_t::message_t::e_type::info);
}

STFI std::string get_name_by_userid(int user_id) {
	player_info_t player_info{};
	interfaces::engine->get_player_info(user_id, &player_info);
	auto name = std::string{ player_info.m_name };
	ranges::transform(name, name.begin(), [](char c) { return c == '\n' || c == '\r' || c == '\t' ? ' ' : c; });
	return name;
}

void bullet_impacts_t::on_player_hurt(game_event_t* e) {
	const auto attacker = interfaces::engine->get_player_for_user_id(e->get_int(STRC("attacker")));
	const auto user_id = interfaces::engine->get_player_for_user_id(e->get_int(STRC("userid")));

	const auto local_player = interfaces::engine->get_local_player();
	const auto hitgroup = (e_hitgroup)e->get_int(STRC("hitgroup"));
	if (attacker == local_player && user_id != local_player) {
		if (!m_shots.empty()) {
			shot_record_rage_t* last_confirmed = nullptr;

			for (auto it = m_shots.rbegin(); it != m_shots.rend(); it = next(it)) {
				if (it->m_confirmed && !it->m_skip) {
					last_confirmed = &*it;
					break;
				}
			}

			if (last_confirmed != nullptr) {
				const auto userid = e->get_int(STRC("userid"));
				const auto index = interfaces::engine->get_player_for_user_id(userid);

				if (index != last_confirmed->m_lag_record.m_index) {
					for (auto it = m_shots.rbegin(); it != m_shots.rend(); it = next(it)) {
						if (index == it->m_lag_record.m_index) {
							it->m_confirmed = last_confirmed->m_confirmed;
							it->m_skip = last_confirmed->m_skip;
							it->m_impacts = last_confirmed->m_impacts;
							last_confirmed->m_confirmed = false;
							last_confirmed = &*it;
							break;
						}
					}
				}

				if (index == last_confirmed->m_lag_record.m_index) {
					last_confirmed->m_index = index;
					last_confirmed->m_damage = e->get_int(STRC("dmg_health"));
					last_confirmed->m_health = e->get_int(STRC("health"));
					last_confirmed->m_hitgroup = (e_hitgroup)e->get_int(STRC("hitgroup"));

					auto player = last_confirmed->m_lag_record.m_player;
					auto event_player = interfaces::entity_list->get_client_entity(user_id);

					if (hitgroup != e_hitgroup::generic && event_player != nullptr && player->index() == event_player->index()) {
						const auto fired_shot_hitgroup = hvh::hitbox_to_hitgroup(last_confirmed->m_hitbox);
						if (fired_shot_hitgroup != hitgroup && globals->m_weapon != nullptr) {
							hvh::backup_player(player);

							auto end_pos = last_confirmed->get_farthest_impact();
							if (end_pos.has_value()) {
								last_confirmed->m_lag_record.apply(last_confirmed->m_lag_record.m_bones);

								// check if we can penetrate the hitgroup we hit.
								const auto penetration = hvh::autowall->run(last_confirmed->m_eye_position, end_pos.value(), globals->m_weapon, &last_confirmed->m_lag_record);
								if (penetration.m_did_hit && penetration.m_hitgroup == fired_shot_hitgroup)
									game_console->print_colored_id(STR("$2Damage mismatch due to $4animation desync\n"));
								else
									game_console->print_colored_id(STR("$2Damage mismatch due to $3spread\n"));
							}

							hvh::restore_player(player);
						}
					}

				} else
					last_confirmed->m_index = -1;
			}
		}

		if (settings->misc.hitsound)
			play_hitsound();

		if (settings->misc.log_filter.at(2))
			print_damage_dealt(get_name_by_userid(user_id), e->get_int(STRC("dmg_health")), e->get_int(STRC("health")), hitgroup);
	} else if (attacker != local_player && user_id == local_player) {
		if (settings->misc.log_filter.at(1))
			print_damage_received(get_name_by_userid(attacker), e->get_int(STRC("dmg_health")), e->get_int(STRC("health")), hitgroup);
	}
}

void bullet_impacts_t::render() {}

STFI void process_shot(shot_record_rage_t& shot) {
	if (shot.m_damage > 0) {
		const auto hit_pos = shot.get_closest_impact_to(shot.m_position);
		if (hit_pos.has_value())
			hitmarker->add(*hit_pos, shot.m_damage, interfaces::global_vars->m_realtime, shot.m_hitgroup == e_hitgroup::head);
		return;
	}

	if (settings->misc.log_filter.at(3) && !shot.m_lag_record.m_player->is_alive()) {
		cheat_logs->add(STR("Missed shot due to enemy death"), cheat_logs_t::message_t::e_type::miss);
		game_console->print_colored_id(STR("$2Missed shot due to $3enemy death\n"));
		return;
	}

	bool spread_miss = false;

	auto end_pos = shot.get_farthest_impact();
	if (end_pos.has_value()) {
		hvh::backup_player(shot.m_lag_record.m_player);
		const auto hitgroup = hvh::hitbox_to_hitgroup(shot.m_hitbox);
		auto end_pos = shot.get_farthest_impact();

		if (end_pos.has_value()) {
			shot.m_lag_record.apply(shot.m_lag_record.m_bones);
			auto info = hvh::autowall->run(shot.m_eye_position, end_pos.value(), globals->m_weapon, &shot.m_lag_record);
			if (!info.m_did_hit || info.m_hitgroup != hitgroup)
				spread_miss = true;
		}

		hvh::restore_player(shot.m_lag_record.m_player);
	}

	if (spread_miss) {
		if (settings->misc.log_filter.at(3)) {
			cheat_logs->add(STR("Missed shot due to spread"), cheat_logs_t::message_t::e_type::miss);
			game_console->print_colored_id(STR("$2Missed shot due to $3spread\n"));
		}
	} else {
		if (settings->misc.log_filter.at(3)) {
			cheat_logs->add(STR("Missed shot due to animation desync"), cheat_logs_t::message_t::e_type::miss);
			game_console->print_colored_id(STR("$2Missed shot due to $4animation desync\n"));
		}

		const auto& res = shot.m_lag_record.m_resolver;
		const auto rmode = std::clamp((int)(res.m_mode.m_value & (0xFF << e_resolver_mode_offset::base_mode)), 0, (int)e_resolver_mode::rmode_max);
		auto& misses = resolver->m_info[shot.m_lag_record.m_player->index()].m_missed_shots;

		if (rmode != e_resolver_mode::jumping) {
			++misses[rmode];
			++misses[e_resolver_mode::rmode_any];
		}
	}
}

void bullet_impacts_t::on_pre_move() {
	if (m_shots.empty())
		return;

	for (auto it = m_shots.begin(); it != m_shots.end();) {
		if (globals->m_local_alive) {
			if (it->m_confirmed && !it->m_impacts.empty()) {
				process_shot(*it);
				it = m_shots.erase(it);
			} else
				it = next(it);
		} else {
			if (it->m_confirmed && it->m_impacts.empty()) {
				if (settings->misc.log_filter.at(3)) {
					cheat_logs->add(STR("Missed shot due to death"), cheat_logs_t::message_t::e_type::miss);
					game_console->print_colored_id(STR("$2Missed shot due to $3death\n"));
				}
			}

			it = m_shots.erase(it);
		}
	}

	for (auto it = m_shots.begin(); it != m_shots.end();) {
		if (it->m_time + 2.f < interfaces::global_vars->m_curtime || !globals->m_local_alive)
			it = m_shots.erase(it);
		else
			it = next(it);
	}
}

std::optional<vec3d> shot_record_t::get_farthest_impact() const {
	if (m_impacts.empty())
		return std::nullopt;

	float max_distance = -FLT_EPSILON;
	vec3d farthest{};

	for (auto& impact: m_impacts) {
		const auto distance = (m_eye_position - impact).length_sqr();
		if (distance > max_distance) {
			max_distance = distance;
			farthest = impact;
		}
	}

	if (max_distance > 0.f)
		return farthest;

	return std::nullopt;
}

std::optional<vec3d> shot_record_t::get_closest_impact_to(const vec3d& to) const {
	if (m_impacts.empty())
		return std::nullopt;

	float min_distance = FLT_MAX;
	vec3d closest{};

	for (auto& impact: m_impacts) {
		const auto distance = (to - impact).length_sqr();
		if (distance < min_distance) {
			min_distance = distance;
			closest = impact;
		}
	}

	if (min_distance < FLT_MAX)
		return closest;

	return std::nullopt;
}