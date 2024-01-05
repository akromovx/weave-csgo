#pragma once
#include "matrix.hpp"

namespace math {
	template <typename type_t>
	static __forceinline type_t lerp(const type_t& from, const type_t& to, const float percent) {
		return to * percent + from * (1.f - percent);
	}
	template <typename T>
	static __forceinline T hermit_spline(T p1, T p2, T d1, T d2, float t) {
		const auto t_sqr = t * t;
		const auto t_cube = t * t_sqr;

		const auto b1 = 2.0f * t_cube - 3.0f * t_sqr + 1.0f;
		const auto b2 = 1.0f - b1; // -2 * t_cube + 3 * t_sqr;
		const auto b3 = t_cube - 2 * t_sqr + t;
		const auto b4 = t_cube - t_sqr;

		T output;
		output = p1 * b1;
		output += p2 * b2;
		output += d1 * b3;
		output += d2 * b4;

		return output;
	}

	template <typename T>
	static __forceinline T hermit_spline(T p0, T p1, T p2, float t) {
		return hermit_spline(p1, p2, p1 - p0, p2 - p1, t);
	}

	extern vec3d vector_rotate(vec3d& in1, matrix3x4_t& in2);
	extern vec3d vector_rotate(vec3d& in1, vec3d& in2);
	extern void matrix_copy(const matrix3x4_t& source, matrix3x4_t& target);
	extern void matrix_multiply(matrix3x4_t& in1, const matrix3x4_t& in2);
	extern void rotate_matrix(matrix3x4_t* in, matrix3x4_t* out, float delta, vec3d origin);
	extern void vector_angles(const vec3d& forward, const vec3d& pseudoup, vec3d& angles);
	extern void vector_angles(const vec3d& forward, vec3d& angles);
	extern void angle_vectors(const vec3d& angles, vec3d& forward);
	extern void angle_vectors(const vec3d& angles, vec3d& forward, vec3d& right, vec3d& up);
	extern float normalize_yaw(float yaw);
	extern float angle_diff(float dst, float src);
	extern bool screen_transform(const vec3d& in, vec2d& out);
	extern bool world_to_screen(const vec3d& in, vec2d& out);
	extern void vector_transform(const vec3d& in1, const matrix3x4_t& in2, vec3d& out);
	extern void change_bones_position(matrix3x4_t* bones, size_t msize, const vec3d& current_position, const vec3d& new_position);
	extern void angle_matrix(const vec3d& ang_view, const vec3d& origin, matrix3x4_t& mat_output);
	extern void angle_matrix(const vec3d& angles, matrix3x4_t& matrix);
	extern vec3d calculate_angle(vec3d src, vec3d dst);
	extern float random_float(float a, float b);
	extern int random_int(int a, int b);
	extern float segment_to_segment(const vec3d s1, const vec3d s2, const vec3d k1, const vec3d k2);
	extern vec3d clamp_angles(const vec3d& angle);

	extern bool is_bones_valid(matrix3x4_t* bones, size_t bones_count);
	extern bool segment_segment_cpa(const vec3d& p0, const vec3d& p1, const vec3d& q0, const vec3d& q1, vec3d& c0, vec3d& c1);
	extern bool capsule_get_contact_center(const vec3d& p0, const vec3d& p1, float pr, const vec3d& q0, const vec3d& q1, float qr, vec3d& contact_center);
}