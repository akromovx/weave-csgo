#pragma once
#include "../features/visuals/esp_utils.hpp"
#include "../../sdk/input.hpp"
#include "../../deps/weave-gui/include.hpp"

struct inferno_entry_t {
	std::vector<vec3d> m_points{};
	vec3d m_entity_origin{};
	vec3d m_origin{};
	float m_time_to_die{};
	float m_range{};
	float m_visual_range{};
	std::optional<vec3d> m_visual_origin{};
};

struct projectile_entry_t {
	enum class e_type : hash_t {
		unknown = HASH("projectile-type:unknown"),
		unitialized = HASH("projectile-type:unitialized"),
		flashbang = HASH("projectile-type:flashbang"),
		hegrenade = HASH("projectile-type:hegrenade"),
		fire = HASH("projectile-type:fire"),
		smokegrenade = HASH("projectile-type:smokegrenade"),
		decoy = HASH("projectile-type:decoy"),
		snowball = HASH("projectile-type:snowball")
	};

	sdk::base_grenade_t* m_entity{};
	e_type m_type{ e_type::unitialized };
	bool m_exploded{};
	vec3d m_predicted_position{};
	vec3d m_origin{};
	vec3d m_old_velocity{};
	float m_spawn_time{};
	float m_detonate_time{};
	int m_predicted_damage{};
	float m_distance_to_local{};
	float m_he_switch_animation{};
	bool m_thrown_by_local{};
	std::string m_icon{};
	std::vector<vec3d> m_predicted_path{};
};

struct projectiles_t {
	void on_frame_render_start();
	void on_create_move(sdk::user_cmd_t* cmd);
	void on_override_view();
	std::vector<vec3d> m_predicted_path{};
	int m_total_predicted_damage{};
	std::mutex m_mutex{};
private:
	int m_act{};
	int m_type{};
};

GLOBAL_DYNPTR(projectiles_t, projectiles);