#pragma once
#include "vector.hpp"

struct quaternion_t {
	__forceinline quaternion_t(void) {}
	__forceinline quaternion_t(float ix, float iy, float iz, float iw) : x(ix), y(iy), z(iz), w(iw) {}

	__forceinline void init(float ix = 0.0f, float iy = 0.0f, float iz = 0.0f, float iw = 0.0f) {
		x = ix;
		y = iy;
		z = iz;
		w = iw;
	}

	float* base() { return (float*)this; }
	const float* base() const { return (float*)this; }

	float x, y, z, w;
};

struct __declspec(align(16)) c_quaternion_aligned : quaternion_t {
	__forceinline c_quaternion_aligned(void){};
	__forceinline c_quaternion_aligned(float X, float Y, float Z, float W) { init(X, Y, Z, W); }

	explicit c_quaternion_aligned(const quaternion_t& v) { init(v.x, v.y, v.z, v.w); }

	c_quaternion_aligned& operator=(const quaternion_t& v) {
		init(v.x, v.y, v.z, v.w);
		return *this;
	}
};

struct matrix3x4_t {
	matrix3x4_t() {}
	matrix3x4_t(
			float m00, float m01, float m02, float m03,
			float m10, float m11, float m12, float m13,
			float m20, float m21, float m22, float m23) {
		m_value[0][0] = m00;
		m_value[0][1] = m01;
		m_value[0][2] = m02;
		m_value[0][3] = m03;
		m_value[1][0] = m10;
		m_value[1][1] = m11;
		m_value[1][2] = m12;
		m_value[1][3] = m13;
		m_value[2][0] = m20;
		m_value[2][1] = m21;
		m_value[2][2] = m22;
		m_value[2][3] = m23;
	}

	__forceinline void init(const vec3d& x, const vec3d& y, const vec3d& z, const vec3d& origin) {
		m_value[0][0] = x.x;
		m_value[0][1] = y.x;
		m_value[0][2] = z.x;
		m_value[0][3] = origin.x;
		m_value[1][0] = x.y;
		m_value[1][1] = y.y;
		m_value[1][2] = z.y;
		m_value[1][3] = origin.y;
		m_value[2][0] = x.z;
		m_value[2][1] = y.z;
		m_value[2][2] = z.z;
		m_value[2][3] = origin.z;
	}

	__forceinline matrix3x4_t(const vec3d& x, const vec3d& y, const vec3d& z, const vec3d& origin) { init(x, y, z, origin); }

	__forceinline void set_origin(vec3d const& p) {
		m_value[0][3] = p.x;
		m_value[1][3] = p.y;
		m_value[2][3] = p.z;
	}

	__forceinline vec3d get_origin() const { return { m_value[0][3], m_value[1][3], m_value[2][3] }; }

	__forceinline void set_forward(const vec3d& forward) {
		m_value[0][0] = forward.x;
		m_value[1][0] = forward.y;
		m_value[2][0] = forward.z;
	}

	__forceinline void set_left(const vec3d& left) {
		m_value[0][1] = left.x;
		m_value[1][1] = left.y;
		m_value[2][1] = left.z;
	}

	__forceinline void set_up(const vec3d& up) {
		m_value[0][2] = up.x;
		m_value[1][2] = up.y;
		m_value[2][2] = up.z;
	}

	__forceinline void quaternion_matrix(const quaternion_t& q) {
		m_value[0][0] = (float)(1.f - 2.f * q.y * q.y - 2.f * q.z * q.z);
		m_value[1][0] = (float)(2.f * q.x * q.y + 2.f * q.w * q.z);
		m_value[2][0] = (float)(2.f * q.x * q.z - 2.f * q.w * q.y);

		m_value[0][1] = (float)(2.f * q.x * q.y - 2.f * q.w * q.z);
		m_value[1][1] = (float)(1.f - 2.0f * q.x * q.x - 2.f * q.z * q.z);
		m_value[2][1] = (float)(2.f * q.y * q.z + 2.f * q.w * q.x);

		m_value[0][2] = (float)(2.f * q.x * q.z + 2.f * q.w * q.y);
		m_value[1][2] = (float)(2.f * q.y * q.z - 2.f * q.w * q.x);
		m_value[2][2] = (float)(1.f - 2.f * q.x * q.x - 2.f * q.y * q.y);

		m_value[0][3] = 0.0f;
		m_value[1][3] = 0.0f;
		m_value[2][3] = 0.0f;
	}

	__forceinline void quaternion_matrix(const quaternion_t& q, const vec3d& pos) {
		quaternion_matrix(q);

		m_value[0][3] = pos.x;
		m_value[1][3] = pos.y;
		m_value[2][3] = pos.z;
	}

	__forceinline matrix3x4_t contact_transforms(matrix3x4_t in) const {
		matrix3x4_t out = {};

		out.m_value[0][0] = m_value[0][0] * in.m_value[0][0] + m_value[0][1] * in.m_value[1][0] + m_value[0][2] * in.m_value[2][0];
		out.m_value[0][1] = m_value[0][0] * in.m_value[0][1] + m_value[0][1] * in.m_value[1][1] + m_value[0][2] * in.m_value[2][1];
		out.m_value[0][2] = m_value[0][0] * in.m_value[0][2] + m_value[0][1] * in.m_value[1][2] + m_value[0][2] * in.m_value[2][2];
		out.m_value[0][3] = m_value[0][0] * in.m_value[0][3] + m_value[0][1] * in.m_value[1][3] + m_value[0][2] * in.m_value[2][3] + m_value[0][3];
		out.m_value[1][0] = m_value[1][0] * in.m_value[0][0] + m_value[1][1] * in.m_value[1][0] + m_value[1][2] * in.m_value[2][0];
		out.m_value[1][1] = m_value[1][0] * in.m_value[0][1] + m_value[1][1] * in.m_value[1][1] + m_value[1][2] * in.m_value[2][1];
		out.m_value[1][2] = m_value[1][0] * in.m_value[0][2] + m_value[1][1] * in.m_value[1][2] + m_value[1][2] * in.m_value[2][2];
		out.m_value[1][3] = m_value[1][0] * in.m_value[0][3] + m_value[1][1] * in.m_value[1][3] + m_value[1][2] * in.m_value[2][3] + m_value[1][3];
		out.m_value[2][0] = m_value[2][0] * in.m_value[0][0] + m_value[2][1] * in.m_value[1][0] + m_value[2][2] * in.m_value[2][0];
		out.m_value[2][1] = m_value[2][0] * in.m_value[0][1] + m_value[2][1] * in.m_value[1][1] + m_value[2][2] * in.m_value[2][1];
		out.m_value[2][2] = m_value[2][0] * in.m_value[0][2] + m_value[2][1] * in.m_value[1][2] + m_value[2][2] * in.m_value[2][2];
		out.m_value[2][3] = m_value[2][0] * in.m_value[0][3] + m_value[2][1] * in.m_value[1][3] + m_value[2][2] * in.m_value[2][3] + m_value[2][3];

		return out;
	}

	__forceinline void invalidate() {
		for (int i = 0; i < 3; i++)
			for (int j = 0; j < 4; j++)
				m_value[i][j] = std::numeric_limits<float>::infinity();
	}

	__forceinline vec3d at(int i) const { return vec3d{ m_value[0][i], m_value[1][i], m_value[2][i] }; }

	__forceinline float* operator[](int i) { return m_value[i]; }
	__forceinline const float* operator[](int i) const { return m_value[i]; }
	__forceinline float* base() { return &m_value[0][0]; }
	__forceinline const float* base() const { return &m_value[0][0]; }

	__forceinline bool is_valid() {
		for (int i = 0; i < 3; ++i)
			for (int j = 0; j < 4; ++j)
				if (!std::isfinite(m_value[i][j]))
					return false;

		return true;
	}

	float m_value[3][4]{};
};

using matrix4x4_t = float[4][4];