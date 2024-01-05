#pragma once
#include <deque>

#include "../base_includes.hpp"
#include "../../sdk/game_events.hpp"
#include "../utils/vector.hpp"
#include "../../sdk/engine_trace.hpp"
#include "../game/players.hpp"

struct shot_record_t {
	std::deque<vec3d> m_impacts{};
	float m_last_impact_time{};

	vec3d m_eye_position{};
	float m_time{};
	bool m_weapon_fired{};
	bool m_hit{};

	std::optional<vec3d> get_farthest_impact() const;
	std::optional<vec3d> get_closest_impact_to(const vec3d&) const;
};

struct shot_record_rage_t : shot_record_t {
	lag_record_t m_lag_record{};
	sdk::e_hitbox m_hitbox{};
	vec3d m_position{};
	bits8_t m_safe{};

	bool m_confirmed{};
	bool m_skip{};
	int m_index{};

	int m_damage{};
	int m_health{};
	sdk::e_hitgroup m_hitgroup{};
};

struct bullet_tracer_t {
	void on_bullet_impact(sdk::game_event_t* e);
	void on_weapon_fire(sdk::game_event_t* e);
	void on_render_start();
	void render();

	__forceinline shot_record_t& push() { return m_local_tracers.emplace_back(); }
	__forceinline void clear() {
		m_client_impacts.clear();
		m_server_impacts.clear();
		m_last_impact_recieved = std::nullopt;
	}
	struct impact_info_t {
		float m_time{};
		vec3d m_position{};
		bool m_valid{};

		float calculate_alpha(float impact_time = 4.f) const;
	};

	void erase_outdated_impacts(std::vector<impact_info_t>& list, float impact_time = 4.f);
	void draw_tracers();

private:
	void draw_tracers(std::vector<shot_record_t>& tracers, color_t color, int type, float duration = 4.f);

	std::vector<shot_record_t> m_local_tracers{};
	std::vector<shot_record_t> m_enemy_tracers{};

	std::vector<impact_info_t> m_client_impacts{};
	std::vector<impact_info_t> m_server_impacts{};
	std::optional<vec3d> m_last_impact_recieved{};

	void store_client_impacts();
	void draw_impacts(const std::vector<impact_info_t>& list, c_imgui_color colors[2]);
};

struct bullet_impacts_t {
	std::function<void()> m_on_fire{ nullptr };

	void on_weapon_fire(sdk::game_event_t* e);
	void on_bullet_impact(sdk::game_event_t* e);
	void on_player_hurt(sdk::game_event_t* e);
	void on_pre_move();
	void render();

	std::deque<shot_record_rage_t> m_shots{};
};

GLOBAL_DYNPTR(bullet_impacts_t, bullet_impacts);
GLOBAL_DYNPTR(bullet_tracer_t, bullet_tracer);