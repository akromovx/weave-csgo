#pragma once
#include "../src/utils/utils.hpp"
#include "client_class.hpp"
#include "../src/utils/displacement.hpp"
#include "../src/utils/matrix.hpp"

namespace sdk {
	struct view_setup_t {
		int x, x_old;
		int y, y_old;
		int m_width;
		int m_width_old;
		int m_height;
		int m_height_old;
		bool m_ortho;
		float m_ortho_left;
		float m_ortho_top;
		float m_ortho_right;
		float m_ortho_bottom;
		bool m_b_custom_view_matrix;
		matrix3x4_t m_custom_view_matrix;
		char pad_0x68[0x48];
		float m_fov;
		float m_fov_viewmodel;
		vec3d m_origin;
		vec3d m_angles;
		float m_near;
		float m_far;
		float m_near_viewmodel;
		float m_far_viewmodel;
		float m_aspect_ratio;
		float m_near_blur_depth;
		float m_near_focus_depth;
		float m_far_focus_depth;
		float m_far_blur_depth;
		float m_near_blur_radius;
		float m_far_blur_radius;
		int m_do_f_quality;
		int m_motion_blur_mode;
		float m_shutter_time;
		vec3d m_shutter_open_position;
		vec3d m_shutter_open_angles;
		vec3d m_shutter_close_position;
		vec3d m_shutter_close_angles;
		float m_off_center_top;
		float m_off_center_bottom;
		float m_off_center_left;
		float m_off_center_right;
		int m_edge_blur;
	};

	struct client_t {
		VFUNC(get_client_classes(), client_class_t*(__thiscall*)(decltype(this)), 8);

		VFUNC(write_user_cmd_delta_to_buffer(int slot, void* buf, int from, int to, bool is_new_cmd),
			  bool(__thiscall*)(decltype(this), int, void*, int, int, bool), 24, slot, buf, from, to, is_new_cmd);

		VFUNC(is_chat_opened(), bool(__thiscall*)(decltype(this)), 91);
	};
} // namespace sdk