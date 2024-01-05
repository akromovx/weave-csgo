#include "vars.hpp"
#include "cheat.hpp"
#include "features/menu.hpp"
#include "utils/hotkeys.hpp"
#include <Windows.h>
#include <fstream>

STFI utils::bytes_t read_file_bin(std::string path) {
	utils::bytes_t result;

	std::ifstream ifs(path, std::ios::binary | std::ios::ate);
	if (ifs) {
		std::ifstream::pos_type pos = ifs.tellg();
		result.resize((size_t)pos);

		ifs.seekg(0, std::ios::beg);
		ifs.read((char*)result.data(), pos);
	}

	return result;
}

STFI void write_file_bin(std::string path, const utils::bytes_t& bytes) {
	std::ofstream file_out(path, std::ios::binary | std::ios::ate);
	if (file_out.good())
		file_out.write((char*)bytes.data(), bytes.size());

	file_out.close();
}

namespace incheat_vars {
	STFI void apply_settings_from_json(const json_t& json) {
		std::memcpy(&settings->begin, &default_settings->begin, (uintptr_t)&settings->end - (uintptr_t)&settings->begin);

		for (auto var = json[STRS("settings")].begin(); var != json[STRS("settings")].end(); ++var) {
			switch (var->type()) {
				case json_t::value_t::boolean: {
					auto ptr = settings->find_bool(var.key());
					if (ptr != nullptr)
						*ptr = var->get<bool>();
					else
						PUSH_LOG("found unknown bool %s\n", var.key().c_str());
					break;
				}
				case json_t::value_t::number_unsigned:
				case json_t::value_t::number_integer: {
					auto ptr_int = settings->find_int(var.key());
					if (ptr_int != nullptr)
						*ptr_int = var->get<int>();
					else {
						auto ptr_flags = settings->find_flag(var.key());
						if (ptr_flags != nullptr)
							ptr_flags->get() = var->get<uint32_t>();
						else
							PUSH_LOG("found unknown (u)int %s\n", var.key().c_str());
					}
					break;
				}
				case json_t::value_t::object:
					auto color_ptr = settings->find_color(var.key());
					if (color_ptr != nullptr) {
						for (auto clr = var->begin(); clr != var->end(); ++clr) {
							switch (fnva1(clr.key().c_str())) {
								case HASH("r"): color_ptr->get().r() = clr->get<uint8_t>(); break;
								case HASH("g"): color_ptr->get().g() = clr->get<uint8_t>(); break;
								case HASH("b"): color_ptr->get().b() = clr->get<uint8_t>(); break;
								case HASH("a"): color_ptr->get().a() = clr->get<uint8_t>(); break;
							}							   
						}
					} else {
						if (var.key().substr(0, 7) == STRS("hotkey_")) {
							auto hotkey = hotkeys->m_hotkeys[std::atoi(var.key().substr(7).c_str())];

							for (auto var2 = var->begin(); var2 != var->end(); ++var2) {
								switch (fnva1(var2.key().c_str())) {
									case HASH("key"): hotkey->m_key = var2->get<int>(); break;
									case HASH("type"): hotkey->m_type = (hotkey_t::e_type)var2->get<int>(); break;
								}
							}
						} else
							PUSH_LOG("found unknown object %s : \"%s\"\n", var.key().c_str(), var->dump().c_str());
					}
					break;
			}
		}
	}

	utils::bytes_t dump_settings(bool pretty) {
		json_t _json{};

		auto& json = _json[STRS("settings")];

		for (const auto& [value_ptr, name]: settings->get_bools()) {
			bool cmp = false;
			const auto ptr = default_settings->find_bool(name);
			if (ptr != nullptr)
				cmp = *ptr == *value_ptr;

			if (!cmp)
				json[name] = (bool)(*value_ptr);
		}

		for (const auto& [value_ptr, name]: settings->get_ints()) {
			bool cmp = false;
			const auto ptr = default_settings->find_int(name);
			if (ptr != nullptr)
				cmp = *ptr == *value_ptr;

			if (!cmp)
				json[name] = (int)(*value_ptr);
		}

		for (const auto& [value_ptr, name]: settings->get_flags()) {
			bool cmp = false;
			const auto ptr = default_settings->find_flag(name);
			if (ptr != nullptr)
				cmp = ptr->get() == value_ptr->get();

			if (!cmp)
				json[name] = value_ptr->get();
		}

		for (const auto& [value_ptr, name]: settings->get_colors()) {
			bool cmp = false;
			const auto ptr = default_settings->find_color(name);
			if (ptr != nullptr)
				cmp = ptr->get().u32() == value_ptr->get().u32();

			if (!cmp) {
				auto& json_color = json[name];
				json_color[STRS("r")] = value_ptr->get().r();
				json_color[STRS("g")] = value_ptr->get().g();
				json_color[STRS("b")] = value_ptr->get().b();
				json_color[STRS("a")] = value_ptr->get().a();
			}
		}

		for (size_t i = 0; i < hotkeys->m_hotkeys.size(); ++i) {
			auto hotkey = hotkeys->m_hotkeys[i];
			auto& json_hotkey = json[STRS("hotkey_") + std::to_string(i)];
			json_hotkey[STRS("key")] = (int)hotkey->m_key;
			json_hotkey[STRS("type")] = (int)hotkey->m_type;
		}

		const auto& out_string = _json.dump(pretty ? 4 : 0);
		return { out_string.data(), out_string.data() + out_string.length() };
	}

#define __USERNAME__ STRS("admin")

#define __CFG_VERSION__ XOR32S(1)
#define __CFG_PATH__ dformat(STRS("weave\\settings\\{}"), __USERNAME__)

	void settings_t::save(std::string_view id) {
		json_t j{};

		j[STRS("version")] = __CFG_VERSION__;
		j[STRS("dpi_scale")] = settings->dpi_scale;

		auto& hkeys = j[STRS("hotkeys")];

		for (auto h: hotkeys->m_hotkeys) {
			if (h->m_hash == HASH("hotkey-uninitialized") || h->m_hash == 0) {
				//gui::msgbox::add(STRS("Hotkey synchronization error (1)"));
				return;
			}

			if (h->m_key > 0 || h->m_type == hotkey_t::e_type::always_on) {
				hkeys.push_back({ { STRS("key"), h->m_key },
								  { STRS("type"), h->m_type },
								  { STRS("show_in_hotkeys"), (bool)h->m_show_in_binds },
								  { STRS("hash"), h->m_hash } });
			}
		}

		const auto& out_string = j.dump(4);
		utils::bytes_t buffer = { out_string.begin(), out_string.end() };
		write_file_bin(__CFG_PATH__, buffer);
		write_file_bin(STR("weave\\settings\\") + id.data(), dump_settings(true));
	}

	void settings_t::load(std::string_view id) {
		auto buffer = read_file_bin(__CFG_PATH__);
		if (buffer.empty())
			return;
		json_t j = json_t::parse(buffer);

		if (j[STRS("version")] != __CFG_VERSION__)
			return;

		{
			const auto new_dpi_scale = j[STRS("dpi_scale")].get<int>();
			const auto updated = new_dpi_scale != settings->dpi_scale;
			settings->dpi_scale = new_dpi_scale;

			if (updated) {
				switch (settings->dpi_scale) {
					case 0:
						gui::dpi::_scale = (1.f);
						break;
					case 1:
						gui::dpi::_scale = (1.25f);
						break;
					case 2:
						gui::dpi::_scale = (1.5f);
						break;
					case 3:
						gui::dpi::_scale = (1.75f);
						break;
					case 4:
						gui::dpi::_scale = (2.f);
						break;
				}
			}
		}

		for (auto& h: j[STRS("hotkeys")]) {
			auto hash = h[STRS("hash")].get<uint32_t>();
			if (hash == HASH("hotkey-uninitialized") || hash == 0) {
				gui::msgbox::add(STRS("Hotkey load error (2)"));
				return;
			}

			auto hotkey = std::find_if(hotkeys->m_hotkeys.begin(), hotkeys->m_hotkeys.end(), [hash](auto hkey) { return hkey->m_hash == hash; });
			if (hotkey == hotkeys->m_hotkeys.end()) {
				gui::msgbox::add(STRS("Hotkey load error (3)"));
				return;
			}

			(*hotkey)->m_key = h[STRS("key")].get<int>();
			(*hotkey)->m_type = (hotkey_t::e_type)h[STRS("type")].get<int>();

			if (h.contains(STRS("show_in_hotkeys")))
				(*hotkey)->m_show_in_binds = h[STRS("show_in_hotkeys")].get<bool>();
		}

		const auto& setting = read_file_bin(STR("weave\\settings\\") + id.data());
		if (setting.empty())
			return;

		apply_settings_from_json(json_t::parse(setting));
	}

#define VARS_ADD_BOOL(varname, def_value)        \
	m_bools[&varname] = _VAR_GET_NAME(#varname); \
	varname = def_value

#define VARS_ADD_INT(varname, def_value)        \
	m_ints[&varname] = _VAR_GET_NAME(#varname); \
	varname = XOR32S(def_value)

#define VARS_ADD_FLOAT(varname, def_value)        \
	m_floats[&varname] = _VAR_GET_NAME(#varname); \
	varname = def_value

#define VARS_ADD_FLAGS(varname, def_value)       \
	m_flags[&varname] = _VAR_GET_NAME(#varname); \
	varname = def_value

#define VARS_ADD_COLOR(varname, r, g, b, a)       \
	m_colors[&varname] = _VAR_GET_NAME(#varname); \
	varname = { XOR8S(r), XOR8S(g), XOR8S(b), XOR8S(a) }

	void settings_t::init_ragebot() {
		VARS_ADD_BOOL(ragebot.enable, false);
		VARS_ADD_BOOL(ragebot.autofire, true);
		VARS_ADD_BOOL(ragebot.silent, true);
		VARS_ADD_INT(ragebot.computing_limit, 2);

#define ADD_RAGE_WEAPON(group_index, weapon_index)                                               \
	VARS_ADD_BOOL(ragebot.weapons[group_index].settings[weapon_index].override_default, false);  \
	VARS_ADD_BOOL(ragebot.weapons[group_index].settings[weapon_index].quick_stop, false);        \
	VARS_ADD_FLAGS(ragebot.weapons[group_index].settings[weapon_index].quick_stop_options, 0b0); \
	VARS_ADD_BOOL(ragebot.weapons[group_index].settings[weapon_index].visible_only, false);      \
	VARS_ADD_INT(ragebot.weapons[group_index].settings[weapon_index].mindamage, 25);             \
	VARS_ADD_INT(ragebot.weapons[group_index].settings[weapon_index].mindamage_override, 5);     \
	VARS_ADD_INT(ragebot.weapons[group_index].settings[weapon_index].hitchance, 60);             \
	VARS_ADD_BOOL(ragebot.weapons[group_index].settings[weapon_index].strict_hitchance, false);  \
	VARS_ADD_FLAGS(ragebot.weapons[group_index].settings[weapon_index].hitboxes, 0b1);           \
	VARS_ADD_FLAGS(ragebot.weapons[group_index].settings[weapon_index].force_safepoint, 0b0);    \
	VARS_ADD_BOOL(ragebot.weapons[group_index].settings[weapon_index].prefer_safepoint, true);   \
	VARS_ADD_BOOL(ragebot.weapons[group_index].settings[weapon_index].autoscope, true);          \
	VARS_ADD_INT(ragebot.weapons[group_index].settings[weapon_index].head_scale, 0);             \
	VARS_ADD_INT(ragebot.weapons[group_index].settings[weapon_index].body_scale, 0);             \
	VARS_ADD_INT(ragebot.weapons[group_index].settings[weapon_index].priority_hitgroup, 0);

#define ADD_RAGE_GROUP(group_index)                                      \
	VARS_ADD_BOOL(ragebot.weapons[group_index].override_default, false); \
	ADD_RAGE_WEAPON(group_index, 0);                                     \
	ADD_RAGE_WEAPON(group_index, 1);                                     \
	ADD_RAGE_WEAPON(group_index, 2);                                     \
	ADD_RAGE_WEAPON(group_index, 3);                                     \
	ADD_RAGE_WEAPON(group_index, 4);                                     \
	ADD_RAGE_WEAPON(group_index, 5);                                     \
	ADD_RAGE_WEAPON(group_index, 6);                                     \
	ADD_RAGE_WEAPON(group_index, 7);                                     \
	ADD_RAGE_WEAPON(group_index, 8);                                     \
	ADD_RAGE_WEAPON(group_index, 9)

		ADD_RAGE_GROUP(0);
		ADD_RAGE_GROUP(1);
		ADD_RAGE_GROUP(2);
		ADD_RAGE_GROUP(3);
		ADD_RAGE_GROUP(4);
		ADD_RAGE_GROUP(5);
		ADD_RAGE_GROUP(6);
		ADD_RAGE_GROUP(7);
		ADD_RAGE_GROUP(8);
	}

	void settings_t::init_antiaim() {
#define ADD_ANTIAIM_TRIGGER(group_index)                                  \
	VARS_ADD_BOOL(antiaim.triggers[group_index].override_default, false); \
	VARS_ADD_BOOL(antiaim.triggers[group_index].force_off, false);        \
	VARS_ADD_INT(antiaim.triggers[group_index].pitch, 0);                 \
	VARS_ADD_BOOL(antiaim.triggers[group_index].ignore_freestand, false); \
	VARS_ADD_BOOL(antiaim.triggers[group_index].ignore_manuals, false);   \
	VARS_ADD_INT(antiaim.triggers[group_index].yaw_add, 0);               \
	VARS_ADD_BOOL(antiaim.triggers[group_index].spin, false);             \
	VARS_ADD_INT(antiaim.triggers[group_index].spin_speed, 5);            \
	VARS_ADD_INT(antiaim.triggers[group_index].spin_range, 30);           \
	VARS_ADD_BOOL(antiaim.triggers[group_index].edge_yaw_on_peek, false); \
	VARS_ADD_INT(antiaim.triggers[group_index].jitter_angle, 0);          \
	VARS_ADD_BOOL(antiaim.triggers[group_index].align_desync, false);     \
	VARS_ADD_BOOL(antiaim.triggers[group_index].randomize_jitter, false); \
	VARS_ADD_INT(antiaim.triggers[group_index].desync_amount, 100);       \
	VARS_ADD_BOOL(antiaim.triggers[group_index].desync_jitter, false);    \
	VARS_ADD_INT(antiaim.triggers[group_index].desync_direction, 0);      \
	VARS_ADD_INT(antiaim.triggers[group_index].align_by_target, 0);

		ADD_ANTIAIM_TRIGGER(0);
		ADD_ANTIAIM_TRIGGER(1);
		ADD_ANTIAIM_TRIGGER(2);
		ADD_ANTIAIM_TRIGGER(3);
		ADD_ANTIAIM_TRIGGER(4);
		ADD_ANTIAIM_TRIGGER(5);
		ADD_ANTIAIM_TRIGGER(6);

		VARS_ADD_BOOL(antiaim.enable, false);
		VARS_ADD_INT(antiaim.fakelag, 0);

		VARS_ADD_FLAGS(exploits.defensive_flags, 0);
		VARS_ADD_BOOL(exploits.immediate_teleport, 0);
		VARS_ADD_FLAGS(exploits.defensive_flags, 0);
	}

	void settings_t::init_visuals() {
		VARS_ADD_BOOL(player_esp.enable, false);
		VARS_ADD_BOOL(player_esp.box, false);
		VARS_ADD_COLOR(player_esp.box_color, 255, 255, 255, 255);

		VARS_ADD_BOOL(player_esp.name.value, true);
		VARS_ADD_COLOR(player_esp.name.colors[0], 255, 255, 255, 255);
		VARS_ADD_COLOR(player_esp.name.colors[1], 255, 255, 255, 255);
		VARS_ADD_INT(player_esp.name.position, 1);
		VARS_ADD_INT(player_esp.name.font, 0);

		VARS_ADD_BOOL(player_esp.ammo.value, true);
		VARS_ADD_COLOR(player_esp.ammo.colors[0], 255, 106, 193, 255);
		VARS_ADD_COLOR(player_esp.ammo.colors[1], 108, 159, 255, 255);
		VARS_ADD_INT(player_esp.ammo.position, 0);

		VARS_ADD_FLAGS(player_esp.weapon.value, 0b111);
		VARS_ADD_COLOR(player_esp.weapon.colors[0], 255, 255, 255, 255);
		VARS_ADD_COLOR(player_esp.weapon.colors[1], 255, 255, 255, 255);
		VARS_ADD_INT(player_esp.weapon.position, 0);
		VARS_ADD_INT(player_esp.weapon.font, 2);

		VARS_ADD_FLAGS(player_esp.flags.value, 0b1111);
		VARS_ADD_COLOR(player_esp.flags.colors[0], 255, 255, 255, 255);
		VARS_ADD_INT(player_esp.flags.position, 3);
		VARS_ADD_INT(player_esp.flags.font, 2);

		VARS_ADD_BOOL(player_esp.glow.enable, false);
		VARS_ADD_COLOR(player_esp.glow.color, 84, 35, 76, 255);

		VARS_ADD_BOOL(player_esp.local_glow.enable, false);
		VARS_ADD_COLOR(player_esp.local_glow.color, 84, 35, 76, 255);

		VARS_ADD_BOOL(player_esp.health.value, true);
		VARS_ADD_COLOR(player_esp.health.colors[0], 255, 251, 64, 255);
		VARS_ADD_COLOR(player_esp.health.colors[1], 255, 102, 145, 255);
		VARS_ADD_INT(player_esp.health.position, 2);
		VARS_ADD_BOOL(player_esp.override_health_color, false);

#define ADD_CHAMS_SETTINGS(index)                                                   \
	VARS_ADD_BOOL(player_esp.chams[index].enable, false);                           \
	VARS_ADD_INT(player_esp.chams[index].material, 0);                              \
	VARS_ADD_COLOR(player_esp.chams[index].color, 255, 255, 255, 255);              \
	VARS_ADD_COLOR(player_esp.chams[index].metallic_color, 255, 255, 255, 255);     \
	VARS_ADD_INT(player_esp.chams[index].phong_amount, 0);                          \
	VARS_ADD_INT(player_esp.chams[index].rim_amount, 100);                          \
	VARS_ADD_FLAGS(player_esp.chams[index].overlays, 0);                            \
	VARS_ADD_COLOR(player_esp.chams[index].overlays_colors[0], 255, 255, 255, 255); \
	VARS_ADD_COLOR(player_esp.chams[index].overlays_colors[1], 255, 255, 255, 255); \
	VARS_ADD_COLOR(player_esp.chams[index].overlays_colors[2], 255, 255, 255, 255); \
	VARS_ADD_COLOR(player_esp.chams[index].overlays_colors[3], 255, 255, 255, 255); \
	VARS_ADD_COLOR(player_esp.chams[index].overlays_colors[4], 255, 255, 255, 255);

		ADD_CHAMS_SETTINGS(0); // vis
		ADD_CHAMS_SETTINGS(1); // xqz
		ADD_CHAMS_SETTINGS(2); // local
		ADD_CHAMS_SETTINGS(3); // desync
		ADD_CHAMS_SETTINGS(4); // backtrack
		ADD_CHAMS_SETTINGS(5); // shot
		ADD_CHAMS_SETTINGS(6); // arms
		ADD_CHAMS_SETTINGS(7); // weapon
		ADD_CHAMS_SETTINGS(8); // attachments

		VARS_ADD_BOOL(player_esp.shot_chams_last_only, true);
		VARS_ADD_BOOL(player_esp.local_blend_in_scope, false);
		VARS_ADD_INT(player_esp.local_blend_in_scope_amount, 25);

		VARS_ADD_BOOL(weapon_esp.enable, false);
		VARS_ADD_BOOL(weapon_esp.box, false);
		VARS_ADD_COLOR(weapon_esp.box_color, 255, 255, 255, 255);

		VARS_ADD_FLAGS(weapon_esp.name.value, true);
		VARS_ADD_COLOR(weapon_esp.name.colors[0], 255, 255, 255, 255);
		VARS_ADD_INT(weapon_esp.name.position, 1);
		VARS_ADD_INT(weapon_esp.name.font, 2);

		VARS_ADD_BOOL(weapon_esp.ammo.value, true);
		VARS_ADD_COLOR(weapon_esp.ammo.colors[0], 66, 156, 245, 255);
		VARS_ADD_COLOR(weapon_esp.ammo.colors[1], 255, 255, 255, 255);
		VARS_ADD_INT(weapon_esp.ammo.position, 1);

		VARS_ADD_BOOL(weapon_esp.glow.enable, false);
		VARS_ADD_COLOR(weapon_esp.glow.color, 84, 35, 76, 255);

		VARS_ADD_BOOL(grenade_esp.enable, false);
		VARS_ADD_BOOL(grenade_esp.prediction, false);

		VARS_ADD_BOOL(bomb_esp.enable, false);

		VARS_ADD_INT(bullets.tracer, 0);
		VARS_ADD_COLOR(bullets.tracer_color, 255, 255, 255, 255);

		VARS_ADD_BOOL(bullets.client_impacts, false);
		VARS_ADD_BOOL(bullets.server_impacts, false);

		VARS_ADD_INT(bullets.impacts_size, 15);

		VARS_ADD_COLOR(bullets.client_impact_colors[0], 255, 56, 35, 70);
		VARS_ADD_COLOR(bullets.client_impact_colors[1], 255, 255, 255, 17);

		VARS_ADD_COLOR(bullets.server_impact_colors[0], 81, 35, 255, 70);
		VARS_ADD_COLOR(bullets.server_impact_colors[1], 255, 255, 255, 25);

		VARS_ADD_FLAGS(visuals.removals, 0b0);
		VARS_ADD_FLAGS(visuals.hitmarker, 0b0);
		VARS_ADD_INT(visuals.scope_gap, 0);
		VARS_ADD_INT(visuals.scope_thickness, 1);
		VARS_ADD_INT(visuals.scope_size, 150);
		VARS_ADD_INT(visuals.override_scope, 0);

		VARS_ADD_BOOL(visuals.nightmode.enable, 0);
		VARS_ADD_COLOR(visuals.nightmode.color, 70, 70, 70, 255);
		VARS_ADD_COLOR(visuals.nightmode.prop_color, 70, 70, 70, 255);
		VARS_ADD_COLOR(visuals.nightmode.skybox_color, 70, 70, 70, 255);

		VARS_ADD_COLOR(visuals.scope_color[0], 255, 35, 150, 255);
		VARS_ADD_COLOR(visuals.scope_color[1], 255, 255, 255, 0);

		VARS_ADD_INT(visuals.world_fov, 0);
		VARS_ADD_INT(visuals.zoom_fov, 0);
		VARS_ADD_INT(visuals.viewmodel_fov, 0);
	}

	void settings_t::init_misc() {
		std::map<bool*, hash_t> a;
		VARS_ADD_BOOL(movement.bhop, false);
		VARS_ADD_BOOL(movement.autostrafe, false);
		VARS_ADD_INT(movement.strafe_smooth, 10);

		VARS_ADD_BOOL(movement.fast_stop, false);
		VARS_ADD_INT(movement.leg_movement, false);
		VARS_ADD_BOOL(movement.peek_assist_retreat_on_key, false);

		VARS_ADD_COLOR(movement.peek_assist_colors[0], 255, 35, 35, 255);
		VARS_ADD_COLOR(movement.peek_assist_colors[1], 35, 255, 35, 255);

		VARS_ADD_BOOL(misc.unlock_inventory, false);
		VARS_ADD_BOOL(misc.unlock_cvars, false);

		VARS_ADD_BOOL(misc.hotkeys_list, false);
		VARS_ADD_BOOL(misc.hitsound, false);
		VARS_ADD_INT(misc.hitsound_volume, 100);
		VARS_ADD_FLAGS(misc.log_filter, 0b0);
		VARS_ADD_BOOL(misc.preserve_killfeed, 0);
		VARS_ADD_BOOL(misc.knife_bot, 0);

		VARS_ADD_BOOL(misc.autobuy.enable, false);
		VARS_ADD_INT(misc.autobuy.main, 0);
		VARS_ADD_INT(misc.autobuy.pistol, 0);
		VARS_ADD_FLAGS(misc.autobuy.additional, 0);
	}

	void settings_t::init_skins() {
		// #STOPSHITCODING
		int i = 0;
		for (auto& skin: skins.items) {
#ifdef _DEBUG
#define VARS_GET_NAME_(x) (x)
#else
#define VARS_GET_NAME_(x) fnva1(x)
#endif
			m_ints[&skin.fallback_paint_kit] = VARS_GET_NAME_(dformat(STRS("skins.items[{}].fallback_paint_kit"), i).c_str());
			skin.fallback_paint_kit = XOR32S(0);
			m_ints[&skin.fallback_seed] = VARS_GET_NAME_(dformat(STRS("skins.items[{}].fallback_seed"), i).c_str());
			skin.fallback_seed = XOR32S(0);
			m_ints[&skin.fallback_stattrak] = VARS_GET_NAME_(dformat(STRS("skins.items[{}].fallback_stattrak"), i).c_str());
			skin.fallback_stattrak = XOR32S(0);
			m_ints[&skin.entity_quality] = VARS_GET_NAME_(dformat(STRS("skins.items[{}].entity_quality"), i).c_str());
			skin.entity_quality = XOR32S(0);
			m_ints[&skin.fallback_wear] = VARS_GET_NAME_(dformat(STRS("skins.items[{}].fallback_wear"), i).c_str());
			skin.fallback_wear = XOR32S(0);
			++i;
		}

		VARS_ADD_INT(skins.knife_model, 0);
		VARS_ADD_INT(skins.agent_t, 0);
		VARS_ADD_INT(skins.agent_ct, 0);
	}

	void settings_t::init() {
		this->init_ragebot();
		this->init_antiaim();
		this->init_visuals();
		this->init_misc();
		this->init_skins();
	}
} // namespace incheat_vars
