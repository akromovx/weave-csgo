#include "skin_changer.hpp"
#include "chams.hpp"

#include "../../../sdk/class_id.hpp"

#include "../../globals.hpp"
#include "../../interfaces.hpp"

#include "../../game/players.hpp"

#include "../menu.hpp"
#include "../network.hpp"

#include <fstream>
#include <map>

using namespace sdk;

namespace sdk {
	struct cs_15_item_schema_t;
	struct cs_15_item_system_t;

	template<typename key_t, typename value_t>
	struct node_t {
		int previous_id;
		int next_id;
		void* _unknown_ptr;
		int _unknown;
		key_t key;
		value_t value;
	};

	template<typename key_t, typename value_t>
	struct head_t {
		node_t<key_t, value_t>* memory;
		int allocation_count;
		int grow_size;
		int start_element;
		int next_available;
		int _unknown;
		int last_element;
	};

	struct string_t {
		char* buffer;
		int capacity;
		int grow_size;
		int length;
	};

	struct paint_kit_t {
		int id;
		string_t name;
		string_t description;
		string_t item_name;
		string_t material_name;
		string_t image_inventory;
		string_t normal;
		string_t logo_material;
		bool base_diffuse_override;
		int rarity;
		char _pad[40];
		float wear_remap_min;
		float wear_remap_max;
	};
} // namespace sdk

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

STFI int get_weapon(const std::string& n) {
	switch (fnva1(n.c_str())) {
		case fnva1("ak47"): return e_weapon_type::weapon_ak47;
		case fnva1("aug"): return e_weapon_type::weapon_aug;
		case fnva1("awp"): return e_weapon_type::weapon_awp;
		case fnva1("cz75a"): return e_weapon_type::weapon_cz75;
		case fnva1("deagle"): return e_weapon_type::weapon_deagle;
		case fnva1("elite"): return e_weapon_type::weapon_dualberetta;
		case fnva1("famas"): return e_weapon_type::weapon_famas;
		case fnva1("fiveseven"): return e_weapon_type::weapon_fiveseven;
		case fnva1("g3sg1"): return e_weapon_type::weapon_g3sg1;
		case fnva1("galilar"): return e_weapon_type::weapon_galil;
		case fnva1("glock"): return e_weapon_type::weapon_glock;
		case fnva1("m249"): return e_weapon_type::weapon_m249;
		case fnva1("m4a1_silencer"): return e_weapon_type::weapon_m4a1s;
		case fnva1("m4a1"): return e_weapon_type::weapon_m4a1;
		case fnva1("mac10"): return e_weapon_type::weapon_mac10;
		case fnva1("mag7"): return e_weapon_type::weapon_mag7;
		case fnva1("mp7"): return e_weapon_type::weapon_mp7;
		case fnva1("mp9"): return e_weapon_type::weapon_mp9;
		case fnva1("negev"): return e_weapon_type::weapon_negev;
		case fnva1("nova"): return e_weapon_type::weapon_nova;
		case fnva1("hkp2000"): return e_weapon_type::weapon_usp;
		case fnva1("p250"): return e_weapon_type::weapon_p250;
		case fnva1("p90"): return e_weapon_type::weapon_p90;
		case fnva1("bizon"): return e_weapon_type::weapon_bizon;
		case fnva1("revolver"): return e_weapon_type::weapon_revolver;
		case fnva1("sawedoff"): return e_weapon_type::weapon_sawedoff;
		case fnva1("scar20"): return e_weapon_type::weapon_scar20;
		case fnva1("ssg08"): return e_weapon_type::weapon_ssg08;
		case fnva1("sg556"): return e_weapon_type::weapon_sg553;
		case fnva1("tec9"): return e_weapon_type::weapon_tec9;
		case fnva1("ump45"): return e_weapon_type::weapon_ump45;
		case fnva1("usp_silencer"): return e_weapon_type::weapon_usps;
		case fnva1("xm1014"): return e_weapon_type::weapon_xm1014;
	}

	return -1;
}

void skin_changer_t::parse() {
	std::string items{};

	if (std::ifstream file{ STRS("csgo/scripts/items/items_game_cdn.txt") })
		items = { std::istreambuf_iterator{ file }, {} };

	//auto jskins = network::simple_get(STRS("skins.json"), RESOURCES);

	//if (items.empty() || jskins.empty())
	//	return;

	//const auto& skins_json = json_t::parse(jskins);

	static auto sig_address = patterns::item_schema;

	const auto item_system_offset = *reinterpret_cast<std::int32_t*>(sig_address + 1);
	const auto item_system_fn = reinterpret_cast<cs_15_item_system_t* (*)()>(sig_address + 5 + item_system_offset);
	const auto item_schema = reinterpret_cast<cs_15_item_schema_t*>(std::uintptr_t(item_system_fn()) + sizeof(void*));

	const auto get_paint_kit_definition_offset = *reinterpret_cast<std::int32_t*>(sig_address + 11 + 1);
	const auto get_paint_kit_definition_fn = reinterpret_cast<sdk::paint_kit_t*(__thiscall*)(cs_15_item_schema_t*, int)>(sig_address + 11 + 5 + get_paint_kit_definition_offset);

	const auto start_element_offset = *reinterpret_cast<std::intptr_t*>(std::uintptr_t(get_paint_kit_definition_fn) + 8 + 2);

	const auto head_offset = start_element_offset - 12;
	const auto map_head = reinterpret_cast<head_t<int, sdk::paint_kit_t*>*>(std::uintptr_t(item_schema) + head_offset);

	for (int i{}; i <= map_head->last_element; ++i) {
		const auto& paint_kit = map_head->memory[i].value;
		if (paint_kit->id == 9001 || paint_kit->id >= 10000)
			continue;

		auto name = nnx::encoding::utf16to8(interfaces::localize->find(paint_kit->item_name.buffer + 1));

		std::string weapon_name;
		std::string weapon_icon;

		const auto pos = items.find('_' + std::string{ paint_kit->name.buffer } + '=');
		if (pos != std::string::npos && items.substr(pos + paint_kit->name.length).find('_' + std::string{ paint_kit->name.buffer } + '=') == std::string::npos) {
			const auto wpn_name = items.rfind(STRS("weapon_"), pos);
			if (wpn_name != std::string::npos) {
				weapon_name = items.substr(wpn_name + 7, pos - wpn_name - 7);
			}
		}

		auto paint_kit_name = std::string{ paint_kit->name.buffer };

		//if (!weapon_name.empty())
		//	printf("name: %s, paint_kit: %s, weapon_name: %s\n", name, paint_kit_name.c_str(), weapozn_name.c_str());

		if (!weapon_name.empty()) {
			auto full_name = STRS("weapon_") + weapon_name + "_" + paint_kit_name;
	/*		auto weapon_url = std::string{};
			if (skins_json.contains(full_name))
				weapon_url = skins_json[full_name];*/

			weapon_icon = ""/*weapon_url*/;
		}

		m_paint_kits.emplace_back(paint_kit->id, name, weapon_name, weapon_icon, get_weapon(weapon_name), paint_kit->rarity);

		// printf("%s: %d\n", name.c_str(), paint_kit->rarity);
	}

	m_parsed = true;
}

STFI int get_knife_id() {
	switch (settings->skins.knife_model - 1) {
		case 0: return 500;
		case 1: return 514;
		case 2: return 515;
		case 3: return 512;
		case 4: return 505;
		case 5: return 506;
		case 6: return 509;
		case 7: return 507;
		case 8: return 508;
		case 9: return 516;
		case 10: return 520;
		case 11: return 522;
		case 12: return 519;
		case 13: return 523;
		case 14: return 503;
		case 15: return 525;
		case 16: return 521;
		case 17: return 518;
		case 18: return 517;
	}

	return 42;
}

namespace models {
	struct weapon_info_t {
		constexpr weapon_info_t(std::string model, std::string icon = "", int animindex = -1) : m_model(model), m_icon(icon), m_animindex(animindex) {}
		std::string m_model;
		std::string m_icon;
		int m_animindex;
	};

	const weapon_info_t* get_weapon_info(int defindex) {
		const static std::map<int, weapon_info_t> info = {
			{ e_weapon_type::weapon_knife, { STR("models/weapons/v_knife_default_ct.mdl"), STR("knife_default_ct"), 2 } },
			{ e_weapon_type::weapon_knife_t, { STR("models/weapons/v_knife_default_t.mdl"), STR("knife_t"), 12 } },
			{ e_weapon_type::weapon_knife_bayonet, { STR("models/weapons/v_knife_bayonet.mdl"), STR("bayonet"), 0 } },
			{ e_weapon_type::weapon_knife_flip, { STR("models/weapons/v_knife_flip.mdl"), STR("knife_flip"), 4 } },
			{ e_weapon_type::weapon_knife_gut, { STR("models/weapons/v_knife_gut.mdl"), STR("knife_gut"), 5 } },
			{ e_weapon_type::weapon_knife_karambit, { STR("models/weapons/v_knife_karam.mdl"), STR("knife_karambit"), 7 } },
			{ e_weapon_type::weapon_knife_m9_bayonet, { STR("models/weapons/v_knife_m9_bay.mdl"), STR("knife_m9_bayonet"), 8 } },
			{ e_weapon_type::weapon_knife_tactical, { STR("models/weapons/v_knife_tactical.mdl"), STR("knife_tactical") } },
			{ e_weapon_type::weapon_knife_falchion, { STR("models/weapons/v_knife_falchion_advanced.mdl"), STR("knife_falchion"), 3 } },
			{ e_weapon_type::weapon_knife_survival_bowie, { STR("models/weapons/v_knife_survival_bowie.mdl"), STR("knife_survival_bowie"), 11 } },
			{ e_weapon_type::weapon_knife_butterfly, { STR("models/weapons/v_knife_butterfly.mdl"), STR("knife_butterfly"), 1 } },
			{ e_weapon_type::weapon_knife_push, { STR("models/weapons/v_knife_push.mdl"), STR("knife_push"), 9 } },
			{ e_weapon_type::weapon_knife_ursus, { STR("models/weapons/v_knife_ursus.mdl"), STR("knife_ursus"), 13 } },
			{ e_weapon_type::weapon_knife_gypsy_jackknife, { STR("models/weapons/v_knife_gypsy_jackknife.mdl"), STR("knife_gypsy_jackknife"), 6 } },
			{ e_weapon_type::weapon_knife_stiletto, { STR("models/weapons/v_knife_stiletto.mdl"), STR("knife_stiletto"), 10 } },
			{ e_weapon_type::weapon_knife_widowmaker, { STR("models/weapons/v_knife_widowmaker.mdl"), STR("knife_widowmaker"), 14 } },
			{ e_weapon_type::weapon_knife_outdoor, { STR("models/weapons/v_knife_outdoor.mdl"), STR("knife_outdoor"), 14 } },
			{ e_weapon_type::weapon_knife_canis, { STR("models/weapons/v_knife_canis.mdl"), STR("knife_canis"), 14 } },
			{ e_weapon_type::weapon_knife_cord, { STR("models/weapons/v_knife_cord.mdl"), STR("knife_cord"), 14 } },
			{ e_weapon_type::weapon_knife_skeleton, { STR("models/weapons/v_knife_skeleton.mdl"), STR("knife_skeleton"), 14 } },
			{ e_weapon_type::weapon_knife_css, { STR("models/weapons/v_knife_css.mdl"), STR("knife_css"), 14 } },
			{ e_weapon_type::glove_studded_bloodhound, { STR("models/weapons/v_models/arms/glove_bloodhound/v_glove_bloodhound.mdl") } },
			{ e_weapon_type::glove_t, { STR("models/weapons/v_models/arms/glove_fingerless/v_glove_fingerless.mdl") } },
			{ e_weapon_type::glove_ct, { STR("models/weapons/v_models/arms/glove_hardknuckle/v_glove_hardknuckle.mdl") } },
			{ e_weapon_type::glove_sporty, { STR("models/weapons/v_models/arms/glove_sporty/v_glove_sporty.mdl") } },
			{ e_weapon_type::glove_slick, { STR("models/weapons/v_models/arms/glove_slick/v_glove_slick.mdl") } },
			{ e_weapon_type::glove_leather_handwraps, { STR("models/weapons/v_models/arms/glove_handwrap_leathery/v_glove_handwrap_leathery.mdl") } },
			{ e_weapon_type::glove_motorcycle, { STR("models/weapons/v_models/arms/glove_motorcycle/v_glove_motorcycle.mdl") } },
			{ e_weapon_type::glove_specialist, { STR("models/weapons/v_models/arms/glove_specialist/v_glove_specialist.mdl") } },
			{ e_weapon_type::glove_studded_hydra, { STR("models/weapons/v_models/arms/glove_bloodhound/v_glove_bloodhound_hydra.mdl") } },
		};

		const auto entry = info.find(defindex);
		return entry == end(info) ? nullptr : &entry->second;
	}
} // namespace models

static std::unordered_map<std::string, int> model_indices;
static std::unordered_map<std::string, int> weapon_indices;

struct ret_counted_t {
private:
	volatile long m_ref_count;

public:
	virtual void destructor(char balls_egs) = 0;
	virtual bool on_final_release() = 0;

	void unref() {
		if (InterlockedDecrement(&m_ref_count) == 0 && on_final_release())
			destructor(1);
	}
};

STFI void force_update_skin(base_combat_weapon_t* weapon) {
	*(bool*)((uintptr_t)weapon + 0x3370u) = false;

	static auto penis = netvars->get_offset(HASH("DT_BaseCombatWeapon"), HASH("m_Item"));

	auto& vec0 = *(utils::utl_vector_t<ret_counted_t*>*)((uintptr_t)weapon + penis + 0x14u);
	for (int i = 0; i < vec0.m_size; ++i)
		vec0.m_memory.base()[i] = nullptr;

	vec0.m_size = 0;

	auto& vec1 = *(utils::utl_vector_t<ret_counted_t*>*)((uintptr_t)weapon + 0x9dcu);
	for (int i = 0; i < vec1.m_size; ++i)
		vec1.m_memory.base()[i] = nullptr;

	vec1.m_size = 0;

	auto& vec2 = *(utils::utl_vector_t<ret_counted_t*>*)((uintptr_t)weapon + penis + 0x230u);
	for (int i = 0; i < vec2.m_size; ++i) {
		auto& element = vec2.m_memory.base()[i];
		if (element == nullptr)
			continue;

		element->unref();
		element = nullptr;
	}

	vec2.m_size = 0;

	using fn = void(__thiscall*)(void*, const int);
	const auto networkable = weapon->networkable();
	utils::vfunc<fn>(networkable, 7)(networkable, 0);
	utils::vfunc<fn>(networkable, 5)(networkable, 0);
}

STFI void full_update() {
	if (!globals->m_local_alive)
		return;

	interfaces::client_state->m_delta_tick = -1;

	for (auto weapon: globals->m_local->get_weapons())
		force_update_skin(weapon);

	static auto hud_ptr = *patterns::hud_ptr.as<DWORD**>();
	static auto find_hud_element = patterns::find_hud_element.as<DWORD(__thiscall*)(void*, const char*)>();
	static auto clear_hud_weapon = patterns::clear_hud_weapon.as<int(__thiscall*)(int*, int)>();

	if (auto hud_weapons = (int*)find_hud_element(hud_ptr, STRSC("CCSGO_HudWeaponSelection")) - 0x28)
		for (auto i = 0; i < *(hud_weapons + 0x20); i++)
			i = clear_hud_weapon(hud_weapons, i);
}

STFI void perform_full_update(base_combat_weapon_t* weapon) {
	if (std::abs(clock() - ctx->m_last_update) >= 1000) {
		weapon->pre_data_update(0);
		players->m_local_player.m_valid = false;
		full_update();
		ctx->perform_update();
		esp::chams->clear_hitmatrices();
	}
}

STFI void update_knife(base_combat_weapon_t* weapon) {
	weapon->fallback_wear() = 0.001f;
	weapon->item_id_high() = -1;

	auto& definition_index = weapon->item_definition_index();

	const auto item_def_knifes = get_knife_id();

	if (definition_index != 0 && item_def_knifes != 0 && item_def_knifes != definition_index) {
		weapon->entity_quality() = 3;

		const auto& replacement_item = models::get_weapon_info(item_def_knifes);

		if (!replacement_item)
			return;

		auto old_definition_index = definition_index;
		definition_index = item_def_knifes;

		if (weapon_indices.find(replacement_item->m_model) == weapon_indices.end())
			weapon_indices.emplace(replacement_item->m_model, interfaces::model_info->get_model_index(replacement_item->m_model.c_str()));

		weapon->set_model_index(weapon_indices.at(replacement_item->m_model));

		perform_full_update(weapon);

		/*if (old_definition_index)
			if (auto original_item = sParser::GetWeaponInfo(old_definition_index); original_item && !original_item->icon.empty() && !replacement_item->icon.empty())
				iconOverrides[original_item->icon] = replacement_item->icon.c_str();*/
	}

	auto viewmodel = (base_viewmodel_t*)globals->m_local->viewmodel_handle().get();
	if (viewmodel == nullptr)
		return;

	auto view_model_weapon = (base_combat_weapon_t*)viewmodel->weapon_handle().get();

	if (!view_model_weapon)
		return;

	auto override_info = models::get_weapon_info(view_model_weapon->item_definition_index());

	if (!override_info)
		return;

	if (model_indices.find(override_info->m_model) == model_indices.end())
		model_indices.emplace(override_info->m_model, interfaces::model_info->get_model_index(override_info->m_model.c_str()));

	viewmodel->model_index() = model_indices.at(override_info->m_model);

	auto world_model = (base_combat_weapon_t*)view_model_weapon->world_model().get();
	if (!world_model)
		return;

	world_model->model_index() = model_indices.at(override_info->m_model) + 1;
}

STFI void update_weapon(base_combat_weapon_t* weapon) {
	auto& fallback_paint_kit = weapon->fallback_paintkit();
	auto target_skin = settings->skins.items[weapon->item_definition_index()].fallback_paint_kit;

	if (target_skin != 0 && target_skin != fallback_paint_kit) {
		fallback_paint_kit = target_skin;

		if (menu::open)
			perform_full_update(weapon);
	}

	if (target_skin != 0)
		fallback_paint_kit = target_skin;

	weapon->owner_xuid_low() = 0;
	weapon->owner_xuid_high() = 0;
	weapon->fallback_wear() = 0.001f;
	weapon->item_id_high() = -1;
}

void skin_changer_t::on_net_update_postdataupdate_start() {
	const auto& local_info = globals->m_local->get_player_info();

	for (auto weapon: globals->m_local->get_weapons()) {
		if (weapon->is_knife())
			update_knife(weapon);
		else
			update_weapon(weapon);
	}
}

STFI int random_sequence(const int low, const int high) {
	return math::random_int(low, high);
}

static int fix_animation(const uint32_t model, const int sequence) noexcept {
	enum e_sequence {
		SEQUENCE_DEFAULT_DRAW = 0,
		SEQUENCE_DEFAULT_IDLE1 = 1,
		SEQUENCE_DEFAULT_IDLE2 = 2,
		SEQUENCE_DEFAULT_LIGHT_MISS1 = 3,
		SEQUENCE_DEFAULT_LIGHT_MISS2 = 4,
		SEQUENCE_DEFAULT_HEAVY_MISS1 = 9,
		SEQUENCE_DEFAULT_HEAVY_HIT1 = 10,
		SEQUENCE_DEFAULT_HEAVY_BACKSTAB = 11,
		SEQUENCE_DEFAULT_LOOKAT01 = 12,

		SEQUENCE_BUTTERFLY_DRAW = 0,
		SEQUENCE_BUTTERFLY_DRAW2 = 1,
		SEQUENCE_BUTTERFLY_LOOKAT01 = 13,
		SEQUENCE_BUTTERFLY_LOOKAT03 = 15,

		SEQUENCE_FALCHION_IDLE1 = 1,
		SEQUENCE_FALCHION_HEAVY_MISS1 = 8,
		SEQUENCE_FALCHION_HEAVY_MISS1_NOFLIP = 9,
		SEQUENCE_FALCHION_LOOKAT01 = 12,
		SEQUENCE_FALCHION_LOOKAT02 = 13,

		SEQUENCE_DAGGERS_IDLE1 = 1,
		SEQUENCE_DAGGERS_LIGHT_MISS1 = 2,
		SEQUENCE_DAGGERS_LIGHT_MISS5 = 6,
		SEQUENCE_DAGGERS_HEAVY_MISS2 = 11,
		SEQUENCE_DAGGERS_HEAVY_MISS1 = 12,

		SEQUENCE_BOWIE_IDLE1 = 1,
	};

	switch (model) {
		case HASH("models/weapons/v_knife_butterfly.mdl"): {
			switch (sequence) {
				case SEQUENCE_DEFAULT_DRAW:
					return random_sequence(SEQUENCE_BUTTERFLY_DRAW, SEQUENCE_BUTTERFLY_DRAW2);
				case SEQUENCE_DEFAULT_LOOKAT01:
					return random_sequence(SEQUENCE_BUTTERFLY_LOOKAT01, SEQUENCE_BUTTERFLY_LOOKAT03);
				default:
					return sequence + 1;
			}
		}

		case HASH("models/weapons/v_knife_falchion_advanced.mdl"): {
			switch (sequence) {
				case SEQUENCE_DEFAULT_IDLE2:
					return SEQUENCE_FALCHION_IDLE1;
				case SEQUENCE_DEFAULT_HEAVY_MISS1:
					return random_sequence(SEQUENCE_FALCHION_HEAVY_MISS1, SEQUENCE_FALCHION_HEAVY_MISS1_NOFLIP);
				case SEQUENCE_DEFAULT_LOOKAT01:
					return random_sequence(SEQUENCE_FALCHION_LOOKAT01, SEQUENCE_FALCHION_LOOKAT02);
				case SEQUENCE_DEFAULT_DRAW:
				case SEQUENCE_DEFAULT_IDLE1:
					return sequence;
				default:
					return sequence - 1;
			}
		}

		case HASH("models/weapons/v_knife_push.mdl"): {
			switch (sequence) {
				case SEQUENCE_DEFAULT_IDLE2:
					return SEQUENCE_DAGGERS_IDLE1;
				case SEQUENCE_DEFAULT_LIGHT_MISS1:
				case SEQUENCE_DEFAULT_LIGHT_MISS2:
					return random_sequence(SEQUENCE_DAGGERS_LIGHT_MISS1, SEQUENCE_DAGGERS_LIGHT_MISS5);
				case SEQUENCE_DEFAULT_HEAVY_MISS1:
					return random_sequence(SEQUENCE_DAGGERS_HEAVY_MISS2, SEQUENCE_DAGGERS_HEAVY_MISS1);
				case SEQUENCE_DEFAULT_HEAVY_HIT1:
				case SEQUENCE_DEFAULT_HEAVY_BACKSTAB:
				case SEQUENCE_DEFAULT_LOOKAT01:
					return sequence + 3;
				case SEQUENCE_DEFAULT_DRAW:
				case SEQUENCE_DEFAULT_IDLE1:
					return sequence;
				default:
					return sequence + 2;
			}
		}

		case HASH("models/weapons/v_knife_survival_bowie.mdl"): {
			switch (sequence) {
				case SEQUENCE_DEFAULT_DRAW:
				case SEQUENCE_DEFAULT_IDLE1:
					return sequence;
				case SEQUENCE_DEFAULT_IDLE2:
					return SEQUENCE_BOWIE_IDLE1;
				default:
					return sequence - 1;
			}
		}

		case HASH("models/weapons/v_knife_ursus.mdl"):
		case HASH("models/weapons/v_knife_skeleton.mdl"):
		case HASH("models/weapons/v_knife_outdoor.mdl"):
		case HASH("models/weapons/v_knife_cord.mdl"):
		case HASH("models/weapons/v_knife_canis.mdl"): {
			switch (sequence) {
				case SEQUENCE_DEFAULT_DRAW:
					return random_sequence(SEQUENCE_BUTTERFLY_DRAW, SEQUENCE_BUTTERFLY_DRAW2);
				case SEQUENCE_DEFAULT_LOOKAT01:
					return random_sequence(SEQUENCE_BUTTERFLY_LOOKAT01, 14);
				default:
					return sequence + 1;
			}
		}

		case HASH("models/weapons/v_knife_stiletto.mdl"): {
			switch (sequence) {
				case SEQUENCE_DEFAULT_LOOKAT01:
					return random_sequence(12, 13);
			}
		}

		case HASH("models/weapons/v_knife_widowmaker.mdl"): {
			switch (sequence) {
				case SEQUENCE_DEFAULT_LOOKAT01:
					return random_sequence(14, 15);
			}
		}

		default: return sequence;
	}
}

void skin_changer_t::sequence_remap(recv_proxy_data_t* data, base_viewmodel_t* view_model) {
	if (settings->skins.knife_model == 0) return;

	const auto owner = (cs_player_t*)view_model->owner_handle().get();

	if (globals->m_local != nullptr && owner != nullptr && owner == globals->m_local) {
		auto weapon = globals->m_local->active_weapon();
		if (weapon == nullptr)
			return;

		const auto weapon_info = models::get_weapon_info(weapon->item_definition_index());
		if (weapon_info == nullptr)
			return;

		auto& m_sequence = data->m_value.m_int;
		m_sequence = fix_animation(fnva1(weapon_info->m_model.c_str()), m_sequence);
	}
}

void skin_changer_t::reset() {
	if (!model_indices.empty()) model_indices.clear();
	if (!weapon_indices.empty()) weapon_indices.clear();
}

STFI std::string get_model(int team) {
	static std::vector<std::string> models = {
		STR("models/player/custom_player/legacy/ctm_fbi_variantb.mdl"),
		STR("models/player/custom_player/legacy/ctm_fbi_variantf.mdl"),
		STR("models/player/custom_player/legacy/ctm_fbi_variantg.mdl"),
		STR("models/player/custom_player/legacy/ctm_fbi_varianth.mdl"),
		STR("models/player/custom_player/legacy/ctm_sas_variantf.mdl"),
		STR("models/player/custom_player/legacy/ctm_st6_variante.mdl"),
		STR("models/player/custom_player/legacy/ctm_st6_variantg.mdl"),
		STR("models/player/custom_player/legacy/ctm_st6_varianti.mdl"),
		STR("models/player/custom_player/legacy/ctm_st6_variantk.mdl"),
		STR("models/player/custom_player/legacy/ctm_st6_variantm.mdl"),
		STR("models/player/custom_player/legacy/tm_balkan_variantf.mdl"),
		STR("models/player/custom_player/legacy/tm_balkan_variantg.mdl"),
		STR("models/player/custom_player/legacy/tm_balkan_varianth.mdl"),
		STR("models/player/custom_player/legacy/tm_balkan_varianti.mdl"),
		STR("models/player/custom_player/legacy/tm_balkan_variantj.mdl"),
		STR("models/player/custom_player/legacy/tm_leet_variantf.mdl"),
		STR("models/player/custom_player/legacy/tm_leet_variantg.mdl"),
		STR("models/player/custom_player/legacy/tm_leet_varianth.mdl"),
		STR("models/player/custom_player/legacy/tm_leet_varianti.mdl"),
		STR("models/player/custom_player/legacy/tm_phoenix_variantf.mdl"),
		STR("models/player/custom_player/legacy/tm_phoenix_variantg.mdl"),
		STR("models/player/custom_player/legacy/tm_phoenix_varianth.mdl"),
		STR("models/player/custom_player/legacy/tm_phoenix_varianti.mdl"),
		STR("models/player/custom_player/legacy/ctm_st6_variantj.mdl"),
		STR("models/player/custom_player/legacy/ctm_st6_variantl.mdl"),
		STR("models/player/custom_player/legacy/tm_balkan_variantk.mdl"),
		STR("models/player/custom_player/legacy/tm_balkan_variantl.mdl"),
		STR("models/player/custom_player/legacy/ctm_swat_variante.mdl"),
		STR("models/player/custom_player/legacy/ctm_swat_variantf.mdl"),
		STR("models/player/custom_player/legacy/ctm_swat_variantg.mdl"),
		STR("models/player/custom_player/legacy/ctm_swat_varianth.mdl"),
		STR("models/player/custom_player/legacy/ctm_swat_varianti.mdl"),
		STR("models/player/custom_player/legacy/ctm_swat_variantj.mdl"),
		STR("models/player/custom_player/legacy/tm_professional_varf.mdl"),
		STR("models/player/custom_player/legacy/tm_professional_varf1.mdl"),
		STR("models/player/custom_player/legacy/tm_professional_varf2.mdl"),
		STR("models/player/custom_player/legacy/tm_professional_varf3.mdl"),
		STR("models/player/custom_player/legacy/tm_professional_varf4.mdl"),
		STR("models/player/custom_player/legacy/tm_professional_varg.mdl"),
		STR("models/player/custom_player/legacy/tm_professional_varh.mdl"),
		STR("models/player/custom_player/legacy/tm_professional_vari.mdl"),
		STR("models/player/custom_player/legacy/tm_professional_varj.mdl"),
		STR("models/player/custom_player/legacy/ctm_diver_varianta.mdl"),		  // Cmdr. Davida 'Goggles' Fernandez | SEAL Frogman
		STR("models/player/custom_player/legacy/ctm_diver_variantb.mdl"),		  // Cmdr. Frank 'Wet Sox' Baroud | SEAL Frogman
		STR("models/player/custom_player/legacy/ctm_diver_variantc.mdl"),		  // Lieutenant Rex Krikey | SEAL Frogman
		STR("models/player/custom_player/legacy/ctm_gendarmerie_varianta.mdl"),	  // Sous-Lieutenant Medic | Gendarmerie Nationale
		STR("models/player/custom_player/legacy/ctm_gendarmerie_variantb.mdl"),	  // Chem-Haz Capitaine | Gendarmerie Nationale
		STR("models/player/custom_player/legacy/ctm_gendarmerie_variantc.mdl"),	  // Chef d'Escadron Rouchard | Gendarmerie Nationale
		STR("models/player/custom_player/legacy/ctm_gendarmerie_variantd.mdl"),	  // Aspirant | Gendarmerie Nationale
		STR("models/player/custom_player/legacy/ctm_gendarmerie_variante.mdl"),	  // Officer Jacques Beltram | Gendarmerie Nationale
		STR("models/player/custom_player/legacy/ctm_sas_variantg.mdl"),			  // D Squadron Officer | NZSAS
		STR("models/player/custom_player/legacy/ctm_st6_variantn.mdl"),			  // Primeiro Tenente | Brazilian 1st Battalion
		STR("models/player/custom_player/legacy/ctm_swat_variantk.mdl"),		  // Lieutenant 'Tree Hugger' Farlow | SWAT
		STR("models/player/custom_player/legacy/tm_professional_varf5.mdl"),	  // Bloody Darryl The Strapped | The Professionals
		STR("models/player/custom_player/legacy/tm_leet_variantj.mdl"),			  // Mr. Muhlik | Elite Crew
		STR("models/player/custom_player/legacy/tm_jungle_raider_variantf2.mdl"), // Trapper | Guerrilla Warfare
		STR("models/player/custom_player/legacy/tm_jungle_raider_variantf.mdl"),  // Trapper Aggressor | Guerrilla Warfare
		STR("models/player/custom_player/legacy/tm_jungle_raider_variante.mdl"),  // Vypa Sista of the Revolution | Guerrilla Warfare
		STR("models/player/custom_player/legacy/tm_jungle_raider_variantd.mdl"),  // Col. Mangos Dabisi | Guerrilla Warfare
		STR("models/player/custom_player/legacy/tm_jungle_raider_variantb2.mdl"), // 'Medium Rare' Crasswater | Guerrilla Warfare
		STR("models/player/custom_player/legacy/tm_jungle_raider_variantb.mdl"),  // Crasswater The Forgotten | Guerrilla Warfare
		STR("models/player/custom_player/legacy/tm_jungle_raider_varianta.mdl"),  // Elite Trapper Solman | Guerrilla Warfare
		STR("models/player/custom_player/legacy/tm_pirate.mdl"),
		STR("models/player/custom_player/legacy/tm_pirate_varianta.mdl"),
		STR("models/player/custom_player/legacy/tm_pirate_variantb.mdl"),
		STR("models/player/custom_player/legacy/tm_pirate_variantc.mdl"),
		STR("models/player/custom_player/legacy/tm_pirate_variantd.mdl"),
		STR("models/player/custom_player/legacy/tm_anarchist.mdl"),
		STR("models/player/custom_player/legacy/tm_anarchist_varianta.mdl"),
		STR("models/player/custom_player/legacy/tm_anarchist_variantb.mdl"),
		STR("models/player/custom_player/legacy/tm_anarchist_variantc.mdl"),
		STR("models/player/custom_player/legacy/tm_anarchist_variantd.mdl"),
		STR("models/player/custom_player/legacy/tm_balkan_varianta.mdl"),
		STR("models/player/custom_player/legacy/tm_balkan_variantb.mdl"),
		STR("models/player/custom_player/legacy/tm_balkan_variantc.mdl"),
		STR("models/player/custom_player/legacy/tm_balkan_variantd.mdl"),
		STR("models/player/custom_player/legacy/tm_balkan_variante.mdl"),
		STR("models/player/custom_player/legacy/tm_jumpsuit_varianta.mdl"),
		STR("models/player/custom_player/legacy/tm_jumpsuit_variantb.mdl"),
		STR("models/player/custom_player/legacy/tm_jumpsuit_variantc.mdl"),
	};

	switch (team) {
		case 2: return (size_t)(settings->skins.agent_t - 1) < models.size() ? models[settings->skins.agent_t - 1] : "";
		case 3: return (size_t)(settings->skins.agent_ct - 1) < models.size() ? models[settings->skins.agent_ct - 1] : "";
		default: return "";
	}
};

void skin_changer_t::on_frame_stage_notify(e_client_frame_stage stage) {
	if (stage != e_client_frame_stage::frame_net_update_postdataupdate_start && stage != e_client_frame_stage::frame_render_end)
		return;

	static int original_index = 0;
	//static int original_model = 0;

	if (globals->m_local == nullptr) {
		original_index = 0;
		//original_model = 0;
		return;
	}

	static auto addr = patterns::get_player_viewmodel_arm_config_for_player_model.as<uintptr_t>();
	static std::add_pointer_t<const char** __fastcall(const char*)> get_player_viewmodel_arm_config_for_player_model =
			utils::relative_to_absolute<decltype(get_player_viewmodel_arm_config_for_player_model)>(addr);

	//if (stage == e_client_frame_stage::frame_net_update_postdataupdate_start && original_model == 0) {
	//	if (globals->m_local->is_alive())
	//		original_model = globals->m_local->model_index();
	//}

	const auto& model = get_model(globals->m_local->team());

	if (!model.empty()) {
		if (stage == e_client_frame_stage::frame_net_update_postdataupdate_start) {
			original_index = globals->m_local->model_index();
			if (const auto modelprecache = interfaces::net_string_container->find_table(STRC("modelprecache"))) {
				modelprecache->add_string(false, model.c_str());
				const auto viewmodelArmConfig = get_player_viewmodel_arm_config_for_player_model(model.c_str());
				modelprecache->add_string(false, viewmodelArmConfig[2]);
				modelprecache->add_string(false, viewmodelArmConfig[3]);
			}
		}

		const auto idx = stage == e_client_frame_stage::frame_render_end && original_index ? original_index : interfaces::model_info->get_model_index(model.c_str());
		globals->m_local->set_model_index(idx);

		if (const auto ragdoll = (base_animating_t*)globals->m_local->ragdoll_handle().get())
			ragdoll->set_model_index(idx);
	} else {
		//if (original_model != 0) {
		//	globals->m_local->set_model_index(original_model);

		//	if (const auto ragdoll = (cs_player_t*)globals->m_local->ragdoll_handle().get())
		//		ragdoll->set_model_index(original_model);
		//}
	}
}
