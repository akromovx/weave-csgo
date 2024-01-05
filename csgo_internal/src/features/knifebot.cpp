#include "knifebot.hpp"

#include "../globals.hpp"
#include "../interfaces.hpp"

#include "../game/players.hpp"
#include "../game/override_entity_list.hpp"

#include "hvh/hvh.hpp"

struct knife_table_t {
	uint8_t m_swing[2][2][2]; // [ first ][ armor ][ back ]
	uint8_t m_stab[2][2];	  // [ armor ][ back ]
};

static const knife_table_t knife_damages = {
	{
			{ { 25, 90 }, { 21, 76 } },
			{ { 40, 90 }, { 34, 76 } },
	},

	{ { 65, 180 }, { 55, 153 } }
};

static const std::vector<vec3d> knife_angles = {
	{ 0.f, 0.f, 0.f },

	{ 0.f, 45.f, 0.f },
	{ 0.f, -45.f, 0.f },
	{ 45.f, 0.f, 0.f },
	{ -45.f, 0.f, 0.f },

	{ 45.f, 45.f, 0.f },
	{ 45.f, -45.f, 0.f },
	{ -45.f, 45.f, 0.f },
	{ -45.f, -45.f, 0.f },
};

STFI bool knife_if_behind(lag_record_t* record) {
	vec3d delta{ record->m_origin - globals->m_eye_position };
	delta.z = 0.f;
	delta = delta.normalize_in_place();

	vec3d target;
	math::angle_vectors(record->m_eye_angles, target);
	target.z = 0.f;

	return delta.dot(target) > 0.475f;
}

STFI bool knife_trace(vec3d dir, bool stab, sdk::game_trace_t* trace) {
	float range = stab ? 32.f : 48.f;

	const vec3d start = globals->m_eye_position;
	const vec3d end = start + (dir * range);

	sdk::trace_filter_t filter;
	filter.m_skip = globals->m_local;
	interfaces::traces->trace_ray({ start, end }, MASK_SOLID, &filter, trace);

	// if the above failed try a hull trace.
	if (trace->m_fraction >= 1.f) {
		const vec3d mins = { -16.f, -16.f, -18.f }, maxs = { 16.f, 16.f, 18.f };
		interfaces::traces->trace_ray({ start, end, maxs - mins, maxs + mins }, MASK_SOLID, &filter, trace);
		return trace->m_fraction < 1.f;
	}

	return true;
}

STFI bool can_knife(lag_record_t* record, vec3d angle, bool& stab) {
	// convert target angle to direction.
	vec3d forward{};
	math::angle_vectors(angle, forward);

	// see if we can hit the player with full range
	// this means no stab.
	sdk::game_trace_t trace{};
	knife_trace(forward, false, &trace);

	// we hit smthing else than we were looking for.
	if (trace.m_entity == nullptr || trace.m_entity != record->m_player)
		return false;

	const auto armor = record->m_player->armor() > 0;
	const auto first = globals->m_weapon->next_primary_attack() + 0.4f < interfaces::global_vars->m_curtime;
	const auto back = knife_if_behind(record);

	const auto stab_dmg = knife_damages.m_stab[armor][back];
	const auto slash_dmg = knife_damages.m_swing[first][armor][back];
	const auto swing_dmg = knife_damages.m_swing[false][armor][back];

	// smart knifebot.
	const auto health = record->m_player->health();
	if (health <= slash_dmg)
		stab = false;
	else if (health <= stab_dmg)
		stab = true;
	else if (health > (slash_dmg + swing_dmg + stab_dmg))
		stab = true;
	else
		stab = false;

	// damage wise a stab would be sufficient here.
	if (stab && !knife_trace(forward, true, &trace))
		return false;

	return true;
}

void knifebot_t::on_create_move(sdk::user_cmd_t* cmd) {
	if (!settings->misc.knife_bot)
		return;

	if (!globals->m_weapon->is_knife())
		return;

	struct {
		bool m_stab{};
		vec3d m_angle{};
		lag_record_t* m_record{};
	} target{};

	for (auto& [player, entry]: entities->m_players) {
		if (!player->is_alive() || player->dormant() || player->is_teammate())
			continue;

		hvh::backup_player(player);
		DEFER([player]() { hvh::restore_player(player); });

		{
			auto best = players->find_record_if(player, lag_record_t::valid);
			if (best == nullptr)
				continue;

			best->apply(best->m_bones);
			vec3d stomach = hvh::aimbot->get_point(best->m_player, sdk::e_hitbox::stomach, best->m_bones);
			if (stomach.is_zero())
				continue;

			auto aim_angle = math::calculate_angle(globals->m_eye_position, stomach);

			// trace with best.
			for (const auto& a: knife_angles) {

				// check if we can knife.
				if (!can_knife(best, aim_angle + a, target.m_stab))
					continue;

				// set target data.
				target.m_angle = aim_angle + a;
				target.m_record = best;
				break;
			}

			auto last = players->rfind_record_if(player, lag_record_t::valid);

			if (last == nullptr || last == best)
				continue;

			stomach = hvh::aimbot->get_point(last->m_player, sdk::e_hitbox::stomach, best->m_bones);
			if (stomach.is_zero())
				continue;

			aim_angle = math::calculate_angle(globals->m_eye_position, stomach);

			last->apply(last->m_bones);

			// trace with last.
			for (const auto& a: knife_angles) {

				// check if we can knife.
				if (!can_knife(last, aim_angle + a, target.m_stab))
					continue;

				// set target data.
				target.m_angle = aim_angle + a;
				target.m_record = last;
				break;
			}
		}

		// target player has been found already.
		if (target.m_record != nullptr)
			break;
	}

	if (target.m_record != nullptr) {
		cmd->m_tickcount = TIME_TO_TICKS(target.m_record->m_simulation_time + hvh::lerp_time());
		cmd->m_viewangles = target.m_angle;
		cmd->m_buttons |= target.m_stab ? sdk::in_attack2 : sdk::in_attack;
	}
}