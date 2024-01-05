#include "animation_handler.hpp"
#include "../interfaces.hpp"

void utils::animation_helper_t::update() {
	m_animation_speed = std::clamp(std::abs(interfaces::global_vars->m_realtime - m_last_time), 0.f, 1.f);
	m_last_time = interfaces::global_vars->m_realtime;
}