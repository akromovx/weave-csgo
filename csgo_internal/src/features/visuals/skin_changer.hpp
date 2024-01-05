#pragma once
#include "../../base_includes.hpp"
#include "../../../sdk/recv_props.hpp"
#include "../../../sdk/entities.hpp"

struct skin_changer_t {
	struct paint_kit_t {
		int m_id;
		std::string m_name;
		std::string m_weapon_name;
		std::string m_weapon_link;
		int m_weapon_id;
		int m_rarity;

		LPDIRECT3DTEXTURE9 m_texture;
	};

	bool m_parsed = false;
	std::vector<paint_kit_t> m_paint_kits{};

	void on_net_update_postdataupdate_start();
	void on_frame_stage_notify(sdk::e_client_frame_stage stage);

	void parse();
	void sequence_remap(sdk::recv_proxy_data_t* data, sdk::base_viewmodel_t* view_model);
	void reset();
};

GLOBAL_DYNPTR(skin_changer_t, skin_changer);