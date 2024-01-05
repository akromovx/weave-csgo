#pragma once
#include "../base_includes.hpp"
#include "../features/visuals/esp_utils.hpp"

struct weapon_entry_t {
	bool m_valid{};
	vec3d m_origin{};
	std::string m_name{};
	std::string m_icon{};
	int m_ammo{};
	int m_max_ammo{};
	esp::bounding_box_t m_box{};

	float m_distance_to_local{};
};

struct weapons_t {
	void on_paint_traverse();
};

GLOBAL_DYNPTR(weapons_t, weapons);