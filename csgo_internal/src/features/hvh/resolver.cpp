#include "../../game/players.hpp"
#include "../../game/override_entity_list.hpp"
#include "../../globals.hpp"
#include "../../interfaces.hpp"

using namespace sdk;

float resolver_t::get_angle(cs_player_t* player) {
	return math::normalize_yaw(player->eye_angles().y);
}

float resolver_t::get_forward_yaw(cs_player_t* player) {
	return math::normalize_yaw(get_backward_yaw(player) - 180.f);
}

float resolver_t::get_away_angle(cs_player_t* player) {
	return math::calculate_angle(globals->m_local->eye_position(), player->eye_position()).y;
}

float resolver_t::get_backward_yaw(cs_player_t* player) {
	return math::calculate_angle(globals->m_local->origin(), player->origin()).y;
}

float resolver_t::get_left_yaw(cs_player_t* player) {
	return math::normalize_yaw(math::calculate_angle(globals->m_local->origin(), player->origin()).y - 90.f);
}

float resolver_t::get_right_yaw(cs_player_t* player) {
	return math::normalize_yaw(math::calculate_angle(globals->m_local->origin(), player->origin()).y + 90.f);
}

void resolver_t::on_create_move() {
	if (!globals->m_local_alive || !settings->ragebot.enable) {
		for (int i = 0; i < 64; ++i) {
			m_info[i].reset();
		}
		return;
	}

	static int old_side[65] = {};

	for (auto& [player, entry]: entities->m_players) {
		if (!player->is_alive() || player->dormant() || !player->is_player() || player->is_teammate() || player == globals->m_local) {
			continue;
		}

		int index = player->index();

		//if (csgo->actual_misses[player->index()] <= 0)
		{
			//auto pLatestAnimation = g_Animfix->GetLatestAnimation(player);
			//if (!pLatestAnimation)
			//	continue;

			//auto vecPoints = Ragebot::GetAdvancedHeadPoints(player, pLatestAnimation->m_arrZeroSideBones);

			//Ragebot::BackupPlayer(player);
			//Ragebot::SetRecord(pLatestAnimation, pLatestAnimation->m_arrZeroSideBones);

			//float flLeftDamage = g_AutoWall->Think(vecPoints[0], player, HITGROUP_HEAD).m_damage;
			//float flRightDamage = g_AutoWall->Think(vecPoints[1], player, HITGROUP_HEAD).m_damage;

			//Ragebot::RestorePlayer(player);

			//if (flLeftDamage > flRightDamage)
			//	m_freestand_side[index] = 1;
			//else if (flLeftDamage < flRightDamage)
			//	m_freestand_side[index] = -1;
			//else

			m_freestand_side[index] = old_side[index] == 0 ? (rand() % 2 != 0 ? 1 : -1) : old_side[index];
		}
	}

	std::memcpy(old_side, m_freestand_side, sizeof(int) * 64);
}

// big thanks to @KIRBI512 for help me developing this shit
bool resolver_t::can_use_animations(resolver_context_t& ctx) {
	if (ctx.m_record->m_previous == nullptr)
		return false;

	auto speed = ctx.m_record->m_velocity.length2d();
	const auto last_speed = ctx.m_record->m_previous->m_velocity.length2d();
	auto& info = m_info[ctx.m_index];

	// zero angle layer rate always has the same value
	// but when enemy is playing with desync - his rate is higher than simulated one
	// we should get difference between them and get the precentage of desync amount

	// IMPORTANT: simulated layers give results that twice higher than original layer value
	const auto original_rate = ctx.m_record->m_layers[6].m_playback_rate;
	const auto zero_rate = ctx.m_record->m_zero_layers[6].m_playback_rate;
	const auto left_delta = std::abs(ctx.m_record->m_left_layers[6].m_playback_rate - original_rate);
	const auto right_delta = std::abs(ctx.m_record->m_right_layers[6].m_playback_rate - original_rate);
	const auto both_delta = std::abs(left_delta - right_delta);
	const auto misses = ctx.m_info->m_missed_shots[e_resolver_mode::animation_layers];

	auto got_side = false;
	if (speed > 10.f) {
		if (speed - last_speed <= -10.f) {
			got_side = true;
			ctx.m_info->m_mode.set(e_resolver_mode::animation_layers, 6);
		} else {

			if (!(left_delta > 0.004f && right_delta > 0.004f)) {
				// m_flPlaybackRate - animation speed
				// in fact side where that value is higher than others could be real side

				// so let's choose it by the highest value
				if (right_delta > left_delta) {
					info.m_last_side = 1;
					got_side = true;
				} else if (right_delta < left_delta) {
					info.m_last_side = -1;
					got_side = true;
				}

				if (got_side)
					ctx.m_info->m_mode.set(e_resolver_mode::animation_layers, 1);

				if (std::abs(zero_rate - original_rate) >= 0.0015f) {
					info.m_last_side = 0;
					got_side = true;
					ctx.m_info->m_mode.set(e_resolver_mode::animation_layers, 2);
				}

				if (got_side) {
					if (both_delta > 0.001f && info.m_last_side != 0) {
						info.m_last_side *= -1;
						ctx.m_info->m_mode.set(e_resolver_mode::animation_layers, 3);
					}
				}
			}
		}
	} else
		return false;

	if (got_side && misses <= 2) {
		info.m_side = info.m_last_side;
		return true;
	} else {
		if (misses > 0) {
			if (info.m_last_side == 0)
				return false;

			ctx.m_info->m_mode.set(e_resolver_mode::animation_layers, 4);
			info.m_side = info.m_last_side;
			switch (misses % 2) {
				case 0:
					info.m_side = info.m_last_side;
					break;
				case 1:
					info.m_side = -info.m_last_side;
					break;
			}
		} else {
			ctx.m_info->m_mode.set(e_resolver_mode::animation_layers, 5);
			info.m_side = info.m_last_side;
		}
		return true;
	}

	return true;
}

bool resolver_t::static_jitter(resolver_context_t& ctx) {
	const auto& deltas = ctx.m_info->m_jitter.m_deltas;

	const auto first_delta = deltas[ctx.m_info->m_jitter.m_deltas_offset % JITTER_CACHE_SIZE];

	for (ptrdiff_t i = 1; i < JITTER_CACHE_SIZE; ++i)
		if (std::abs(deltas[(i + ctx.m_info->m_jitter.m_deltas_offset) % JITTER_CACHE_SIZE] - first_delta) > JITTER_BEGIN_ANGLE)
			return false;

	return true;
}

bool resolver_t::jitter_fix(resolver_context_t& ctx) {
	if (ctx.m_record->m_previous == nullptr)
		return false;

	auto& jitter = ctx.m_info->m_jitter;

	// ignore shots/holding use button & other shit
	if (ctx.m_record->m_eye_angles.x < 45.f || ctx.m_previous == nullptr) {
		++jitter.m_static_ticks;
		jitter.m_switch_count = 0;
		return false;
	}

	// difference between current and last angle
	const float eye_delta = math::normalize_yaw(ctx.m_record->m_eye_angles.y - ctx.m_record->m_previous->m_eye_angles.y);
	const float abs_eye_delta = std::abs(eye_delta);

	// if angle doesn't switch alot, reset the timer
	if (abs_eye_delta <= JITTER_BEGIN_ANGLE) {
		jitter.m_switch_count = 0;
		++jitter.m_static_ticks;
	} else {
		// increase the counter & reset static ticks
		++jitter.m_switch_count;
		jitter.m_static_ticks = 0;

		// cache the eye delta
		// so we will use it to check static or dynamic jitter
		jitter.m_deltas[(ctx.m_info->m_jitter.m_deltas_offset++) % JITTER_CACHE_SIZE] = abs_eye_delta;
	}

	// if entity angles don't jittering skip em
	if (jitter.m_static_ticks > JITTER_CACHE_SIZE)
		return false;

	// we won't fix random angle jitters using that logic
	// so skip enemy with dynamic jitter
	if (!static_jitter(ctx))
		return false;

	// we're selecting closest side to center angle
	const auto inner_delta = math::normalize_yaw(ctx.m_record->m_eye_angles.y - ctx.m_previous->m_eye_angles.y);
	const auto inner_side = inner_delta > 0.f ? -1 : 1;
	const auto misses = ctx.m_info->m_missed_shots[e_resolver_mode::jitter];

	ctx.m_info->m_jitter.m_should_fix = true;
	switch (misses % 2) {
		case 0:
			ctx.m_info->m_mode.set(e_resolver_mode::jitter, 0);
			ctx.m_info->m_side = inner_side;
			break;
		case 1:
			ctx.m_info->m_mode.set(e_resolver_mode::jitter, 1);
			ctx.m_info->m_side = -inner_side;
			break;
	}

	return true;
}

void resolver_t::check_inverse_side(resolver_context_t& ctx) {
	const auto sideways_left = std::abs(math::normalize_yaw(ctx.m_angle - get_left_yaw(ctx.m_player))) < 45.f;
	const auto sideways_right = std::abs(math::normalize_yaw(ctx.m_angle - get_right_yaw(ctx.m_player))) < 45.f;

	const auto forward = std::abs(math::normalize_yaw(ctx.m_angle - get_forward_yaw(ctx.m_player))) < 45.f;

	ctx.m_inverse_side = (forward || sideways_right) && !sideways_left;
	ctx.m_info->m_using_sideways = sideways_left || sideways_right;
}

bool resolver_t::air_fix(resolver_context_t& ctx) {
	if (ctx.m_info->m_ground_ticks > 1)
		return false;

	const auto misses = ctx.m_info->m_missed_shots[e_resolver_mode::jumping];
	if (misses == 0) {
		ctx.m_info->m_side = 0;
		ctx.m_info->m_mode.set(e_resolver_mode::jumping, 0);
	} else {
		switch (misses % 2) {
			case 0: ctx.m_info->m_side = -m_freestand_side[ctx.m_index]; break;
			case 1: ctx.m_info->m_side = m_freestand_side[ctx.m_index]; break;
		}
		ctx.m_info->m_mode.set(e_resolver_mode::jumping, 1);
	}

	return true;
}

void resolver_t::apply_edge(resolver_context_t& ctx) {
	if (m_freestand_side[ctx.m_index] != 0) {
		ctx.m_info->m_mode.set(e_resolver_mode::edge, ctx.m_inverse_side);
		ctx.m_info->m_side = ctx.m_inverse_side ? -m_freestand_side[ctx.m_index] : m_freestand_side[ctx.m_index];
	} else {
		ctx.m_info->m_mode.set(e_resolver_mode::edge, 0);
		ctx.m_info->m_side = 0;
	}
}

void resolver_t::apply_bruteforce(resolver_context_t& ctx) {
	const auto misses = ctx.m_info->m_missed_shots[e_resolver_mode::edge];
	const auto brute = /*ctx.m_info->m_roll.m_enabled ? (misses >> 1) : */ misses; // prevent changing side on every miss

	ctx.m_info->m_mode.set(e_resolver_mode::edge, (int)ctx.m_inverse_side | 2);

	if (m_freestand_side[ctx.m_index] == 0)
		m_freestand_side[ctx.m_index] = brute % 2 ? 1 : -1;

	switch (brute % 2) {
		case 0: ctx.m_info->m_side = ctx.m_inverse_side ? -m_freestand_side[ctx.m_index] : m_freestand_side[ctx.m_index]; break;
		case 1: ctx.m_info->m_side = ctx.m_inverse_side ? m_freestand_side[ctx.m_index] : -m_freestand_side[ctx.m_index]; break;
	}
}

void resolver_t::apply(resolver_info_t* info) {
	// set side in resolver info
	info->m_mode.set_additional_info((info->m_side == 0 ? 1 : 0) | ((info->m_side == 1 ? 1 : 0) << 1));
}

void resolver_t::reset(int idx) {
	m_info[idx].reset();
}

bool resolver_t::run(lag_record_t* record, lag_record_t* previous) {
	auto player = record->m_player;
	int index = player->index();
	auto& info = m_info[index];
	info.m_jitter.m_should_fix = false;

	auto& mode = info.m_mode;

	mode.reset();

	auto animstate = player->animstate();
	if (!animstate) {
		mode.set(e_resolver_mode::off, 1);
		info.m_side = 0;
		info.m_desync = 0.f;
		apply(&info);
		return false;
	}

	if (!settings->ragebot.enable) {
		mode.set(e_resolver_mode::off, 2);
		info.m_side = 0;
		info.m_desync = 0.f;
		apply(&info);
		return false;
	}

	if (!globals->m_local->is_alive()) {
		mode.set(e_resolver_mode::off, 3);
		info.m_side = 0;
		info.m_desync = 0.f;
		apply(&info);
		return false;
	}

	if (player->flags().has(fl_onground))
		++info.m_ground_ticks;
	else
		info.m_ground_ticks = 0;

	if (player->eye_angles().x < 45.f)
		++info.m_zero_pitch_ticks;
	else
		info.m_zero_pitch_ticks = 0;

	resolver_context_t ctx{};
	ctx.m_player = player;
	ctx.m_index = index;
	ctx.m_record = record;
	ctx.m_previous = previous;
	ctx.m_info = &info;

#ifndef _DEBUG
	if (player->get_player_info().m_fakeplayer) {
		info.m_side = 0;
		info.m_desync = 0.f;
		apply(&info);
		return false;
	}
#endif

	// detect if enemy triggers sidemove for desync
	if (ctx.m_record->m_velocity.length2d() < 5.f) {
		// feet cycle only changes when enemy is moving / accelerating
		// when enemy is standing cycle stops his calculations
		if (animstate->m_primary_cycle != info.m_old_feet_cycle) {
			info.m_cycle_change_time = interfaces::global_vars->m_curtime;
			info.m_old_feet_cycle = animstate->m_primary_cycle;
		}

		info.m_using_micro_movement = std::abs(interfaces::global_vars->m_curtime - info.m_cycle_change_time) <= 0.1f;

	} else
		info.m_using_micro_movement = false;

	ctx.m_angle = get_angle(player);
	ctx.m_desync = player->max_desync();
	ctx.m_speed = ctx.m_record->m_velocity.length2d();

	// check if we should invert anti freestand side
	check_inverse_side(ctx);

	if (jitter_fix(ctx)) {
		apply(&info);
		return true;
	}

	if (air_fix(ctx)) {
		apply(&info);
		return true;
	}

	if (ctx.m_info->m_missed_shots[e_resolver_mode::animation_layers] <= 3) {
		if (can_use_animations(ctx)) {
			apply(&info);
			return true;
		}
	}

	if (ctx.m_info->m_missed_shots[e_resolver_mode::edge] == 0)
		apply_edge(ctx);
	else
		apply_bruteforce(ctx);

	ctx.m_info->m_desync = ctx.m_desync;

	apply(&info);
	return true;
}

void resolver_t::force_off(cs_player_t* player) {
	int idx = player->index();
	auto& info = m_info[idx];

	info.m_mode.reset();
	info.m_mode.set(e_resolver_mode::off, 4);

	apply(&info);
}

void resolver_t::apply_resolver_info(lag_record_t* anims) {
	std::memcpy(&anims->m_resolver, &m_info[anims->m_player->index()], sizeof(resolver_info_t));
}