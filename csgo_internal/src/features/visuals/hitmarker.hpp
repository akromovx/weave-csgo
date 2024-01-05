#pragma once
#include "../../base_includes.hpp"
#include "../../utils/vector.hpp"
#include "../../../sdk/game_events.hpp"
#include <mutex>

struct hitmarker_t {
	void render();

	void add(vec3d position, int damage, float time, bool headshot = false) {
		m_last_hit_time = time;
		THREAD_SAFE(m_mutex);
		m_hits.emplace_back(position, damage, time, headshot);
	}

	float m_last_hit_time{};

private:
	struct log_t {
		vec3d m_position{};
		int m_damage{};
		float m_time{};
		bool m_headshot{};

		float calculate_alpha() const;
	};

	std::vector<log_t> m_hits{};
	std::mutex m_mutex{};
};

GLOBAL_DYNPTR(hitmarker_t, hitmarker);