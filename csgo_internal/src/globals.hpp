#pragma once
#include "../sdk/entities.hpp"
#include "../sdk/input.hpp"
#include <deque>

struct command_t {
	int previous_command_number = 0;
	bool is_used = false;
	int cmd_number = 0;
	bool is_outgoing = false;
};

struct globals_t {
	sdk::user_cmd_t* m_cmd = nullptr;
	bool* m_send_packet = nullptr;
	sdk::cs_player_t* m_local = nullptr;
	float m_local_alpha{};
	float m_thirdperson_alpha{};
	sdk::base_combat_weapon_t* m_weapon = nullptr;
	vec3d m_original_angle{};
	vec3d m_eye_position{};

	vec3d m_nades_eye_position{};

	bool m_frametime_dropped{};
	int m_groundtick{};
	int m_airtick{};
	bool m_fake_duck{};
	bool m_round_start{};

	float m_tickrate{};
	float m_weapon_inaccuracy{};
	float m_weapon_spread{};

	int m_tickbase{};

	//std::vector<int> m_commands{};
	std::deque<command_t> m_cmds{};
	std::vector<int> m_choke_commands;

	bool m_local_alive{};
	int m_local_hp{};

	bool m_in_prediction{};
	bool m_is_connected{};
	int m_zoom_level{};
	vec3d m_old_velocity{};

	vec3d m_last_shoot_angles{};
	vec3d m_last_shoot_position{};

	std::string m_default_skybox{};
	std::string m_previous_skybox{};

	void on_cl_move();
	void on_render_start();
};

GLOBAL_DYNPTR(globals_t, globals);