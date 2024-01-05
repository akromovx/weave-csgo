#pragma once
#include "../../../sdk/input.hpp"
#include "../../../sdk/model_info.hpp"
#include "../../base_includes.hpp"
#include "../../cheat.hpp"
#include "../../game/players.hpp"
#include <deque>
#include <variant>

#define MAX_FAKELAG (CVAR_INT("sv_maxusrcmdprocessticks") - 2)

namespace hvh {
	extern bool is_able_to_shoot(int tickbase = 0);
	extern bool is_shooting(sdk::user_cmd_t* cmd);
	extern sdk::e_hitgroup hitbox_to_hitgroup(sdk::e_hitbox hitbox);
	extern const std::vector<sdk::e_hitbox> get_hitboxes_in_hitgroup(sdk::e_hitbox hitbox);
	extern float lerp_time();

	extern float can_hit_hitbox(sdk::mstudiobbox_t* bbox, const vec3d& start, const vec3d& end, sdk::cs_player_t* player, sdk::e_hitbox hitbox, matrix3x4_t* bones);

	extern bool calculate_hitchance(float* out_chance, float weapon_inaccuracy, lag_record_t* record, const vec3d& eye_position, const vec3d& position, float chance,
									sdk::e_hitbox hitbox, matrix3x4_t* bones, bool optimization = true, bool use_autowall = false);

	extern float calculate_hitchance_fast(lag_record_t* record, const vec3d& eye_position, const vec3d& position, sdk::e_hitbox hitbox, matrix3x4_t* bones);
	extern bool is_baim_hitbox(sdk::e_hitbox id);

	extern void extrapolate_position(sdk::cs_player_t* player, vec3d& origin, vec3d& velocity, flags_t& flags, bool was_onground);
	extern vec3d predict_local_player_origin(const vec3d& origin, const vec3d& velocity, const vec3d& old_velocity, int ticks, bool debug = false);
	extern vec3d predict_player_origin(sdk::cs_player_t* player, int ticks, bool debug = false);

	extern void backup_player(sdk::cs_player_t* player);
	extern void restore_player(sdk::cs_player_t* player);

	extern int get_ticks_to_stop();
	extern int get_ticks_to_shoot();
	extern bool is_between_shots();
	extern float get_standing_accuracy();
	extern int get_ticks_to_standing_accuracy();

	extern bool auto_revolver(sdk::user_cmd_t* cmd);

	struct aimbot_t {
		static constexpr auto max_extra_safe_count = 4;

		struct point_t {
			__forceinline point_t(const vec3d& position, bool center, sdk::e_hitbox hitbox) : m_position(position), m_center(center), m_hitbox(hitbox) {}
			__forceinline point_t() {}
			__forceinline ~point_t(){};

			bool m_valid{};

			vec3d m_position{};
			int m_damage{};
			bool m_center{};
			bool m_lethal{};
			int m_priority{};
			bool m_valid_damage{};
			sdk::e_hitbox m_hitbox{};
			bits8_t m_safe{};
			float m_hitchance{};
			int m_extra_safe{};
		};

		struct data_t {
			sdk::cs_player_t* m_player{};
			lag_record_t* m_record{};
			std::vector<lag_record_t*> m_valid_records{};
			int32_t m_command_number{};
			int32_t m_tickcount{};
			std::optional<point_t> m_best_point{};
			std::vector<point_t> m_points{};
			int m_extra_safe_counter{};
			int m_extra_safe_max{};

			bool m_can_stop{};

			__forceinline data_t(){};

			__forceinline data_t(const data_t&) = delete;
			__forceinline data_t& operator=(const data_t&) = delete;

			__forceinline data_t(data_t&&) = delete;
			__forceinline data_t& operator=(data_t&&) = delete;

			__forceinline void reset() {
				m_player = nullptr;
				m_record = nullptr;
				m_command_number = 0;
				m_tickcount = 0;
				m_best_point = std::nullopt;
				m_extra_safe_counter = 0;

				// idk why it happens but every new tick (except first ever) std::vector's data is invalid
				m_points = std::vector<point_t>{};
			}
		} m_data[64]{};

		struct {
			vec3d m_origin{};
			vec3d m_mins{};
			vec3d m_maxs{};
			vec3d m_eye_angles{};
			matrix3x4_t m_bones[sdk::max_bones]{};
			flags_t m_flags{};
			float m_collision_change_height{};
			float m_collision_change_time{};
		} m_backup_records[64]{};

		struct {
			matrix3x4_t m_bones[max_extra_safe_count][sdk::max_bones]{};
		} m_extra_safe[64]{};

		std::array<int, 64> m_sorted_indices{};

		vec3d m_eye_position{};

		std::optional<vec3d> m_aim_position{};
		std::vector<sdk::cs_player_t*> m_players{};
		std::optional<incheat_vars::ragebot_settings_t> m_settings{};

		sdk::cs_player_t* m_last_target{};

		bool m_revolver_fire{};
		bool m_should_work{};
		int m_data_counter{};
		std::atomic<int> m_total_points_scanned{};
		bool m_autostop_called{};
		bool m_should_predictive_stop{};
		bool m_shot{};

		vec3d m_shoot_position{};
		float m_inaccuracy_delta{};
		float m_last_inaccuracy{};
		int m_ticks_inaccuracy_stop_changing{};

		__forceinline auto& next_data() {
			return m_data[m_data_counter++ & 63];
		}

		std::function<void(lag_record_t*)> m_on_weapon_fire{};
		lag_record_t m_last_fired_record{};

		int get_minimum_damage(int health);
		int correct_damage(int damage);
		vec3d get_shoot_position(std::optional<vec3d> angle = std::nullopt);

		vec3d get_advanced_point(sdk::cs_player_t* player, const vec3d& origin, float angle, float length);
		vec3d get_point(sdk::cs_player_t* player, sdk::e_hitbox hitbox, matrix3x4_t* bones);
		std::vector<sdk::e_hitbox> get_hitboxes_to_scan(bool primary = false);
		void collect_points(lag_record_t* record, float scale, const sdk::model_t* model,
							sdk::studiohdr_t* hdr, sdk::mstudiohitboxset_t* set, matrix3x4_t* bones, sdk::e_hitbox hitbox, std::vector<point_t>& points, bool primary_hitscan = false);

		void collect_points(lag_record_t* record, matrix3x4_t* bones, const std::vector<sdk::e_hitbox>& hitboxes,
							std::vector<point_t>& points, bool primary_hitscan = false);

		void collect_points(sdk::cs_player_t* player, matrix3x4_t* bones, const std::vector<sdk::e_hitbox>& hitboxes,
							std::vector<point_t>& points, bool primary_hitscan = false);

		void scan_point(lag_record_t* record, point_t& point);
		void scan_points(lag_record_t* record, std::vector<point_t>& points);

		void on_pre_move(sdk::user_cmd_t* cmd);
		void on_create_move(sdk::user_cmd_t* cmd);
	};

	struct antiaim_t {
		int m_freestand_side{};
		int m_last_applied_tick{};
		int m_trigger_id{};
		std::optional<incheat_vars::antiaim_settings_t> m_settings{};
		bool can_work(sdk::user_cmd_t* cmd);
		std::optional<float> get_at_target_yaw(bool only_on_screen);
		void setup_settings(incheat_vars::antiaim_settings_t& set, int& side);
		void apply(sdk::user_cmd_t* cmd);
	};

	struct fakelag_t {
		bool m_last_tick_fired{};
		bool m_need_reset_shot{};
		bool m_last_send_packet{};
		void on_create_move(sdk::user_cmd_t* cmd);
	};

	GLOBAL_DYNPTR(aimbot_t, aimbot);
	GLOBAL_DYNPTR(antiaim_t, antiaim);
	GLOBAL_DYNPTR(fakelag_t, fakelag);

	__forceinline void on_pre_move(sdk::user_cmd_t* cmd) {
		aimbot->m_should_work = false;
		aimbot->on_pre_move(cmd);
	}

	__forceinline void on_create_move(sdk::user_cmd_t* cmd) {
		aimbot->on_create_move(cmd);
		fakelag->on_create_move(cmd);
		// antiaim->on_create_move(cmd);
	}
} // namespace hvh