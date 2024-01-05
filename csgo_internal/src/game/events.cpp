#include "events.hpp"
#include "../features/bullets.hpp"
#include "../features/hvh/autowall.hpp"
#include "../features/hvh/hvh.hpp"
#include "../features/misc.hpp"
#include "../features/visuals/logs.hpp"
#include "../globals.hpp"
#include "players.hpp"

#include <format>

using namespace sdk;

namespace events {
	static void round_prestart(game_event_t* e) {
		players->m_local_player.m_valid = false;
		misc->buy_items();
	}

	static void round_start(game_event_t* e) {
		misc->buy_items();
		resolver->reset_all();

		globals->m_round_start = true;
		players->m_local_player.m_valid = false;
		players->for_each_entry([](player_entry_t* entry) { entry->m_visual_alpha = 0.f; });
		globals->m_local_alpha = 0.f;
	}

	static void round_end(game_event_t* e) {
		players->m_local_player.m_valid = false;
		globals->m_local_alpha = 0.f;
	}

	static void player_hurt(game_event_t* e) {
		bullet_impacts->on_player_hurt(e);
	}

	static void bullet_impact(game_event_t* e) {
		bullet_impacts->on_bullet_impact(e);
		bullet_tracer->on_bullet_impact(e);
	}

	static void item_purchase(game_event_t* e) {
	}

	static void player_death(game_event_t* e) {
	}

	static void player_given_c4(game_event_t* e) {
	}

	static void bomb_beginplant(game_event_t* e) {
	}

	static void bomb_abortplant(game_event_t* e) {
	}

	static void bomb_planted(game_event_t* e) {
	}

	static void bomb_beep(game_event_t* e) {
	}

	static void bomb_begindefuse(game_event_t* e) {
	}

	static void bomb_abortdefuse(game_event_t* e) {
	}

	static void bomb_defused(game_event_t* e) {
	}

	static void bomb_exploded(game_event_t* e) {
	}

	static void weapon_fire(game_event_t* e) {
		bullet_impacts->on_weapon_fire(e);
	}
} // namespace events

void event_listener_t::init() {
	using namespace events;

	add(STRS("round_start"), round_start);
	add(STRS("round_prestart"), round_prestart);
	add(STRS("round_end"), round_end);
	add(STRS("player_hurt"), player_hurt);
	add(STRS("bullet_impact"), bullet_impact);
	add(STRS("item_purchase"), item_purchase);
	add(STRS("player_death"), player_death);
	add(STRS("player_given_c4"), player_given_c4);
	add(STRS("bomb_beginplant"), bomb_beginplant);
	add(STRS("bomb_abortplant"), bomb_abortplant);
	add(STRS("bomb_planted"), bomb_planted);
	add(STRS("bomb_beep"), bomb_beep);
	add(STRS("bomb_begindefuse"), bomb_begindefuse);
	add(STRS("bomb_abortdefuse"), bomb_abortdefuse);
	add(STRS("bomb_exploded"), bomb_exploded);
	add(STRS("bomb_defused"), bomb_defused);
	add(STRS("weapon_fire"), weapon_fire);

	register_events();
}