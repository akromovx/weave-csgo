#include "engine_prediction.hpp"

#include "../../sdk/entities.hpp"
#include "../../sdk/game_movement.hpp"

#include "../cheat.hpp"
#include "../globals.hpp"

#include "../utils/hotkeys.hpp"
#include "../utils/md5.hpp"

#include "../features/hvh/exploits.hpp"
#include "../features/hvh/hvh.hpp"
#include "../features/misc.hpp"

using namespace sdk;
using namespace interfaces;

void engine_prediction_t::entry_t::store(sdk::user_cmd_t* cmd) {
	std::memcpy(&m_cmd, cmd, sizeof(user_cmd_t));
	std::memcpy(&m_global_vars, global_vars, sizeof(global_vars_t));

	m_origin = globals->m_local->origin();
	m_velocity = globals->m_local->velocity();
	m_eye_position = globals->m_local->eye_position();

	m_flags = globals->m_local->flags();
	m_tickbase = globals->m_local->tickbase();

	if (globals->m_weapon->is_gun()) {
		m_spread = globals->m_weapon->get_spread();
		m_inaccuracy = globals->m_weapon->get_inaccuracy();
	}

	auto viewmodel = (base_viewmodel_t*)globals->m_local->viewmodel_handle().get();
	if (viewmodel == nullptr)
		return;

	m_viewmodel.m_cycle = viewmodel->cycle();
	m_viewmodel.m_animation_parity = viewmodel->animation_parity();

	m_shift = 0;
	m_sequence = cmd->m_command_number;
}

void engine_prediction_t::store_unpredicted_data(user_cmd_t* cmd) {
	m_unpredicted_data.store(cmd);
	m_entries[cmd->m_command_number % max_input].store(cmd);
}

void engine_prediction_t::update(user_cmd_t* cmd) {
	if (client_state->m_delta_tick <= 0)
		return;

	prediction->update(client_state->m_delta_tick, true, client_state->m_last_command_ack, client_state->m_last_outgoing_command + client_state->m_choked_commands);
}

STFI void apply_abs_velocity_impulse(vec3d v) { patterns::apply_abs_velocity_impulse.as<void(__thiscall*)(void*, const vec3d&)>()(globals->m_local, v); }
STFI void pre_think() { patterns::pre_think.as<void(__thiscall*)(void*)>()(globals->m_local); }
STFI void think() { patterns::think.as<void(__thiscall*)(void*)>()(globals->m_local); }
STFI void think_unk() { patterns::think_unk.as<char(__thiscall*)(void*, char)>()(globals->m_local, 0); }
STFI void simulate_player_simulated_entities() { patterns::simulate_player_simulated_entities.as<void(__thiscall*)(void*)>()(globals->m_local); }
STFI void post_think_vphysics() { patterns::post_think_vphysics.as<void(__thiscall*)(void*)>()(globals->m_local); }

STFI void post_think() {
	interfaces::model_cache->begin_lock();
	globals->m_local->update_collision_bounds();

	if (globals->m_local->flags().has(fl_onground))
		globals->m_local->fall_velocity() = 0.f;

	if (((base_animating_t*)globals->m_local)->sequence() == -1)
		globals->m_local->set_sequence(0);

	globals->m_local->studio_frame_advance();

	post_think_vphysics();
	simulate_player_simulated_entities();

	interfaces::model_cache->end_lock();
}

static bool old_in_prediction, old_first_time_predicted;
void engine_prediction_t::begin(user_cmd_t* cmd) {
	if (hvh::exploits->m_in_shift)
		return;

	m_is_predicted = true;

	old_in_prediction = interfaces::prediction->m_in_prediction;
	old_first_time_predicted = interfaces::prediction->m_is_first_time_predicted;

	interfaces::global_vars->m_curtime = TICKS_TO_TIME(globals->m_local->tickbase());
	interfaces::global_vars->m_frametime = interfaces::global_vars->m_interval_per_tick;

	*(user_cmd_t**)((uintptr_t)globals->m_local + 0x3348) = cmd;
	*(user_cmd_t*)((uintptr_t)globals->m_local + 0x3298) = *cmd;

	**patterns::prediction_random_seed.as<int**>() = MD5_PseudoRandom(cmd->m_command_number) & INT_MAX;
	**patterns::prediction_player.as<int**>() = (int)globals->m_local;

	interfaces::prediction->m_in_prediction = true;
	interfaces::prediction->m_is_first_time_predicted = false;

	auto state = globals->m_local->animstate();
	if (!state)
		return;

	auto old_tickbase = globals->m_local->tickbase();

	cmd->m_buttons |= globals->m_local->button_forced();
	cmd->m_buttons &= ~(globals->m_local->button_disabled());

	interfaces::game_movement->start_track_prediction_errors(globals->m_local);

	const int buttons = cmd->m_buttons;
	const int local_buttons = globals->m_local->buttons();
	const int buttons_changed = buttons ^ local_buttons;

	globals->m_local->button_last() = local_buttons;
	globals->m_local->buttons() = buttons;
	globals->m_local->button_pressed() = buttons_changed & buttons;
	globals->m_local->button_released() = buttons_changed & (~buttons);

	memset(&m_move_data, 0, sizeof(move_data_t));

	interfaces::prediction->check_moving_ground(globals->m_local, interfaces::global_vars->m_frametime);
	interfaces::prediction->set_local_viewangles(cmd->m_viewangles);

	pre_think();
	think();

	interfaces::move_helper->set_host(globals->m_local);
	interfaces::prediction->setup_move(globals->m_local, cmd, interfaces::move_helper, &m_move_data);
	interfaces::game_movement->process_movement(globals->m_local, &m_move_data);
	interfaces::prediction->finish_move(globals->m_local, cmd, &m_move_data);

	interfaces::move_helper->process_impacts();

	post_think();

	globals->m_local->tickbase() = old_tickbase;

	interfaces::game_movement->finish_track_prediction_errors(globals->m_local);
	interfaces::move_helper->set_host(nullptr);

	if (globals->m_weapon->is_gun())
		globals->m_weapon->update_accuracy_penalty();
}

void engine_prediction_t::repredict(user_cmd_t* cmd) {
	if (hvh::exploits->m_in_shift)
		return;

	if (!m_is_predicted)
		return;

	auto player = globals->m_local;

	interfaces::game_movement->start_track_prediction_errors(player);

	m_move_data.m_forwardmove = cmd->m_forwardmove;
	m_move_data.m_sidemove = cmd->m_sidemove;
	m_move_data.m_upmove = cmd->m_upmove;
	m_move_data.m_buttons = cmd->m_buttons;
	m_move_data.m_view_angles = cmd->m_viewangles;
	m_move_data.m_abs_view_angles = cmd->m_viewangles;
	m_move_data.m_impulse_command = cmd->m_impulse;

	interfaces::move_helper->set_host(player);
	interfaces::prediction->setup_move(globals->m_local, cmd, interfaces::move_helper, &m_move_data);
	interfaces::game_movement->process_movement(globals->m_local, &m_move_data);
	interfaces::prediction->finish_move(player, cmd, &m_move_data);
	interfaces::game_movement->finish_track_prediction_errors(player);
	interfaces::move_helper->set_host(nullptr);

	//update();

	globals->m_weapon->update_accuracy_penalty();

	const auto hideshot = hvh::exploits->m_type == hvh::exploits_t::e_type::hideshot && globals->m_weapon->is_gun() && globals->m_weapon->item_definition_index() != e_weapon_type::weapon_revolver;
	const auto curtime = TICKS_TO_TIME(/*hideshot ? globals->m_local->tickbase() - 8 : */ globals->m_local->tickbase());

	interfaces::global_vars->m_curtime = curtime - TICKS_TO_TIME(1);
	interfaces::global_vars->m_frametime = interfaces::prediction->m_engine_paused ? 0.0f : interfaces::global_vars->m_interval_per_tick;
}

void engine_prediction_t::end(user_cmd_t* cmd) {
	if (hvh::exploits->m_in_shift)
		return;

	//this->repredict(cmd);

	*(user_cmd_t**)((uintptr_t)globals->m_local + 0x3348) = 0;
	**patterns::prediction_random_seed.as<int**>() = -1;
	**patterns::prediction_player.as<int**>() = 0;

	global_vars->m_curtime = m_unpredicted_data.m_global_vars.m_curtime;
	global_vars->m_frametime = m_unpredicted_data.m_global_vars.m_frametime;

	interfaces::game_movement->reset();

	interfaces::prediction->m_in_prediction = old_in_prediction;
	interfaces::prediction->m_is_first_time_predicted = old_first_time_predicted;

	m_is_predicted = false;

	m_last_data.store(cmd);

	if (*globals->m_send_packet)
		m_last_sent_data.store(cmd);
}

void engine_prediction_t::on_render_start(bool after) {
	if (!globals->m_local_alive)
		return;

	if (!after) {
		m_interpolation_amount = interfaces::global_vars->m_interpolation_amount;
		if (hvh::exploits->m_charging)
			interfaces::global_vars->m_interpolation_amount = 0.f;

		m_last_data.m_origin = globals->m_local->origin();
		auto viewmodel = (base_viewmodel_t*)globals->m_local->viewmodel_handle().get();
		if (viewmodel != nullptr) {
			m_last_data.m_viewmodel.m_cycle = viewmodel->cycle();
			m_last_data.m_viewmodel.m_animation_parity = viewmodel->animation_parity();
		}
	} else
		interfaces::global_vars->m_interpolation_amount = m_interpolation_amount;

	if (!after) {
		if (globals->m_local_alive)
			globals->m_local->set_abs_origin(globals->m_local->origin());
	}
}

void engine_prediction_t::on_run_command(sdk::user_cmd_t* cmd, bool after) {
	static float backup_cycle = 0.f;
	static bool weapon_triggered = false;

	if (globals->m_local == nullptr || !globals->m_local->is_alive()) {
		backup_cycle = 0.f;
		weapon_triggered = false;
		m_cycle_changed = false;
		m_fix_cycle = false;
		return;
	}

	auto viewmodel = (base_viewmodel_t*)globals->m_local->viewmodel_handle().get();
	if (viewmodel != nullptr) {
		if (!after) {
			weapon_triggered = cmd->m_weapon_select > 0 || cmd->m_buttons.has(in_attack | in_attack2);
			backup_cycle = viewmodel->cycle();
		} else if (weapon_triggered && !m_cycle_changed)
			m_cycle_changed = viewmodel->cycle() == 0.f && backup_cycle > 0.f;
	}
}

void engine_prediction_t::on_net_update_postdataupdate_start() {
	static float networked_cycle = 0.f;
	static float animation_time = 0.f;

	if (!globals->m_local->is_alive()) {
		networked_cycle = animation_time = 0.f;
		m_fix_cycle = false;
		return;
	}

	auto viewmodel = (base_viewmodel_t*)globals->m_local->viewmodel_handle().get();
	if (viewmodel != nullptr) {
		if (m_fix_cycle && viewmodel->cycle() == 0.f) {
			viewmodel->cycle() = networked_cycle;
			viewmodel->animtime() = animation_time;
			m_fix_cycle = false;
		}

		networked_cycle = viewmodel->cycle();
		animation_time = viewmodel->animtime();
	}
}