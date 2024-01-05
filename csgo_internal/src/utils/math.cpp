#include "math.hpp"
#include "../render.hpp"
#include "utils.hpp"
#include <DirectXMath.h>
#include <random>

typedef __declspec(align(16)) union {
	float f[4];
	__m128 v;
} m128;

namespace math {
	inline vec3d vector_rotate(vec3d& in1, matrix3x4_t& in2) {
		return { in1.dot(in2[0]), in1.dot(in2[1]), in1.dot(in2[2]) };
	}

	inline vec3d vector_rotate(vec3d& in1, vec3d& in2) {
		matrix3x4_t m;
		math::angle_matrix(in2, m);
		return vector_rotate(in1, m);
	}

	inline void matrix_copy(const matrix3x4_t& source, matrix3x4_t& target) {
		for (int i = 0; i < 3; i++)
			for (int j = 0; j < 4; j++)
				target[i][j] = source[i][j];
	}

	inline void matrix_multiply(matrix3x4_t& in1, const matrix3x4_t& in2) {
		matrix3x4_t out;
		if (&in1 == &out) {
			matrix3x4_t in1b;
			matrix_copy(in1, in1b);
			matrix_multiply(in1b, in2);
			return;
		}

		if (&in2 == &out) {
			matrix3x4_t in2b;
			matrix_copy(in2, in2b);
			matrix_multiply(in1, in2b);
			return;
		}

		out[0][0] = in1[0][0] * in2[0][0] + in1[0][1] * in2[1][0] + in1[0][2] * in2[2][0];
		out[0][1] = in1[0][0] * in2[0][1] + in1[0][1] * in2[1][1] + in1[0][2] * in2[2][1];
		out[0][2] = in1[0][0] * in2[0][2] + in1[0][1] * in2[1][2] + in1[0][2] * in2[2][2];
		out[0][3] = in1[0][0] * in2[0][3] + in1[0][1] * in2[1][3] + in1[0][2] * in2[2][3] + in1[0][3];
		out[1][0] = in1[1][0] * in2[0][0] + in1[1][1] * in2[1][0] + in1[1][2] * in2[2][0];
		out[1][1] = in1[1][0] * in2[0][1] + in1[1][1] * in2[1][1] + in1[1][2] * in2[2][1];
		out[1][2] = in1[1][0] * in2[0][2] + in1[1][1] * in2[1][2] + in1[1][2] * in2[2][2];
		out[1][3] = in1[1][0] * in2[0][3] + in1[1][1] * in2[1][3] + in1[1][2] * in2[2][3] + in1[1][3];
		out[2][0] = in1[2][0] * in2[0][0] + in1[2][1] * in2[1][0] + in1[2][2] * in2[2][0];
		out[2][1] = in1[2][0] * in2[0][1] + in1[2][1] * in2[1][1] + in1[2][2] * in2[2][1];
		out[2][2] = in1[2][0] * in2[0][2] + in1[2][1] * in2[1][2] + in1[2][2] * in2[2][2];
		out[2][3] = in1[2][0] * in2[0][3] + in1[2][1] * in2[1][3] + in1[2][2] * in2[2][3] + in1[2][3];

		in1 = out;
	}

	inline void rotate_matrix(matrix3x4_t* in, matrix3x4_t* out, float delta, vec3d origin) {
		auto vDelta = vec3d{ 0.f, delta, 0.f };
		vec3d vOutPos;
		for (int i = 0; i < 128; i++) {
			math::angle_matrix(vDelta, out[i]);
			matrix_multiply(out[i], in[i]);
			auto vBonePos = vec3d{ in[i][0][3],
								   in[i][1][3],
								   in[i][2][3] } -
							origin;
			vOutPos = vector_rotate(vBonePos, vDelta);
			vOutPos += origin;
			out[i][0][3] = vOutPos.x;
			out[i][1][3] = vOutPos.y;
			out[i][2][3] = vOutPos.z;
		}
	}

	inline void vector_angles(const vec3d& forward, const vec3d& pseudoup, vec3d& angles) {
		vec3d left = pseudoup.cross_product(forward);
		left.normalize_in_place();

		float forward_dist = forward.length2d();
		if (forward_dist > 0.001f) {
			angles.x = (float)(atan2f(-forward.z, forward_dist) * 180.0 / PI);
			angles.y = (float)(atan2f(forward.y, forward.x) * 180.0 / PI);
			angles.z = (float)(atan2f(left.z, (left.y * forward.x) - (left.x * forward.y)) * 180.0 / PI);
		} else {
			angles.x = (float)(atan2f(-forward.z, forward_dist) * 180 / PI);
			angles.y = (float)(atan2f(-left.x, left.y) * 180.0 / PI);
			angles.z = 0.f;
		}
	}

	inline void vector_angles(const vec3d& forward, vec3d& angles) {
		float tmp, yaw, pitch;

		if (forward.y == 0.f && forward.x == 0.f) {
			yaw = 0.f;
			if (forward.z > 0)
				pitch = 270.f;
			else
				pitch = 90.f;
		} else {
			yaw = (float)(atan2(forward.y, forward.x) * 180.0 / PI);
			if (yaw < 0.f)
				yaw += 360.f;

			tmp = sqrt(forward.x * forward.x + forward.y * forward.y);
			pitch = (float)(atan2(-forward[2], tmp) * 180.0 / PI);
			if (pitch < 0.f)
				pitch += 360.f;
		}

		angles.x = pitch;
		angles.y = yaw;
		angles.z = 0.f;
	}

	inline void angle_vectors(const vec3d& angles, vec3d& forward) {
		float sx{}, sy{}, cx{}, cy{};

		DirectX::XMScalarSinCos(&sy, &cy, DEG2RAD(angles[1]));
		DirectX::XMScalarSinCos(&sx, &cx, DEG2RAD(angles[0]));

		forward.x = cx * cy;
		forward.y = cx * sy;
		forward.z = -sx;
	}

	inline void angle_vectors(const vec3d& angles, vec3d& forward, vec3d& right, vec3d& up) {
		float sr, sp, sy, cr, cp, cy;

		DirectX::XMScalarSinCos(&sy, &cy, DEG2RAD(angles[1]));
		DirectX::XMScalarSinCos(&sp, &cp, DEG2RAD(angles[0]));
		DirectX::XMScalarSinCos(&sr, &cr, DEG2RAD(angles[2]));

		forward.x = (cp * cy);
		forward.y = (cp * sy);
		forward.z = (-sp);
		right.x = (-1.f * sr * sp * cy + -1.f * cr * -sy);
		right.y = (-1.f * sr * sp * sy + -1.f * cr * cy);
		right.z = (-1.f * sr * cp);
		up.x = (cr * sp * cy + -sr * -sy);
		up.y = (cr * sp * sy + -sr * cy);
		up.z = (cr * cp);
	}

	inline float normalize_yaw(float yaw) {
		while (yaw < -180.f)
			yaw += 360.f;
		while (yaw > 180.f)
			yaw -= 360.f;

		return yaw;
	}

	inline float angle_diff(float dst, float src) {
		float delta = dst - src;

		if (delta < -180)
			delta += 360;
		else if (delta > 180)
			delta -= 360;

		return delta;
	}

	inline bool screen_transform(const vec3d& in, vec2d& out) {
		const auto& w2_s_matrix = render::get_view_matrix();
		out.x = w2_s_matrix[0][0] * in[0] + w2_s_matrix[0][1] * in[1] + w2_s_matrix[0][2] * in[2] + w2_s_matrix[0][3];
		out.y = w2_s_matrix[1][0] * in[0] + w2_s_matrix[1][1] * in[1] + w2_s_matrix[1][2] * in[2] + w2_s_matrix[1][3];

		const auto w = w2_s_matrix[3][0] * in.x + w2_s_matrix[3][1] * in.y + w2_s_matrix[3][2] * in.z + w2_s_matrix[3][3];

		if (w < 0.001f) {
			out.x *= 100000;
			out.y *= 100000;
			return false;
		}

		const auto invw = 1.0f / w;
		out.x *= invw;
		out.y *= invw;

		return true;
	}

	inline bool world_to_screen(const vec3d& in, vec2d& out) {
		if (screen_transform(in, out)) {
			out.x = (render::screen_width / 2.0f) + (out.x * render::screen_width) / 2.0f;
			out.y = (render::screen_height / 2.0f) - (out.y * render::screen_height) / 2.0f;
			return true;
		}
		return false;
	}

	inline void vector_transform(const vec3d& in1, const matrix3x4_t& in2, vec3d& out) {
		out = {
			in1.dot({ in2[0][0], in2[0][1], in2[0][2] }) + in2[0][3],
			in1.dot({ in2[1][0], in2[1][1], in2[1][2] }) + in2[1][3],
			in1.dot({ in2[2][0], in2[2][1], in2[2][2] }) + in2[2][3]
		};
	}

	inline void change_bones_position(matrix3x4_t* bones, size_t msize, const vec3d& current_position, const vec3d& new_position) {
		const auto delta = new_position - current_position;

		for (size_t i = 0; i < msize; ++i) {
			bones[i][0][3] += delta.x;
			bones[i][1][3] += delta.y;
			bones[i][2][3] += delta.z;
		}
	}

	inline void angle_matrix(const vec3d& ang_view, const vec3d& origin, matrix3x4_t& mat_output) {
		float sp, sy, sr, cp, cy, cr;

		DirectX::XMScalarSinCos(&sp, &cp, DEG2RAD(ang_view.x));
		DirectX::XMScalarSinCos(&sy, &cy, DEG2RAD(ang_view.y));
		DirectX::XMScalarSinCos(&sr, &cr, DEG2RAD(ang_view.z));

		mat_output.set_forward(vec3d(cp * cy, cp * sy, -sp));

		const float crcy = cr * cy;
		const float crsy = cr * sy;
		const float srcy = sr * cy;
		const float srsy = sr * sy;

		mat_output.set_left(vec3d(sp * srcy - crsy, sp * srsy + crcy, sr * cp));
		mat_output.set_up(vec3d(sp * crcy + srsy, sp * crsy - srcy, cr * cp));
		mat_output.set_origin(origin);
	}

	inline void angle_matrix(const vec3d& angles, matrix3x4_t& matrix) {
		float sr, sp, sy, cr, cp, cy;

		DirectX::XMScalarSinCos(&sy, &cy, DEG2RAD(angles[1]));
		DirectX::XMScalarSinCos(&sp, &cp, DEG2RAD(angles[0]));
		DirectX::XMScalarSinCos(&sr, &cr, DEG2RAD(angles[2]));

		// matrix = (YAW * PITCH) * ROLL
		matrix[0][0] = cp * cy;
		matrix[1][0] = cp * sy;
		matrix[2][0] = -sp;

		float crcy = cr * cy;
		float crsy = cr * sy;
		float srcy = sr * cy;
		float srsy = sr * sy;

		matrix[0][1] = sp * srcy - crsy;
		matrix[1][1] = sp * srsy + crcy;
		matrix[2][1] = sr * cp;

		matrix[0][2] = (sp * crcy + srsy);
		matrix[1][2] = (sp * crsy - srcy);
		matrix[2][2] = cr * cp;

		matrix[0][3] = 0.0f;
		matrix[1][3] = 0.0f;
		matrix[2][3] = 0.0f;
	}

	inline vec3d calculate_angle(vec3d src, vec3d dst) {
		vec3d angles{};

		vec3d delta = src - dst;
		float hyp = delta.length2d();

		angles.y = std::atanf(delta.y / delta.x) * 57.2957795131f;
		angles.x = std::atanf(-delta.z / hyp) * -57.2957795131f;
		angles.z = 0.0f;

		if (delta.x >= 0.0f)
			angles.y += 180.0f;

		return angles;
	}

	inline float segment_to_segment(const vec3d s1, const vec3d s2, const vec3d k1, const vec3d k2) {
		static auto constexpr epsilon = 0.00000001;

		auto u = s2 - s1;
		auto v = k2 - k1;
		const auto w = s1 - k1;

		const auto a = u.dot(u);
		const auto b = u.dot(v);
		const auto c = v.dot(v);
		const auto d = u.dot(w);
		const auto e = v.dot(w);
		const auto D = a * c - b * b;
		float sn, sd = D;
		float tn, td = D;

		if (D < epsilon) {
			sn = 0.0;
			sd = 1.0;
			tn = e;
			td = c;
		} else {
			sn = b * e - c * d;
			tn = a * e - b * d;

			if (sn < 0.0) {
				sn = 0.0;
				tn = e;
				td = c;
			} else if (sn > sd) {
				sn = sd;
				tn = e + b;
				td = c;
			}
		}

		if (tn < 0.0) {
			tn = 0.0;

			if (-d < 0.0)
				sn = 0.0;
			else if (-d > a)
				sn = sd;
			else {
				sn = -d;
				sd = a;
			}
		} else if (tn > td) {
			tn = td;

			if (-d + b < 0.0)
				sn = 0;
			else if (-d + b > a)
				sn = sd;
			else {
				sn = -d + b;
				sd = a;
			}
		}

		const float sc = abs(sn) < epsilon ? 0.0f : sn / sd;
		const float tc = abs(tn) < epsilon ? 0.0f : tn / td;

		m128 n{};
		auto dp = w + u * sc - v * tc;
		n.f[0] = dp.dot(dp);
		const auto calc = _mm_sqrt_ps(n.v);
		return reinterpret_cast<const m128*>(&calc)->f[0];
	}

	inline vec3d clamp_angles(const vec3d& angle) {
		vec3d ret = angle;
		ret.x = std::clamp(ret.x, -89.f, 89.f);
		ret.z = std::clamp(ret.z, -50.f, 50.f);

		while (ret.y < -180.f)
			ret.y += 360.f;
		while (ret.y > 180.f)
			ret.y -= 360.f;

		return ret;
	}

	inline bool is_bones_valid(matrix3x4_t* bones, size_t bones_count) {
		return bones[0].is_valid();
		//for (size_t i = 0; i < bones_count; ++i)
		//	if (!bones[i].is_valid())
		//		return false;

		//return true;
	}

	template<typename T>
	static inline T saturate(T value) {
		return std::clamp<T>(value, (T)0, (T)1);
	};

	static inline vec3d vec_lerp(const vec3d v, const vec3d& other, const vec3d& t) {
		return { std::lerp(v.x, other.x, t.x), std::lerp(v.y, other.y, t.y), std::lerp(v.z, other.z, t.z) };
	}

	inline bool segment_segment_cpa(const vec3d& p0, const vec3d& p1, const vec3d& q0, const vec3d& q1, vec3d& c0, vec3d& c1) {
		vec3d u(p1 - p0),
				v(q1 - q0),
				w0(p0 - q0);

		float a = u.dot(u),
			  b = u.dot(v),
			  c = v.dot(v),
			  d = u.dot(w0),
			  e = v.dot(w0),
			  len = a * c - b * b,
			  sc, tc;

		if (len == 0.f) {
			sc = 0.f;
			tc = d / b;
		} else {
			sc = (b * e - c * d) / (a * c - b * b);
			tc = (a * e - b * d) / (a * c - b * b);
		}

		c0 = vec_lerp(p0, p1, saturate(sc));
		c1 = vec_lerp(q0, q1, saturate(tc));

		return len != 0.f;
	}

	inline bool capsule_get_contact_center(const vec3d& p0, const vec3d& p1, float pr, const vec3d& q0, const vec3d& q1, float qr, vec3d& contact_center) {
		vec3d c0, c1;
		segment_segment_cpa(p0, p1, q0, q1, c0, c1);

		vec3d c = c1 - c0;
		float distance = c.length();
		vec3d axis = c / distance;
		bool contact = distance <= pr + qr;
		if (contact) {
			vec3d contact_a = c0 + axis * std::min<float>(pr, distance),
				  contact_b = c1 - axis * std::min<float>(qr, distance);
			contact_center = (contact_a + contact_b) * 0.5f;
		}

		return contact;
	}

	class c_random_generator {
	public:
		template<std::integral T>
		[[nodiscard]] static T random(T min, T max) noexcept {
			std::scoped_lock lock{ mutex };
			return std::uniform_int_distribution{ min, max }(gen);
		}

		template<std::floating_point T>
		[[nodiscard]] static T random(T min, T max) noexcept {
			std::scoped_lock lock{ mutex };
			return std::uniform_real_distribution{ min, max }(gen);
		}

		template<typename T>
		[[nodiscard]] static std::enable_if_t<std::is_enum_v<T>, T> random(T min, T max) noexcept {
			return static_cast<T>(random(static_cast<std::underlying_type_t<T>>(min), static_cast<std::underlying_type_t<T>>(max)));
		}

	private:
		inline static std::mt19937 gen{ std::random_device{}() };
		inline static std::mutex mutex;
	};

	inline float random_float(float a, float b) {
		return c_random_generator::random(a, b);
	}

	inline int random_int(int a, int b) {
		return c_random_generator::random(a, b);
	}
} // namespace math