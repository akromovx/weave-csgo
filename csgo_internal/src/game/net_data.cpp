#include "net_data.hpp"
#include "../globals.hpp"

using namespace sdk;

void net_data_t::store() {
	if (globals->m_local == nullptr || !globals->m_local->is_alive())
		return reset();

	int tickbase;
	stored_data_t* data;

	tickbase = globals->m_local->tickbase();

	// get current record and store data.
	data = &m_data[tickbase % max_input];

	data->m_tickbase = tickbase;
	data->m_punch = globals->m_local->punch_angle();
	data->m_punch_vel = globals->m_local->punch_angle_vel();
	data->m_view_offset = globals->m_local->view_offset();
	data->m_velocity_modifier = globals->m_local->velocity_modifier();
}

void net_data_t::apply() {
	if (globals->m_local == nullptr || !globals->m_local->is_alive())
		return reset();

	int tickbase;
	stored_data_t* data;
	vec3d punch_delta, punch_vel_delta;
	vec3d view_delta;
	float modifier_delta;

	tickbase = globals->m_local->tickbase();

	// get current record and validate.
	data = &m_data[tickbase % max_input];

	if (globals->m_local->tickbase() != data->m_tickbase)
		return;

	// get deltas.
	// note - dex;  before, when you stop shooting, punch values would sit around 0.03125 and then goto 0 next update.
	//              with this fix applied, values slowly decay under 0.03125.
	punch_delta = globals->m_local->punch_angle() - data->m_punch;
	punch_vel_delta = globals->m_local->punch_angle_vel() - data->m_punch_vel;
	view_delta = globals->m_local->view_offset() - data->m_view_offset;
	modifier_delta = globals->m_local->velocity_modifier() - data->m_velocity_modifier;

	// set data.
	if (std::abs(punch_delta.x) < 0.03125f &&
		std::abs(punch_delta.y) < 0.03125f &&
		std::abs(punch_delta.z) < 0.03125f)
		globals->m_local->punch_angle() = data->m_punch;

	if (std::abs(punch_vel_delta.x) < 0.03125f &&
		std::abs(punch_vel_delta.y) < 0.03125f &&
		std::abs(punch_vel_delta.z) < 0.03125f)
		globals->m_local->punch_angle_vel() = data->m_punch_vel;

	if (std::abs(view_delta.x) < 0.03125f &&
		std::abs(view_delta.y) < 0.03125f &&
		std::abs(view_delta.z) < 0.03125f)
		globals->m_local->view_offset() = data->m_view_offset;

	if (std::abs(modifier_delta) < 0.03125f)
		globals->m_local->velocity_modifier() = data->m_velocity_modifier;
}

void net_data_t::reset() {
	m_data.fill({});
}