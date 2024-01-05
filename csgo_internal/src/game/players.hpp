#pragma once
#include "../../sdk/entities.hpp"
#include "../../sdk/input.hpp"
#include "../../sdk/model_render.hpp"
#include "../../sdk/net_channel.hpp"
#include "../features/visuals/esp_utils.hpp"
#include "../utils/ofs_vector.hpp"
#include "../utils/vector.hpp"
#include <functional>
#include <mutex>

enum e_animation_side {
	as_right = -1,
	as_zero = 0,
	as_left = 1,
	as_main = 2
};

struct player_entry_t;

inline constexpr auto JITTER_CACHE_SIZE = 4;
inline constexpr auto JITTER_BEGIN_ANGLE = 6.f;

enum e_resolver_mode_offset {
	base_mode = 0,
	base_mode_id = 8,
	roll = 16,
	additional = 24,
};

enum e_resolver_mode {
	animation_layers = 0,
	edge = 1,
	off = 2,
	jumping = 3,
	jitter = 4,
	rmode_any,
	rmode_max
};

enum e_resolver_roll {
	enable = 1,
	history = 2,
	inverse = 4,
};

struct resolver_info_t {
	struct {
		uint32_t m_value = 0u;

		__forceinline void reset() {
			m_value = 0u;
		}

		__forceinline void set(int base_mode, uint8_t ID = 0) {
			m_value |= (base_mode << e_resolver_mode_offset::base_mode) | (ID << e_resolver_mode_offset::base_mode_id);
		}

		__forceinline void set_id(uint8_t ID) {
			m_value |= ID << e_resolver_mode_offset::base_mode_id;
		}

		__forceinline void set_roll_info(uint8_t info) {
			m_value |= info << e_resolver_mode_offset::roll;
		}

		__forceinline void set_additional_info(uint8_t info) {
			m_value |= info << e_resolver_mode_offset::additional;
		}

	} m_mode;

	int m_side = 0;		 // the side that will applied on this tick
	int m_last_side = 0; // last applied side

	float m_desync = 0.f; // current desync angle amount
	float m_old_feet_cycle = 0.f;
	float m_cycle_change_time = 0.f;

	bool m_using_micro_movement = false; // is player using sidemoves
	int m_zero_pitch_ticks = 0;			 // ticks were been with pitch < 45.f

	int m_ground_ticks = 0; // ticks player is being on the ground

	bool m_using_sideways = 0; // was player used sideways AA in this tick
	int m_missed_shots[rmode_max];

	struct {
		int m_static_ticks = 0; // ticks without switching angle more than JITTER_BEGIN_ANGLE
		int m_switch_count = 0; // ticks in a row angle switching more than JITTER_BEGIN_ANGLE
		float m_deltas[8]{};	// cached eye angle deltas
		int m_deltas_offset = 0;
		bool m_should_fix{};

		void reset() {
			m_static_ticks = 0;
			m_switch_count = 0;
			m_deltas_offset = 0;
			m_should_fix = false;
		}
	} m_jitter; // that struct contains information about jitter

	enum class e_roll_mode : int {
		absolute = 0,
		relative,
	};

	struct {
		bool m_enabled = false;
		e_roll_mode m_mode = e_roll_mode::absolute;
		float m_angle = 0.f;

		inline void reset() {
			m_enabled = false;
			m_mode = e_roll_mode::absolute;
			m_angle = 0.f;
		}
	} m_roll;

	// fully resets info
	inline void reset() {
		m_side = 0;
		m_last_side = 0;
		m_desync = 0.f;
		m_old_feet_cycle = 0.f;
		m_cycle_change_time = 0.f;
		m_using_micro_movement = false;
		m_zero_pitch_ticks = 0;
		m_ground_ticks = 0;
		m_jitter.reset();
		m_roll.reset();
		m_mode.reset();
		std::memset(m_missed_shots, 0, sizeof(m_missed_shots));
	}
};

struct lag_record_t {
	player_entry_t* m_entry{};
	sdk::cs_player_t* m_player{};
	int m_index{};

	lag_record_t* m_previous{};
	lag_record_t* m_next{};

	float m_simulation_time{};
	float m_old_simulation_time{};
	int m_lag{};

	sdk::animation_layers_t m_layers;
	float m_poses[24]{};

	vec3d m_velocity{};
	vec3d m_eye_angles{};
	flags_t m_flags{};
	int m_eflags{};
	float m_duck_amount{};
	float m_lby{};
	float m_thirdperson_recoil{};
	vec3d m_origin{};
	vec3d m_abs_angle{};
	bool m_crouch_jumping{};
	bool m_may_has_fake{};
	bool m_came_from_dormant{};
	bool m_valid{};
	bool m_valid_check{};
	bool m_valid_visual{};

	bool m_on_ground{};
	float m_last_shot_time{};
	bool m_shot{};
	float m_land_time{};
	bool m_land_in_cycle{};
	bool m_landed{};

	float m_animation_start{};
	int m_tick_received{};

	bool m_animating_in_chams{};
	float m_last_received_time{};
	float m_lerp_time{};
	float m_begin_time{};
	float m_received_time{};
	float m_received_curtime{};

	int m_ground_entity{};

	vec3d m_mins{}, m_maxs{};

	matrix3x4_t m_main_bones[sdk::max_bones]{};
	matrix3x4_t m_visual_bones[sdk::max_bones]{};
	matrix3x4_t m_right_bones[sdk::max_bones]{};
	sdk::animation_layers_t m_right_layers{};
	matrix3x4_t m_left_bones[sdk::max_bones]{};
	sdk::animation_layers_t m_left_layers{};
	matrix3x4_t m_zero_bones[sdk::max_bones]{};
	sdk::animation_layers_t m_zero_layers{};

	matrix3x4_t* m_bones{};
	matrix3x4_t* m_inversed_bones{};
	matrix3x4_t* m_unresolved_bones{};

	bool m_got_side{};
	int m_side{};

	float m_collision_change_height{};
	float m_collision_change_time{};

	void store_data(player_entry_t& entry);
	void restore();
	void setup_bones(matrix3x4_t* matrix, uint32_t bone_mask, lag_record_t* previous = nullptr, bool main = false);
	void update_animations();
	void update_animations(int as);
	void clamp_bones();

	void apply(matrix3x4_t* bones);

	__forceinline bool is_nulled() const {
		return m_entry == nullptr || m_player == nullptr;
	}

	bool is_valid() const;

	lag_record_t() = default;
	~lag_record_t() { CLEAR_THIS(); }

	resolver_info_t m_resolver{};

	struct {
		bool m_primary{};
		bits8_t m_safety{};
	} m_aimbot;

	static bool valid(lag_record_t* record) { return record->is_valid(); }
	static bool vulnerable(lag_record_t* record) { return record->m_shot || record->m_lag <= 1 || record->m_eye_angles.x <= 30.f; }
};

using lag_records_t = utils::ofs_vector_t<lag_record_t>;

struct player_entry_t {
	lag_records_t m_records{};

	sdk::cs_player_t* m_player{};

	lag_record_t* m_previous{};
	lag_record_t* m_prelast_record{};

	lag_record_t* m_last_valid_record{};

	bool m_valid_data{};
	bool m_initialized{ false };
	bool m_last_dormant_tick{};
	float m_last_spawntime{};
	float m_time_came_dormant{};

	int m_choked_ticks{};
	bool m_using_fakelags{};

	float m_next_valid_time{};
	float m_old_simulation_time{};

	matrix3x4_t m_last_bones[sdk::max_bones]{};
	vec3d m_last_origin{};

	bool m_dormant{};

	esp::bounding_box_t m_box{};
	bool m_valid_for_esp{};
	float m_visual_alpha{};
	float m_visual_health{};

	vec3d m_render_origin{};
	vec3d m_view_offset{};
	float m_duck_amount{};

	std::string m_name{};
	int m_health{};
	float m_distance{};
	bool m_zeus_warning{};

	bool m_scoped{};
	bool m_flashed{};
	bool m_has_kit{};
	bool m_has_armor{};
	bool m_has_helmet{};
	bool m_fakeducking{};

	int m_friend_cheat{};
	int m_ping{};

	struct {
		bool m_valid{};
		int m_ammo{};
		int m_max_ammo{};
		bool m_in_reload{};
		float m_reload_ratio{};
		std::string m_name{};
		std::string m_icon{};
	} m_weapon;

	void update_animations();
	void update_box();
	void update_data();

	__forceinline ~player_entry_t() {
		m_records.clear();
	}
};

struct players_t {
private:
	bool m_updating[64]{};
	bool m_setuping[64]{};
	bool m_clamping[64]{};
	std::mutex m_mutex{};

public:
	lag_record_t m_backup_records[64]{};

	struct {
		sdk::animation_state_t* m_real_state = nullptr;
		sdk::animation_state_t* m_fake_state = nullptr;

		sdk::animation_layers_t m_layers{};
		float m_poses[24]{};

		float m_broken_angles{};

		matrix3x4_t m_bones[sdk::max_bones]{};
		matrix3x4_t m_fake_bones[sdk::max_bones]{};
		matrix3x4_t m_interpolated_bones[sdk::max_bones]{};

		float m_last_goal_feet_yaw{};
		float m_duck_amount{};
		float m_desync{};
		float m_head_delta{};

		vec3d m_current_angle{};
		bool m_valid{};

		sdk::e_move_type m_move_type{};
		flags_t m_flags{};
	} m_local_player;

#pragma pack(push, 1)
	struct shared_esp_data_t {
		uint32_t m_id{};
		uint8_t m_user_id{};
		int16_t m_origin_x{};
		int16_t m_origin_y{};
		int16_t m_origin_z{};
		int8_t m_health{};
		uint32_t m_tick{};

		struct user_info_t {
			uint16_t m_id{};
			uint8_t m_crash : 1 {};
			uint8_t m_boss : 1 {};

			user_info_t() { static_assert(sizeof(*this) <= 5); }
		} m_user_info;

		shared_esp_data_t() { static_assert(sizeof(*this) <= 19); }
	};
#pragma pack(pop)

	struct voice_data_received_t {
		players_t::shared_esp_data_t m_data{};
		int m_friend_cheat{};
		float m_time{};

		bool m_verified{};

		bool is_expired() const;

		__forceinline void reset() { CLEAR_THIS(); }
	} m_shared[64];

	void on_render_start();
	void on_net_update_end();
	void on_voice_data_received(sdk::svc_msg_voice_data_t* msg);

	void update_local_player(sdk::user_cmd_t* cmd);
	void on_create_move();
	void on_paint_traverse();

	void for_each_entry(std::function<void(player_entry_t*)> callback);
	player_entry_t* find_entry(sdk::cs_player_t* player);

	__forceinline lag_record_t* get_last_record(sdk::cs_player_t* player) {
		auto entry = find_entry(player);
		if (entry != nullptr && entry->m_initialized) {
			auto& record = entry->m_records.back();
			if (record.m_entry == entry)
				return &record;
		}

		return nullptr;
	}

	__forceinline std::vector<lag_record_t*> find_records_if(player_entry_t* entry, std::function<bool(lag_record_t*)> compare_fn) {
		std::vector<lag_record_t*> ret{};

		if (entry != nullptr && entry->m_initialized) {
			auto& records = entry->m_records;
			for (ptrdiff_t i = records.size() - 1; i >= 0; --i) {
				auto& record = records[i];
				if (compare_fn(&record) && record.m_entry == entry)
					ret.emplace_back(&record);
			}
		}

		return ret;
	}

	__forceinline lag_record_t* find_record_if(player_entry_t* entry, std::function<bool(lag_record_t*)> compare_fn) {
		if (entry != nullptr && entry->m_initialized) {
			auto& records = entry->m_records;
			for (ptrdiff_t i = records.size() - 1; i >= 0; --i) {
				auto& record = records[i];
				if (compare_fn(&record) && record.m_entry == entry)
					return &record;
			}
		}

		return nullptr;
	}

	__forceinline lag_record_t* rfind_record_if(player_entry_t* entry, std::function<bool(lag_record_t*)> compare_fn) {
		if (entry != nullptr && entry->m_initialized) {
			auto& records = entry->m_records;
			for (size_t i = 0; i < records.size(); ++i) {
				auto& record = records[i];
				if (compare_fn(&record) && record.m_entry == entry)
					return &record;
			}
		}

		return nullptr;
	}

	__forceinline std::pair<lag_record_t*, lag_record_t*> find_interp_records(player_entry_t* entry, lag_record_t*& old_record) {
		auto& records = entry->m_records;
		if (entry == nullptr || !entry->m_initialized)
			return { nullptr, nullptr };

		for (ptrdiff_t last_index = records.size() - 1, i = last_index; i >= 0; --i) {
			auto &record = records[i], &prev_record = records[i + 1];

			if (record.m_player == nullptr || prev_record.m_player == nullptr || record.m_entry != entry || prev_record.m_entry != entry)
				continue;

			if (record.m_animating_in_chams || (prev_record.m_valid_visual && !record.m_valid_visual)) {
				/*if (prev_record.m_previous != nullptr)
					old_record = prev_record.m_previous;*/

				return { &prev_record, &record };
			}
		}

		return { nullptr, nullptr };
	}

	__forceinline lag_record_t* find_record_if(sdk::cs_player_t* player, std::function<bool(lag_record_t*)> compare_fn) {
		return find_record_if(find_entry(player), compare_fn);
	}

	__forceinline lag_record_t* rfind_record_if(sdk::cs_player_t* player, std::function<bool(lag_record_t*)> compare_fn) {
		return rfind_record_if(find_entry(player), compare_fn);
	}

	__forceinline bool is_exist(sdk::cs_player_t* player) {
		return find_entry(player) != nullptr;
	}

	__forceinline bool& is_setuping(sdk::cs_player_t* player) { return m_setuping[player->index()]; }
	__forceinline bool& is_clamping(sdk::cs_player_t* player) { return m_clamping[player->index()]; }
	__forceinline bool& is_updating(sdk::cs_player_t* player) { return m_updating[player->index()]; }
};

struct resolver_context_t {
	sdk::cs_player_t* m_player{};
	lag_record_t *m_record{}, *m_previous{};
	int m_index{};
	float m_angle{};
	float m_desync{};
	resolver_info_t* m_info{};
	bool m_inverse_side{};
	float m_speed{};
};

struct resolver_t {
private:
	std::vector<resolver_info_t> m_hit_info[64]{};
	int m_freestand_side[64]{};

	float get_left_yaw(sdk::cs_player_t*);
	float get_right_yaw(sdk::cs_player_t*);
	void check_inverse_side(resolver_context_t&);
	bool can_use_animations(resolver_context_t&);
	bool static_jitter(resolver_context_t&);
	bool jitter_fix(resolver_context_t&);
	bool air_fix(resolver_context_t&);
	void apply_edge(resolver_context_t&);
	void apply_bruteforce(resolver_context_t&);
	void apply(resolver_info_t*);

public:
	resolver_info_t m_info[64]{};

	__forceinline void add_hit_info(sdk::cs_player_t* player, resolver_info_t* info) {
		const int& idx = player->index();
		auto new_info = &m_hit_info[idx].emplace_back();

		memcpy(new_info, info, sizeof(resolver_info_t));

		while (m_hit_info[idx].size() > 5)
			m_hit_info[idx].erase(m_hit_info[idx].begin());
	};

	__forceinline void remove_hit_info(sdk::cs_player_t* player) {
		const int& idx = player->index();
		if (m_hit_info[idx].empty())
			return;

		m_hit_info[idx].erase(m_hit_info[idx].begin());
	}

	__forceinline void clear_hit_info() {
		for (auto& info: m_hit_info)
			info.clear();
	}

	__forceinline void clear_hit_info(int idx) {
		if (idx >= 64 || idx < 0)
			return;

		m_hit_info[idx].clear();
	}

	float get_angle(sdk::cs_player_t*);
	float get_forward_yaw(sdk::cs_player_t*);
	float get_away_angle(sdk::cs_player_t*);
	float get_backward_yaw(sdk::cs_player_t*);
	bool run(lag_record_t*, lag_record_t*);
	void apply_resolver_info(lag_record_t*);
	void force_off(sdk::cs_player_t*);
	void reset(int);

	__forceinline void reset_all() {
		for (auto& info: m_info)
			info.reset();
	}

	void on_create_move();
};

GLOBAL_DYNPTR(resolver_t, resolver);
GLOBAL_DYNPTR(players_t, players);