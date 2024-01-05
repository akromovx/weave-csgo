#include "misc.hpp"
#include "../game/inventory.hpp"
#include "../globals.hpp"
#include "../interfaces.hpp"
#include "../utils/hotkeys.hpp"

using namespace sdk;

namespace autobuy {
	enum class e_weapon_group {
		single,
		auto_sniper,
		rifle,
		sniper_rifle,
		auto_pistol,
		smg,
		heavy_pistol
	};

	STFI int item_def_index_by_name(std::string name) {
		switch (fnva1(name.c_str())) {
			case HASH("scar20"):
				return e_weapon_type::weapon_scar20;
			case HASH("g3sg1"):
				return e_weapon_type::weapon_g3sg1;
			case HASH("ssg08"):
				return e_weapon_type::weapon_ssg08;
			case HASH("awp"):
				return e_weapon_type::weapon_awp;
			case HASH("negev"):
				return e_weapon_type::weapon_negev;
			case HASH("m249"):
				return e_weapon_type::weapon_m249;
			case HASH("ak47"):
				return e_weapon_type::weapon_ak47;
			case HASH("m4a1"):
				return e_weapon_type::weapon_m4a1;
			case HASH("m4a1_silencer"):
				return e_weapon_type::weapon_m4a1s;
			case HASH("aug"):
				return e_weapon_type::weapon_aug;
			case HASH("sg556"):
				return e_weapon_type::weapon_sg553;
			case HASH("elite"):
				return e_weapon_type::weapon_dualberetta;
			case HASH("p250"):
				return e_weapon_type::weapon_p250;
			case HASH("tec9"):
				return e_weapon_type::weapon_tec9;
			case HASH("fn57"):
				return e_weapon_type::weapon_cz75;
			case HASH("deagle"):
				return e_weapon_type::weapon_deagle;
			case HASH("revolver"):
				return e_weapon_type::weapon_revolver;
		}

		return -1;
	}

	STFI e_weapon_group get_weapon_type(int def_index) {
		switch (def_index) {
			case e_weapon_type::weapon_scar20:
			case e_weapon_type::weapon_g3sg1:
				return e_weapon_group::auto_sniper;

			case e_weapon_type::weapon_mac10:
			case e_weapon_type::weapon_mp9:
			case e_weapon_type::weapon_mp7:
			case e_weapon_type::weapon_mp5sd:
			case e_weapon_type::weapon_ump45:
			case e_weapon_type::weapon_p90:
			case e_weapon_type::weapon_bizon:
				return e_weapon_group::smg;

			case e_weapon_type::weapon_galil:
			case e_weapon_type::weapon_famas:
			case e_weapon_type::weapon_ak47:
			case e_weapon_type::weapon_m4a1:
			case e_weapon_type::weapon_m4a1s:
				return e_weapon_group::rifle;

			case e_weapon_type::weapon_aug:
			case e_weapon_type::weapon_sg553:
				return e_weapon_group::sniper_rifle;

			case e_weapon_type::weapon_tec9:
			case e_weapon_type::weapon_cz75:
				return e_weapon_group::auto_pistol;

			case e_weapon_type::weapon_deagle:
			case e_weapon_type::weapon_revolver:
				return e_weapon_group::heavy_pistol;
		}

		return e_weapon_group::single;
	}

	STFI std::vector<int> get_weapon_ids_by_type(e_weapon_group type) {
		switch (type) {
			case e_weapon_group::auto_sniper:
				return { e_weapon_type::weapon_scar20, e_weapon_type::weapon_g3sg1 };

			case e_weapon_group::rifle:
				return { e_weapon_type::weapon_ak47, e_weapon_type::weapon_m4a1, e_weapon_type::weapon_m4a1s };

			case e_weapon_group::sniper_rifle:
				return { e_weapon_type::weapon_aug, e_weapon_type::weapon_sg553 };

			case e_weapon_group::auto_pistol:
				return { e_weapon_type::weapon_tec9, e_weapon_type::weapon_cz75 };

			case e_weapon_group::heavy_pistol:
				return { e_weapon_type::weapon_deagle, e_weapon_type::weapon_revolver };

			case e_weapon_group::smg:
				return { e_weapon_type::weapon_mac10, e_weapon_type::weapon_mp9, e_weapon_type::weapon_mp7,
						 e_weapon_type::weapon_mp5sd, e_weapon_type::weapon_ump45, e_weapon_type::weapon_p90, e_weapon_type::weapon_bizon };
		}

		return {};
	}

	STFI bool check_weapon(int def_index) {
		if (def_index == -1)
			return true;

		auto weapon_type = get_weapon_type(def_index);
		if (weapon_type == e_weapon_group::single) {
			for (auto weapon: inventory->get_items())
				if (weapon == def_index)
					return false;
		} else {
			auto weapon_ids = get_weapon_ids_by_type(weapon_type);
			for (auto weapon: inventory->get_items()) {
				for (auto weapon_id: weapon_ids) {
					if (weapon == weapon_id)
						return false;
				}
			}
		}
		return true;
	}

	static void add_weapon(std::string weapon, std::string& str) {
		if (check_weapon(item_def_index_by_name(weapon)))
			str += STRS("buy ") + weapon + STRS("; ");
	}
} // namespace autobuy

void misc_t::preserve_kill_feed(bool round_start) {
	static auto hud_ptr = *patterns::hud_ptr.as<uintptr_t**>();
	static auto find_hud_element = patterns::find_hud_element.as<uintptr_t(__thiscall*)(void*, const char*)>();

	static auto next_update = 0.0f;

	if (round_start || !settings->misc.preserve_killfeed) {
		next_update = interfaces::global_vars->m_realtime + 10.0f;
		return;
	}

	if (next_update > interfaces::global_vars->m_realtime)
		return;

	next_update = interfaces::global_vars->m_realtime + 2.0f;

	const auto deathNotice = std::uintptr_t(find_hud_element(hud_ptr, STRSC("CCSGO_HudDeathNotice")));
	if (!deathNotice)
		return;

	const auto deathNoticePanel = (*(ui_panel_t**)(*reinterpret_cast<std::uintptr_t*>(deathNotice - 20 + 88) + sizeof(std::uintptr_t)));

	const auto childPanelCount = deathNoticePanel->child_count();

	for (int i = 0; i < childPanelCount; ++i) {
		const auto child = deathNoticePanel->child(i);
		if (!child)
			continue;

		if (child->has_class(STRSC("DeathNotice_Killer")))
			child->set_attribute_float(STRSC("SpawnTime"), interfaces::global_vars->m_curtime);
	}
}

void misc_t::buy_items() {
	if (settings->misc.autobuy.enable) {
		std::string buy_str;

		switch (settings->misc.autobuy.main) {
			case 1:
				autobuy::add_weapon(STRS("scar20"), buy_str);
				autobuy::add_weapon(STRS("g3sg1"), buy_str);
				break;
			case 2:
				autobuy::add_weapon(STRS("ssg08"), buy_str);
				break;
			case 3:
				autobuy::add_weapon(STRS("awp"), buy_str);
				break;
			case 4:
				autobuy::add_weapon(STRS("negev"), buy_str);
				break;
			case 5:
				autobuy::add_weapon(STRS("m249"), buy_str);
				break;
			case 6:
				autobuy::add_weapon(STRS("ak47"), buy_str);
				autobuy::add_weapon(STRS("m4a1"), buy_str);
				autobuy::add_weapon(STRS("m4a1_silencer"), buy_str);
				break;
			case 7:
				autobuy::add_weapon(STRS("aug"), buy_str);
				autobuy::add_weapon(STRS("sg556"), buy_str);
				break;
		}

		switch (settings->misc.autobuy.pistol) {
			case 1:
				autobuy::add_weapon(STRS("elite"), buy_str);
				break;
			case 2:
				autobuy::add_weapon(STRS("p250"), buy_str);
				break;
			case 3:
				autobuy::add_weapon(STRS("tec9"), buy_str);
				autobuy::add_weapon(STRS("fn57"), buy_str);
				break;
			case 4:
				autobuy::add_weapon(STRS("deagle"), buy_str);
				autobuy::add_weapon(STRS("revolver"), buy_str);
				break;
		}

		if (settings->misc.autobuy.additional & 1)
			autobuy::add_weapon(STRS("vesthelm"), buy_str);

		if (settings->misc.autobuy.additional & 2)
			autobuy::add_weapon(STRS("vest"), buy_str);

		if (settings->misc.autobuy.additional & 4)
			autobuy::add_weapon(STRS("hegrenade"), buy_str);

		if (settings->misc.autobuy.additional & 8) {
			autobuy::add_weapon(STRS("molotov"), buy_str);
			autobuy::add_weapon(STRS("incgrenade"), buy_str);
		}

		if (settings->misc.autobuy.additional & 16)
			autobuy::add_weapon(STRS("smokegrenade"), buy_str);

		if (settings->misc.autobuy.additional & 32)
			autobuy::add_weapon(STRS("taser"), buy_str);

		if (settings->misc.autobuy.additional & 64)
			autobuy::add_weapon(STRS("defuser"), buy_str);

		interfaces::engine->execute_client_cmd(buy_str.c_str());
	}
}

const char8_t* misc_t::get_weapon_icon(short item_definition_index) {
	/*
		@note: icon code = weapon item definition index in hex
		list of other icons:
		"E210" - kevlar
		"E20E" - helmet
		"E20F" - defuser kit
		"E211" - banner
		"E212" - target
	*/

	switch (item_definition_index) {
		case e_weapon_type::weapon_deagle:
			return u8"\uE001";
		case e_weapon_type::weapon_dualberetta:
			return u8"\uE002";
		case e_weapon_type::weapon_fiveseven:
			return u8"\uE003";
		case e_weapon_type::weapon_glock:
			return u8"\uE004";
		case e_weapon_type::weapon_ak47:
			return u8"\uE007";
		case e_weapon_type::weapon_aug:
			return u8"\uE008";
		case e_weapon_type::weapon_awp:
			return u8"\uE009";
		case e_weapon_type::weapon_famas:
			return u8"\uE00A";
		case e_weapon_type::weapon_g3sg1:
			return u8"\uE00B";
		case e_weapon_type::weapon_galil:
			return u8"\uE00D";
		case e_weapon_type::weapon_m249:
			return u8"\uE00E";
		case e_weapon_type::weapon_m4a1:
			return u8"\uE010";
		case e_weapon_type::weapon_mac10:
			return u8"\uE011";
		case e_weapon_type::weapon_p90:
			return u8"\uE013";
		case e_weapon_type::weapon_mp5sd:
			return u8"\uE017";
		case e_weapon_type::weapon_ump45:
			return u8"\uE018";
		case e_weapon_type::weapon_xm1014:
			return u8"\uE019";
		case e_weapon_type::weapon_bizon:
			return u8"\uE01A";
		case e_weapon_type::weapon_mag7:
			return u8"\uE01B";
		case e_weapon_type::weapon_negev:
			return u8"\uE01C";
		case e_weapon_type::weapon_sawedoff:
			return u8"\uE01D";
		case e_weapon_type::weapon_tec9:
			return u8"\uE01E";
		case e_weapon_type::weapon_zeusx27:
			return u8"\uE01F";
		case e_weapon_type::weapon_usp:
			return u8"\uE020";
		case e_weapon_type::weapon_mp7:
			return u8"\uE021";
		case e_weapon_type::weapon_mp9:
			return u8"\uE022";
		case e_weapon_type::weapon_nova:
			return u8"\uE023";
		case e_weapon_type::weapon_p250:
			return u8"\uE024";
		case e_weapon_type::weapon_scar20:
			return u8"\uE026";
		case e_weapon_type::weapon_sg553:
			return u8"\uE027";
		case e_weapon_type::weapon_ssg08:
			return u8"\uE028";
		case e_weapon_type::weapon_knife:
			return u8"\uE02A";
		case e_weapon_type::weapon_flashbang:
			return u8"\uE02B";
		case e_weapon_type::weapon_hegrenade:
			return u8"\uE02C";
		case e_weapon_type::weapon_smokegrenade:
			return u8"\uE02D";
		case e_weapon_type::weapon_molotov:
		case e_weapon_type::weapon_firebomb:
			return u8"\uE02E";
		case e_weapon_type::weapon_decoy:
		case e_weapon_type::weapon_diversion:
			return u8"\uE02F";
		case e_weapon_type::weapon_inc:
			return u8"\uE030";
		case e_weapon_type::weapon_c4:
			return u8"\uE031";
		case e_weapon_type::weapon_healthshot:
			return u8"\uE039";
		case e_weapon_type::weapon_knife_gg:
		case e_weapon_type::weapon_knife_t:
			return u8"\uE03B";
		case e_weapon_type::weapon_m4a1s:
			return u8"\uE03C";
		case e_weapon_type::weapon_usps:
			return u8"\uE03D";
		case e_weapon_type::weapon_cz75:
			return u8"\uE03F";
		case e_weapon_type::weapon_revolver:
			return u8"\uE040";
		case e_weapon_type::weapon_tagrenade:
			return u8"\uE044";
		case e_weapon_type::weapon_fists:
			return u8"\uE045";
		case e_weapon_type::weapon_tablet:
			return u8"\uE048";
		case e_weapon_type::weapon_melee:
			return u8"\uE04A";
		case e_weapon_type::weapon_axe:
			return u8"\uE04B";
		case e_weapon_type::weapon_hammer:
			return u8"\uE04C";
		case e_weapon_type::weapon_spanner:
			return u8"\uE04E";
		case e_weapon_type::weapon_knife_bayonet:
			return u8"\uE1F4";
		case e_weapon_type::weapon_knife_css:
			return u8"\uE1F7";
		case e_weapon_type::weapon_knife_flip:
			return u8"\uE1F9";
		case e_weapon_type::weapon_knife_gut:
			return u8"\uE1FA";
		case e_weapon_type::weapon_knife_karambit:
			return u8"\uE1FB";
		case e_weapon_type::weapon_knife_m9_bayonet:
			return u8"\uE1FC";
		case e_weapon_type::weapon_knife_tactical:
			return u8"\uE1FD";
		case e_weapon_type::weapon_knife_falchion:
			return u8"\uE200";
		case e_weapon_type::weapon_knife_survival_bowie:
			return u8"\uE202";
		case e_weapon_type::weapon_knife_butterfly:
			return u8"\uE203";
		case e_weapon_type::weapon_knife_push:
			return u8"\uE204";
		case e_weapon_type::weapon_knife_cord:
			return u8"\uE205";
		case e_weapon_type::weapon_knife_canis:
			return u8"\uE206";
		case e_weapon_type::weapon_knife_ursus:
			return u8"\uE207";
		case e_weapon_type::weapon_knife_gypsy_jackknife:
			return u8"\uE208";
		case e_weapon_type::weapon_knife_outdoor:
			return u8"\uE209";
		case e_weapon_type::weapon_knife_stiletto:
			return u8"\uE20A";
		case e_weapon_type::weapon_knife_widowmaker:
			return u8"\uE20B";
		case e_weapon_type::weapon_knife_skeleton:
			return u8"\uE20D";
		default:
			return u8"?";
	}
}

void misc_t::on_create_move() {
}

STFI void thirdperson() {
	if (!hotkeys->thirdperson.m_active || globals->m_local == nullptr) {
		interfaces::input->m_camera_in_third_person = false;
		return;
	}

	if (globals->m_local->is_alive() && interfaces::client_state->m_delta_tick > 0) {
		interfaces::input->m_camera_in_third_person = true;

		auto distance = CVAR_FLOAT("cam_idealdist") /*vars.visuals.thirdperson_dist*/;

		vec3d angles = interfaces::engine->get_view_angles(), inverse_angles = angles;

		inverse_angles.z = distance;

		vec3d forward, right, up;
		math::angle_vectors(inverse_angles, forward, right, up);

		ray_t ray;
		trace_filter_world_and_props_only_t filter;
		trace_t trace;

		auto eye_pos = globals->m_fake_duck
							   ? globals->m_local->render_origin() + interfaces::game_movement->get_player_view_offset(false)
							   : globals->m_local->render_eye_position();

		auto offset = eye_pos + forward * -distance + right + up;

		ray.init(eye_pos, offset, vec3d{ -16.0f, -16.0f, -16.0f }, vec3d{ 16.0f, 16.0f, 16.0f });
		interfaces::traces->trace_ray(ray, MASK_SHOT_HULL, &filter, &trace);

		trace.m_fraction = std::clamp(trace.m_fraction, 0.f, 1.f);
		globals->m_thirdperson_alpha = trace.m_fraction * trace.m_fraction * trace.m_fraction;

		angles.z = distance * trace.m_fraction;

		interfaces::input->m_camera_offset = angles;
	} else {
		interfaces::input->m_camera_in_third_person = false;
		interfaces::input->m_camera_offset.z = 0.f;
	}

	static auto b_once = false;

	if (globals->m_local->is_alive()) {
		b_once = false;
		return;
	}

	if (b_once) {
		globals->m_local->observer_mode() = 5;
		b_once = false;
	}

	if (globals->m_local->observer_mode() == 4)
		b_once = true;
}

void misc_t::on_override_view() {
	thirdperson();
}

void misc_t::enable_hidden_convars() {
	constexpr auto flag_devonly_or_hidden = (1 << 1) | (1 << 4);

	static bool enabled = false;
	static std::vector<con_command_base_t*> hidden_cvars{};

	if (enabled != settings->misc.unlock_cvars) {
		enabled = settings->misc.unlock_cvars;

		if (hidden_cvars.empty()) {
			for (auto cmd = (**interfaces::convar->commands_base())->m_next; cmd != nullptr; cmd = cmd->m_next)
				if (cmd->m_flags.has(XOR32S(flag_devonly_or_hidden)))
					hidden_cvars.push_back(cmd);
		}

		for (auto cmd: hidden_cvars) {
			if (enabled)
				cmd->m_flags.remove(XOR32S(flag_devonly_or_hidden));
			else
				cmd->m_flags.add(XOR32S(flag_devonly_or_hidden));
		}
	}
}

void misc_t::on_render_start() {
	auto mat_postprocess_enable = GET_CVAR("mat_postprocess_enable");
	mat_postprocess_enable->m_fn_change_callbacks.m_size = 0;
	mat_postprocess_enable->set_value(!settings->visuals.removals.at(4));

	if (globals->m_local != nullptr) {
		if (settings->visuals.removals.at(2))
			globals->m_local->flash_amount() = 0.f;

		globals->m_zoom_level = 0;
		if (globals->m_local->is_alive()) {
			auto weapon = sdk::get_local_weapon();
			if (weapon != nullptr && weapon->is_gun())
				globals->m_zoom_level = weapon->zoom_level() * (int)globals->m_local->scoped();
		}
	}
}

csgo_hud_radar_t* misc_t::get_hud_radar() {
	static auto hud_ptr = *patterns::hud_ptr.as<uintptr_t**>();
	static auto find_hud_element = patterns::find_hud_element.as<uintptr_t(__thiscall*)(void*, const char*)>();

	if (hud_ptr == nullptr || find_hud_element == nullptr)
		return nullptr;

	auto radar_base = find_hud_element(hud_ptr, STRC("CCSGO_HudRadar"));
	return (csgo_hud_radar_t*)(radar_base - 0x14);
}

__forceinline base_combat_weapon_t* sdk::get_local_weapon() {
	if (globals->m_local == nullptr)
		return nullptr;

	return (base_combat_weapon_t*)globals->m_local->active_weapon_handle().get();
}

bool misc_t::is_in_chat() {
	if (globals->m_local == nullptr)
		return false;

	static auto hud_ptr = *patterns::hud_ptr.as<uintptr_t**>();
	static auto find_hud_element = patterns::find_hud_element.as<uintptr_t(__thiscall*)(void*, const char*)>();

	if (hud_ptr == nullptr || find_hud_element == nullptr)
		return false;

	auto chat = find_hud_element(hud_ptr, STRC("CCSGO_HudChat"));
	if (chat == 0)
		return false;

	return *(bool*)(chat + XOR32(13));
}