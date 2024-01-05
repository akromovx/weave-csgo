#pragma once
#include "../deps/json/json.hpp"
#include "utils/color.hpp"
#include "utils/fnva1.hpp"
#include "utils/obfuscation.hpp"
#include <map>
#include <memory>
#include <string>

namespace utils {
	using bytes_t = std::vector<uint8_t>;
}

using json_t = nlohmann::json;

class c_imgui_color {
private:
	float m_value[4] = { 1.f, 1.f, 1.f, 1.f };

public:
	__forceinline c_imgui_color(uint8_t r = 255, uint8_t g = 255, uint8_t b = 255, uint8_t a = 255) {
		m_value[0] = r / (float)XOR32S(255);
		m_value[1] = g / (float)XOR32S(255);
		m_value[2] = b / (float)XOR32S(255);
		m_value[3] = a / (float)XOR32S(255);
	}

	__forceinline uint8_t r() const { return uint8_t(m_value[0] * XOR32(255)); }
	__forceinline uint8_t g() const { return uint8_t(m_value[1] * XOR32(255)); }
	__forceinline uint8_t b() const { return uint8_t(m_value[2] * XOR32(255)); }
	__forceinline uint8_t a() const { return uint8_t(m_value[3] * XOR32(255)); }

	__forceinline float* base() { return m_value; }

	__forceinline color_t get() const {
		return { uint8_t(m_value[0] * XOR32(255)), uint8_t(m_value[1] * XOR32(255)), uint8_t(m_value[2] * XOR32(255)), uint8_t(m_value[3] * XOR32(255)) };
	}

	__forceinline c_imgui_color& operator=(const color_t& clr) {
		m_value[0] = clr.r() / (float)XOR32S(255);
		m_value[1] = clr.g() / (float)XOR32S(255);
		m_value[2] = clr.b() / (float)XOR32S(255);
		m_value[3] = clr.a() / (float)XOR32S(255);

		return *this;
	}
};

namespace incheat_vars {
	enum class e_chams : int {
		chams_vis,
		chams_xqz,
		chams_local,
		chams_desync,
		chams_backtrack,
		chams_shot,
		chams_arms,
		chams_weapon,
		chams_attachments,
		chams_max,
	};

	enum {
		rage_group_default,
		rage_group_snipers,
		rage_group_autosnipers,
		rage_group_heavy_pistols,
		rage_group_pistols,
		rage_group_rifles,
		rage_group_heavies,
		rage_group_shotguns,
		rage_group_smgs,
		rage_group_max
	};

	namespace types {
		using name_t = std::string;

#define _VAR_GET_NAME(x) \
	std::string { x }

#define _VAR_GET_NAME_RT(x) \
	std::string { x }
		template<typename type_t>
		class list_t : public std::unordered_map<type_t*, name_t> {};

		using bool_t = bool;
		using int_t = int32_t;
		using flags_t = ::flags_t;
		using color_t = ::c_imgui_color;
	} // namespace types

	struct glow_esp_settings_t {
		types::color_t color{};
		types::bool_t enable{};

		__forceinline glow_esp_settings_t() : color({}), enable(false) {}
		__forceinline glow_esp_settings_t(types::color_t color, types::bool_t enable) : color(color), enable(enable) {}
	};

#define _VAR_DECL_(type, name) type name{};

#pragma pack(push, 1)
	struct ragebot_settings_t {
		_VAR_DECL_(types::bool_t, override_default);
		_VAR_DECL_(types::bool_t, quick_stop);
		_VAR_DECL_(types::flags_t, quick_stop_options);
		_VAR_DECL_(types::bool_t, visible_only);

		_VAR_DECL_(types::int_t, mindamage);
		_VAR_DECL_(types::int_t, mindamage_override);
		_VAR_DECL_(types::int_t, hitchance);
		_VAR_DECL_(types::bool_t, strict_hitchance);
		_VAR_DECL_(types::flags_t, hitboxes);
		_VAR_DECL_(types::flags_t, force_safepoint);
		_VAR_DECL_(types::bool_t, prefer_safepoint);

		_VAR_DECL_(types::bool_t, autoscope);
		_VAR_DECL_(types::int_t, head_scale);
		_VAR_DECL_(types::int_t, body_scale);

		_VAR_DECL_(types::int_t, priority_hitgroup);
	};

	struct antiaim_settings_t {
		_VAR_DECL_(types::bool_t, override_default);
		_VAR_DECL_(types::bool_t, force_off);
		_VAR_DECL_(types::int_t, pitch);
		_VAR_DECL_(types::bool_t, ignore_freestand);
		_VAR_DECL_(types::bool_t, ignore_manuals);
		_VAR_DECL_(types::int_t, yaw_add);
		_VAR_DECL_(types::bool_t, spin);
		_VAR_DECL_(types::int_t, spin_speed);
		_VAR_DECL_(types::int_t, spin_range);
		_VAR_DECL_(types::bool_t, edge_yaw_on_peek);
		_VAR_DECL_(types::int_t, jitter_angle);
		_VAR_DECL_(types::bool_t, align_desync);
		_VAR_DECL_(types::bool_t, randomize_jitter);
		_VAR_DECL_(types::int_t, desync_amount);
		_VAR_DECL_(types::bool_t, desync_jitter);
		_VAR_DECL_(types::int_t, desync_direction);
		_VAR_DECL_(types::int_t, align_by_target);

		// NOTE: the struct below has nothing to do with config
		struct {
			int last_update{};
			bool jitter_switch{};
			float spin_angle{};
		} temp;
	};

	struct chams_settings_t {
		_VAR_DECL_(types::bool_t, enable);
		_VAR_DECL_(types::int_t, material);
		_VAR_DECL_(types::color_t, color);

		_VAR_DECL_(types::color_t, metallic_color);
		_VAR_DECL_(types::int_t, phong_amount);
		_VAR_DECL_(types::int_t, rim_amount);

		_VAR_DECL_(types::flags_t, overlays);
		_VAR_DECL_(types::color_t, overlays_colors[5]);
	};

	namespace esp {
		template<typename control_t = types::bool_t>
		struct element_t {
			_VAR_DECL_(control_t, value);
			_VAR_DECL_(types::color_t, colors[2]);
			_VAR_DECL_(types::int_t, position);
		};

		struct text_t : public element_t<types::bool_t> {
			types::int_t font{};
		};
		struct text_array_t : public element_t<types::flags_t> {
			types::int_t font{};
		};
		using bar_t = element_t<types::bool_t>;
	} // namespace esp

	class settings_t {
	private:
		types::list_t<types::bool_t> m_bools{};
		types::list_t<types::int_t> m_ints{};
		types::list_t<types::color_t> m_colors{};
		types::list_t<types::flags_t> m_flags{};

		template<typename type_t>
		__forceinline type_t* find_in_hashtable(types::name_t name, types::list_t<type_t>& map) {
			auto it = std::find_if(map.begin(), map.end(), [=](std::pair<type_t*, types::name_t> p) {
				return name == p.second;
			});

			if (it != map.end())
				return it->first;

			return nullptr;
		}

		void init_ragebot();
		void init_antiaim();
		void init_visuals();
		void init_misc();
		void init_skins();

	public:
		__forceinline auto& get_bools() { return m_bools; }
		__forceinline auto& get_ints() { return m_ints; }
		__forceinline auto& get_flags() { return m_flags; }
		__forceinline auto& get_colors() { return m_colors; }

#ifdef _DEBUG
		types::bool_t multithreading{ true };
#endif

		uint8_t begin{};

		struct {
			_VAR_DECL_(types::bool_t, enable);
			_VAR_DECL_(types::bool_t, autofire);
			_VAR_DECL_(types::int_t, computing_limit);
			_VAR_DECL_(types::bool_t, silent);

			struct {
				_VAR_DECL_(types::bool_t, override_default);
				_VAR_DECL_(ragebot_settings_t, settings[10]);

				_VAR_DECL_(types::int_t, tab);
			} weapons[rage_group_max];

		} ragebot;

		struct {
			_VAR_DECL_(types::flags_t, defensive_flags);
			_VAR_DECL_(types::bool_t, immediate_teleport)
		} exploits;

		struct {
			_VAR_DECL_(types::bool_t, enable);

			_VAR_DECL_(antiaim_settings_t, triggers[7]); // default, stand, move, jump, slowwalk, crouch
			_VAR_DECL_(types::int_t, fakelag);
		} antiaim;

		struct {
			_VAR_DECL_(types::bool_t, enable);
			_VAR_DECL_(types::bool_t, box);
			_VAR_DECL_(types::color_t, box_color);

			_VAR_DECL_(esp::text_t, name);
			_VAR_DECL_(esp::bar_t, health);
			_VAR_DECL_(types::bool_t, override_health_color);
			_VAR_DECL_(esp::bar_t, ammo);
			_VAR_DECL_(esp::text_array_t, weapon);
			_VAR_DECL_(esp::text_array_t, flags);

			_VAR_DECL_(chams_settings_t, chams[(int)e_chams::chams_max]);
			_VAR_DECL_(types::bool_t, shot_chams_last_only);

			_VAR_DECL_(glow_esp_settings_t, glow);
			_VAR_DECL_(glow_esp_settings_t, local_glow);

			_VAR_DECL_(types::bool_t, local_blend_in_scope);
			_VAR_DECL_(types::int_t, local_blend_in_scope_amount);
		} player_esp;

		struct {
			_VAR_DECL_(types::bool_t, enable);
			_VAR_DECL_(types::bool_t, box);
			_VAR_DECL_(types::color_t, box_color);
			_VAR_DECL_(esp::text_array_t, name);
			_VAR_DECL_(esp::bar_t, ammo);

			_VAR_DECL_(glow_esp_settings_t, glow);
		} weapon_esp;

		struct {
			_VAR_DECL_(types::bool_t, enable);
			_VAR_DECL_(types::bool_t, prediction);
		} grenade_esp;

		struct {
			_VAR_DECL_(types::bool_t, enable);
		} bomb_esp;

		struct {
			_VAR_DECL_(types::int_t, tracer);
			_VAR_DECL_(types::color_t, tracer_color);

			_VAR_DECL_(types::bool_t, server_impacts);
			_VAR_DECL_(types::bool_t, client_impacts);

			_VAR_DECL_(types::int_t, impacts_size);

			_VAR_DECL_(types::color_t, server_impact_colors[2]);
			_VAR_DECL_(types::color_t, client_impact_colors[2]);
		} bullets;

		struct {
			_VAR_DECL_(types::flags_t, removals);
			_VAR_DECL_(types::flags_t, hitmarker);

			_VAR_DECL_(types::int_t, scope_gap);
			_VAR_DECL_(types::int_t, scope_thickness);
			_VAR_DECL_(types::int_t, scope_size);

			_VAR_DECL_(types::int_t, override_scope);
			_VAR_DECL_(types::color_t, scope_color[2]);

			_VAR_DECL_(types::int_t, viewmodel_fov);
			_VAR_DECL_(types::int_t, world_fov);
			_VAR_DECL_(types::int_t, zoom_fov);

			struct {
				_VAR_DECL_(types::bool_t, enable);
				_VAR_DECL_(types::color_t, color);
				_VAR_DECL_(types::color_t, prop_color);
				_VAR_DECL_(types::color_t, skybox_color);
			} nightmode;
		} visuals;

		struct {
			_VAR_DECL_(types::bool_t, unlock_inventory);
			_VAR_DECL_(types::bool_t, unlock_cvars);
			_VAR_DECL_(types::bool_t, hotkeys_list);
			_VAR_DECL_(types::bool_t, hitsound);
			_VAR_DECL_(types::int_t, hitsound_volume);
			_VAR_DECL_(types::flags_t, log_filter);
			_VAR_DECL_(types::bool_t, preserve_killfeed)
			_VAR_DECL_(types::bool_t, knife_bot)

			struct {
				_VAR_DECL_(types::bool_t, enable);
				_VAR_DECL_(types::int_t, main);
				_VAR_DECL_(types::int_t, pistol);
				_VAR_DECL_(types::flags_t, additional);
			} autobuy;
		} misc;

		struct {
			_VAR_DECL_(types::bool_t, bhop);
			_VAR_DECL_(types::bool_t, autostrafe);
			_VAR_DECL_(types::int_t, strafe_smooth);
			_VAR_DECL_(types::bool_t, fast_stop);
			_VAR_DECL_(types::int_t, leg_movement);

			_VAR_DECL_(types::bool_t, peek_assist_retreat_on_key);
			_VAR_DECL_(types::color_t, peek_assist_colors[2]);
		} movement;

		struct {
			struct {
				_VAR_DECL_(types::int_t, item_definition_index);
				_VAR_DECL_(types::int_t, fallback_paint_kit);
				_VAR_DECL_(types::int_t, fallback_seed);
				_VAR_DECL_(types::int_t, fallback_stattrak);
				_VAR_DECL_(types::int_t, entity_quality);
				_VAR_DECL_(types::int_t, fallback_wear);
			} items[512];

			_VAR_DECL_(types::int_t, knife_model);
			_VAR_DECL_(types::int_t, agent_t);
			_VAR_DECL_(types::int_t, agent_ct);
		} skins;

		uint8_t end{};

		types::int_t dpi_scale = 1;

		__forceinline types::bool_t* find_bool(types::name_t name) { return find_in_hashtable(name, m_bools); }
		__forceinline types::flags_t* find_flag(types::name_t name) { return find_in_hashtable(name, m_flags); }
		__forceinline types::color_t* find_color(types::name_t name) { return find_in_hashtable(name, m_colors); }
		__forceinline types::int_t* find_int(types::name_t name) { return find_in_hashtable(name, m_ints); }

		void init();

		static json_t dump_settings_addr();
		static void save(std::string_view id);
		static void load(std::string_view id);
	};
#pragma pack(pop)
} // namespace incheat_vars

using user_settings = incheat_vars::settings_t;

// global class that contains all user settings
inline auto settings = std::make_shared<incheat_vars::settings_t>();
inline auto default_settings = std::make_shared<incheat_vars::settings_t>();

#define BASE_ADDRESS (ptrdiff_t)(&settings->begin)