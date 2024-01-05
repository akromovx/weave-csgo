#pragma once
#include "../../sdk/base_handle.hpp"
#include "../../sdk/input.hpp"
#include "../base_includes.hpp"
#include "../utils/vector.hpp"

struct net_data_t {
	void store();
	void apply();
	void reset();

private:
	struct stored_data_t {
		int m_tickbase;
		vec3d m_punch;
		vec3d m_punch_vel;
		vec3d m_view_offset;
		float m_velocity_modifier;

		__forceinline stored_data_t() : m_tickbase{}, m_punch{}, m_punch_vel{}, m_view_offset{}, m_velocity_modifier{} {};
	};

	std::array<stored_data_t, 150> m_data;
};

GLOBAL_DYNPTR(net_data_t, net_data);