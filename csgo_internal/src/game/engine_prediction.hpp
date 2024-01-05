#pragma once
#include "../base_includes.hpp"
#include "../interfaces.hpp"
#include "../../sdk/prediction.hpp"

class engine_prediction_t {
	sdk::move_data_t m_move_data{};

	float m_interpolation_amount{};

	bool m_cycle_changed{};
	bool m_fix_cycle{};
public:
	bool m_is_predicted{};

	void store_unpredicted_data(sdk::user_cmd_t* cmd);
	void update(sdk::user_cmd_t* cmd = nullptr);
	void begin(sdk::user_cmd_t* cmd);
	void repredict(sdk::user_cmd_t* cmd);
	void end(sdk::user_cmd_t* cmd);

	void on_render_start(bool after);
	void on_run_command(sdk::user_cmd_t* cmd, bool after);
	void on_net_update_postdataupdate_start();

	struct entry_t {
		sdk::user_cmd_t m_cmd{};
		sdk::global_vars_t m_global_vars{};

		int m_sequence{};

		vec3d m_origin{};
		vec3d m_velocity{};
		vec3d m_eye_position{};

		flags_t m_flags{};
		int m_tickbase{};

		float m_spread{};
		float m_inaccuracy{};

		bool m_shot{};

		struct {
			float m_cycle{};
			float m_animtime{};
			int m_animation_parity{};
		} m_viewmodel;
		int m_shift{};

		void store(sdk::user_cmd_t* cmd);
	};

	entry_t m_unpredicted_data{};
	entry_t m_last_data{};
	entry_t m_last_sent_data{};
	entry_t m_entries[sdk::max_input]{};
};

GLOBAL_DYNPTR(engine_prediction_t, engine_prediction);