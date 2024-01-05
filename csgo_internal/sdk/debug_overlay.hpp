#pragma once
#include "../src/utils/utils.hpp"
#include "../src/utils/vector.hpp"
#include "../src/utils/color.hpp"

namespace sdk {
	struct overlay_text_t;

	struct debug_overlay_t {
		virtual void add_entity_text_overlay(int ent_index, int line_offset, float duration, int r, int g, int b, int a, _Printf_format_string_ const char* format, ...) = 0;
		virtual void add_box_overlay(const vec3d& origin, const vec3d& mins, const vec3d& max, vec3d const& orientation, int r, int g, int b, int a, float duration) = 0;
		virtual void add_sphere_overlay(const vec3d& vOrigin, float flRadius, int nTheta, int nPhi, int r, int g, int b, int a, float flDuration) = 0;
		virtual void add_triangle_overlay(const vec3d& p1, const vec3d& p2, const vec3d& p3, int r, int g, int b, int a, bool noDepthTest, float duration) = 0;
		virtual void add_line_overlay(const vec3d& origin, const vec3d& dest, int r, int g, int b, bool noDepthTest, float duration) = 0;
		virtual void add_text_overlay(const vec3d& origin, float duration, _Printf_format_string_ const char* format, ...) = 0;
		virtual void add_text_overlay(const vec3d& origin, int line_offset, float duration, _Printf_format_string_ const char* format, ...) = 0;
		virtual void add_screen_text_overlay(float flXPos, float flYPos, float flDuration, int r, int g, int b, int a, const char* text) = 0;
		virtual void add_swept_box_overlay(const vec3d& start, const vec3d& end, const vec3d& mins, const vec3d& max, const vec3d& angles, int r, int g, int b, int a, float flDuration) = 0;
		virtual void add_grid_overlay(const vec3d& origin) = 0;
		virtual void add_coord_frame_overlay(const /*matrix_t&*/ int frame, float flScale, int vColorTable[3][3] = NULL) = 0;
		virtual int screen_position(const vec3d& point, vec3d& screen) = 0;
		virtual int screen_position(float flXPos, float flYPos, vec3d& screen) = 0;
		virtual overlay_text_t* get_first(void) = 0;
		virtual overlay_text_t* get_next(overlay_text_t* current) = 0;
		virtual void clear_dead_overlays(void) = 0;
		virtual void clear_all_overlays() = 0;
		virtual void add_text_overlay_rgb(const vec3d& origin, int line_offset, float duration, float r, float g, float b, float alpha, _Printf_format_string_ const char* format, ...) = 0;
		virtual void add_text_overlay_rgb(const vec3d& origin, int line_offset, float duration, int r, int g, int b, int a, _Printf_format_string_ const char* format, ...) = 0;
		virtual void add_line_overlay_alpha(const vec3d& origin, const vec3d& dest, int r, int g, int b, int a, bool noDepthTest, float duration) = 0;
		virtual void add_box_overlay2(const vec3d& origin, const vec3d& mins, const vec3d& max, vec3d const& orientation, const color_t& faceColor, const color_t& edgeColor, float duration) = 0;
		virtual void add_line_overlay(const vec3d& origin, const vec3d& dest, int r, int g, int b, int a, float thickness, float duration) = 0;
		virtual void purge_text_overlays() = 0;
		virtual void add_capsule_overlay(const vec3d& mins, const vec3d& max, float& radius, int r, int g, int b, int a, float duration, char unknown, char ignorez) = 0;
		inline void add_text_overlay(const vec3d& origin, int line_offset, float duration, int r, int g, int b, int a, _Printf_format_string_ const char* format, ...) {} /* catch improper use of bad interface. Needed because '0' duration can be resolved by compiler to NULL format string (i.e., compiles but calls wrong function) */
	};
} // namespace sdk