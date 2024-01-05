#pragma once

namespace sdk {
	struct global_vars_t {
		float m_realtime;
		int m_framecount;
		float m_absolute_frametime;
		float m_absolute_frame_start_time_std_dev;
		float m_curtime;
		float m_frametime;
		int m_max_clients;
		int m_tickcount;
		float m_interval_per_tick;
		float m_interpolation_amount;
		int m_simticks_this_frame;
		int m_network_protocol;
		void* m_save_data;
		bool m_client;
		bool m_remote_client;
		int m_time_stamp_networking_base;
		int m_time_stamp_randomize_window;
	};
} // namespace sdk