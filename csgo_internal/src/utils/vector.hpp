#pragma once
#include "../base_includes.hpp"

struct vec2d {
	float x, y;

	__forceinline constexpr vec2d() {
		x = y = 0.0f;
	}

	__forceinline constexpr vec2d(float X, float Y) {
		x = float(X);
		y = float(Y);
	}

	__forceinline constexpr vec2d(float* v) {
		x = v[0];
		y = v[1];
	}

	__forceinline vec2d(const float* v) {
		x = v[0];
		y = v[1];
	}

	__forceinline vec2d(const vec2d& v) {
		x = v.x;
		y = v.y;
	}

	__forceinline vec2d& operator=(const vec2d& v) {
		x = v.x;
		y = v.y;
		return *this;
	}

	__forceinline float& operator[](int i) {
		return ((float*)this)[i];
	}

	__forceinline float operator[](int i) const {
		return ((float*)this)[i];
	}

	__forceinline vec2d& operator+=(const vec2d& v) {
		x += v.x;
		y += v.y;
		return *this;
	}

	__forceinline vec2d& operator-=(const vec2d& v) {
		x -= v.x;
		y -= v.y;
		return *this;
	}

	__forceinline vec2d& operator*=(const vec2d& v) {
		x *= v.x;
		y *= v.y;
		return *this;
	}

	__forceinline vec2d& operator/=(const vec2d& v) {
		x /= v.x;
		y /= v.y;
		return *this;
	}

	__forceinline vec2d& operator+=(float v) {
		x += v;
		y += v;
		return *this;
	}

	__forceinline vec2d& operator-=(float v) {
		x -= v;
		y -= v;
		return *this;
	}

	__forceinline vec2d& operator*=(float v) {
		x *= v;
		y *= v;
		return *this;
	}

	__forceinline vec2d& operator/=(float v) {
		x /= v;
		y /= v;
		return *this;
	}

	__forceinline vec2d operator+(const vec2d& v) const {
		return { x + v.x, y + v.y };
	}

	__forceinline vec2d operator-(const vec2d& v) const {
		return { x - v.x, y - v.y };
	}

	__forceinline vec2d operator*(const vec2d& v) const {
		return { x * v.x, y * v.y };
	}

	__forceinline vec2d operator/(const vec2d& v) const {
		return { x / v.x, y / v.y };
	}

	__forceinline vec2d operator+(float v) const {
		return { x + v, y + v };
	}

	__forceinline vec2d operator-(float v) const {
		return { x - v, y - v };
	}

	__forceinline vec2d operator*(float v) const {
		return { x * v, y * v };
	}

	__forceinline vec2d operator/(float v) const {
		return { x / v, y / v };
	}

	__forceinline void set(float X = 0.0f, float Y = 0.0f) {
		x = X;
		y = Y;
	}

	__forceinline float length(void) const {
		return std::sqrt(x * x + y * y);
	}

	__forceinline float length_sqr(void) const {
		return (x * x + y * y);
	}

	__forceinline float dist_to(const vec2d& v) const {
		return (*this - v).length();
	}

	__forceinline float dist_to_sqr(const vec2d& v) const {
		return (*this - v).length_sqr();
	}

	__forceinline float dot(const vec2d& v) const {
		return (x * v.x + y * v.y);
	}

	__forceinline bool is_zero() const {
		return (x > -FLT_EPSILON && x < FLT_EPSILON &&
				y > -FLT_EPSILON && y < FLT_EPSILON);
	}

	__forceinline operator bool() const noexcept {
		return !is_zero();
	}

	__forceinline vec2d round() {
		x = std::round(x);
		y = std::round(y);
		return *this;
	}

	__forceinline bool is_negative() const noexcept {
		return x < 0.f || y < 0.f;
	}
};

struct vec3d {
	float x, y, z;

	__forceinline constexpr vec3d() {
		x = y = z = 0.0f;
	}

	__forceinline constexpr vec3d(float value) {
		x = y = z = value;
	}

	__forceinline constexpr vec3d(float X, float Y, float Z) {
		x = X;
		y = Y;
		z = Z;
	}

	__forceinline constexpr vec3d(float* v) {
		x = v[0];
		y = v[1];
		z = v[2];
	}

	__forceinline constexpr vec3d(const float* v) {
		x = v[0];
		y = v[1];
		z = v[2];
	}

	__forceinline constexpr vec3d(const vec3d& v) {
		x = v.x;
		y = v.y;
		z = v.z;
	}

	__forceinline constexpr vec3d(const vec2d& v) {
		x = v.x;
		y = v.y;
		z = 0.0f;
	}

	__forceinline constexpr vec3d& operator=(const vec3d& v) {
		x = v.x;
		y = v.y;
		z = v.z;
		return *this;
	}

	__forceinline constexpr vec3d& operator=(const vec2d& v) {
		x = v.x;
		y = v.y;
		z = 0.0f;
		return *this;
	}

	__forceinline constexpr float& operator[](int i) {
		return ((float*)this)[i];
	}

	__forceinline constexpr float operator[](int i) const {
		return ((float*)this)[i];
	}

	__forceinline constexpr vec3d& operator+=(const vec3d& v) {
		x += v.x;
		y += v.y;
		z += v.z;
		return *this;
	}

	__forceinline constexpr vec3d& operator-=(const vec3d& v) {
		x -= v.x;
		y -= v.y;
		z -= v.z;
		return *this;
	}

	__forceinline constexpr vec3d& operator*=(const vec3d& v) {
		x *= v.x;
		y *= v.y;
		z *= v.z;
		return *this;
	}

	__forceinline constexpr vec3d& operator/=(const vec3d& v) {
		x /= v.x;
		y /= v.y;
		z /= v.z;
		return *this;
	}

	__forceinline constexpr vec3d& operator+=(float v) {
		x += v;
		y += v;
		z += v;
		return *this;
	}

	__forceinline constexpr vec3d& operator-=(float v) {
		x -= v;
		y -= v;
		z -= v;
		return *this;
	}

	__forceinline constexpr vec3d& operator*=(float v) {
		x *= v;
		y *= v;
		z *= v;
		return *this;
	}

	__forceinline constexpr vec3d& operator/=(float v) {
		x /= v;
		y /= v;
		z /= v;
		return *this;
	}

	__forceinline constexpr vec3d operator+(const vec3d& v) const {
		return vec3d{ x + v.x, y + v.y, z + v.z };
	}

	__forceinline constexpr vec3d operator-(const vec3d& v) const {
		return vec3d{ x - v.x, y - v.y, z - v.z };
	}

	__forceinline constexpr vec3d operator*(const vec3d& v) const {
		return vec3d{ x * v.x, y * v.y, z * v.z };
	}

	__forceinline constexpr vec3d operator/(const vec3d& v) const {
		return vec3d{ x / v.x, y / v.y, z / v.z };
	}

	__forceinline constexpr vec3d operator+(float v) const {
		return vec3d{ x + v, y + v, z + v };
	}

	__forceinline constexpr vec3d operator-(float v) const {
		return vec3d{ x - v, y - v, z - v };
	}

	__forceinline constexpr vec3d operator*(float v) const {
		return vec3d{ x * v, y * v, z * v };
	}

	__forceinline constexpr vec3d operator/(float v) const {
		return vec3d{ x / v, y / v, z / v };
	}

	__forceinline constexpr bool operator==(const vec3d& other) const {
		return x == other.x && y == other.y && other.z == other.z;
	}

	__forceinline constexpr bool operator!=(const vec3d& other) const {
		return !(*this == other);
	}

	__forceinline constexpr bool operator!() const {
		return this->is_zero();
	}

	__forceinline constexpr void set(float X = 0.0f, float Y = 0.0f, float Z = 0.0f) {
		x = X;
		y = Y;
		z = Z;
	}

	__forceinline constexpr vec3d center() const {
		return *this * 0.5f;
	}

	__forceinline float length() const {
		return std::sqrt(x * x + y * y + z * z);
	}

	__forceinline constexpr float length_sqr() const {
		return x * x + y * y + z * z;
	}

	__forceinline float length2d() const {
		return std::sqrt(x * x + y * y);
	}

	__forceinline constexpr float length2d_sqr() const {
		return (x * x + y * y);
	}

	__forceinline float dist(const vec3d& v) const {
		return (*this - v).length();
	}

	__forceinline float dist_sqr(const vec3d& v) const {
		return (*this - v).length_sqr();
	}

	__forceinline constexpr float dot(const vec3d& v) const {
		return x * v.x + y * v.y + z * v.z;
	}

	__forceinline constexpr float dot_2d(const vec3d& v) const {
		return x * v.x + y * v.y;
	}

	__forceinline constexpr vec3d cross_product(const vec3d& v) const {
		return vec3d{ y * v.z - z * v.y, z * v.x - x * v.z, x * v.y - y * v.x };
	}

	__forceinline bool is_nan() const { return std::isnan(x) || std::isnan(y) || std::isnan(z); }
	__forceinline bool is_valid() const { return std::isfinite(x) && std::isfinite(y) && std::isfinite(z); }

	__forceinline constexpr void normalize() {
		auto& vec = *this;

		for (auto i = 0; i <= 2; i++)
			if (vec[i] > 180.f || vec[i] < -180.f)
				vec[i] = (vec[i] < 0.f) ? vec[i] + 360.f * round(abs(vec[i] / 360.f)) : vec[i] - 360.f * round(abs(vec[i] / 360.f));
	}

	__forceinline constexpr vec3d normalized() const {
		vec3d vec = *this;
		vec.normalize();
		return vec;
	}

	__forceinline constexpr void clamp() {
		x = std::clamp<float>(x, -89.f, 89.f);
		y = std::clamp<float>(y, -180.0f, 180.0f);
		z = 0.0f;
	}

	__forceinline vec3d clamped() const {
		return vec3d{ std::clamp<float>(x, -89.f, 89.f), std::clamp<float>(y, -180.0f, 180.0f), 0.0f };
	}

	__forceinline void sanitize() {
		if (!std::isfinite(x) || std::isnan(x) || std::isinf(x))
			x = 0.0f;

		if (!std::isfinite(y) || std::isnan(y) || std::isinf(y))
			y = 0.0f;

		normalize();
		clamp();
	}

	__forceinline auto normalize_in_place() {
		float iradius = 1.f / (this->length() + FLT_EPSILON);

		x *= iradius;
		y *= iradius;
		z *= iradius;

		return *this;
	}

	__forceinline float normalize_movement() const {
		vec3d res = *this;
		float len = res.length();
		if (len != 0.0f)
			res /= len;
		else
			res.x = res.y = res.z = 0.0f;

		return len;
	}

	__forceinline constexpr bool is_zero() const {
		return (x > -FLT_EPSILON && x < FLT_EPSILON &&
				y > -FLT_EPSILON && y < FLT_EPSILON &&
				z > -FLT_EPSILON && z < FLT_EPSILON);
	}
};

struct vec4d {
	float x, y, z, w;
};

struct __declspec(align(16)) vec3d_aligned : public vec3d {
	__forceinline vec3d_aligned(void){};
	__forceinline vec3d_aligned(float X, float Y, float Z) {
		set(X, Y, Z);
	}

	__forceinline explicit vec3d_aligned(const vec3d& other) {
		set(other.x, other.y, other.z);
	}

	vec3d_aligned& operator=(const vec3d& other) {
		set(other.x, other.y, other.z);
		return *this;
	}

	vec3d_aligned& operator=(const vec3d_aligned& other) {
		set(other.x, other.y, other.z);
		return *this;
	}

	float w{};
};