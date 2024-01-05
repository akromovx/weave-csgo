#include "globals.hpp"
#include "interfaces.hpp"
#include "utils/animation_handler.hpp"

void globals_t::on_cl_move() {
	static float last_frametime = interfaces::global_vars->m_frametime;

	m_local = **patterns::local_player.as<sdk::cs_player_t***>();
	//m_local = (sdk::cs_player_t*)interfaces::entity_list->get_client_entity(interfaces::engine->get_local_player());
	m_tickrate = 1.f / interfaces::global_vars->m_interval_per_tick;

	m_frametime_dropped = false /*interfaces::global_vars->m_frametime > last_frametime*/;
	last_frametime = interfaces::global_vars->m_frametime;
}

void globals_t::on_render_start() {
	m_local = **patterns::local_player.as<sdk::cs_player_t***>();
	animations_fsn->lerp(m_local_alpha, m_local->is_alive() ? 1.f : 0.f);

	//m_local = (sdk::cs_player_t*)interfaces::entity_list->get_client_entity(interfaces::engine->get_local_player());
}