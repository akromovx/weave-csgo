#pragma once
#include <algorithm>
#include <cmath>

#ifdef IMGUI_USE_BGRA_PACKED_COLOR
#define IM_COL32_R_SHIFT_ 16
#define IM_COL32_G_SHIFT_ 8
#define IM_COL32_B_SHIFT_ 0
#define IM_COL32_A_SHIFT_ 24
#define IM_COL32_A_MASK_ 0xFF000000
#else
#define IM_COL32_R_SHIFT_ 0
#define IM_COL32_G_SHIFT_ 8
#define IM_COL32_B_SHIFT_ 16
#define IM_COL32_A_SHIFT_ 24
#define IM_COL32_A_MASK_ 0xFF000000
#endif

#define IM_COL32_(R, G, B, A) (((uint32_t)(A) << IM_COL32_A_SHIFT_) | ((uint32_t)(B) << IM_COL32_B_SHIFT_) | ((uint32_t)(G) << IM_COL32_G_SHIFT_) | ((uint32_t)(R) << IM_COL32_R_SHIFT_))

class color_t {
private:
	union {
		uint8_t rgba[4] = {};
		uint32_t as_u32;
	} m_color;

public:
	__forceinline constexpr color_t() {
		this->u32() = 0xFFFFFFFF;
	}

	__forceinline constexpr color_t(uint32_t color32) {
		this->u32() = color32;
	}

	__forceinline constexpr color_t(int r, int g, int b) {
		this->set((uint8_t)r, (uint8_t)g, (uint8_t)b, 255);
	}

	__forceinline constexpr color_t(int r, int g, int b, int a) {
		this->set((uint8_t)r, (uint8_t)g, (uint8_t)b, (uint8_t)a);
	}

	__forceinline constexpr void set(uint32_t value) {
		this->u32() = value;
	}

	__forceinline constexpr void set(uint8_t _r, uint8_t _g, uint8_t _b, uint8_t _a = 0xFF) {
		m_color.rgba[0] = std::clamp<uint8_t>(_r, 0u, 255u);
		m_color.rgba[1] = std::clamp<uint8_t>(_g, 0u, 255u);
		m_color.rgba[2] = std::clamp<uint8_t>(_b, 0u, 255u);
		m_color.rgba[3] = std::clamp<uint8_t>(_a, 0u, 255u);
	}

	__forceinline constexpr void get(int& r, int& g, int& b, int& a) const {
		r = m_color.rgba[0];
		g = m_color.rgba[1];
		b = m_color.rgba[2];
		a = m_color.rgba[3];
	}

	__forceinline constexpr void get(int& r, int& g, int& b) const {
		r = m_color.rgba[0];
		g = m_color.rgba[1];
		b = m_color.rgba[2];
	}

	__forceinline constexpr uint8_t& r() { return m_color.rgba[0]; }
	__forceinline constexpr uint8_t& g() { return m_color.rgba[1]; }
	__forceinline constexpr uint8_t& b() { return m_color.rgba[2]; }
	__forceinline constexpr uint8_t& a() { return m_color.rgba[3]; }

	__forceinline constexpr uint8_t r() const { return m_color.rgba[0]; }
	__forceinline constexpr uint8_t g() const { return m_color.rgba[1]; }
	__forceinline constexpr uint8_t b() const { return m_color.rgba[2]; }
	__forceinline constexpr uint8_t a() const { return m_color.rgba[3]; }

	__forceinline constexpr uint8_t& operator[](int idx) {
		return m_color.rgba[idx];
	}

	const __forceinline constexpr uint8_t& operator[](int idx) const {
		return m_color.rgba[idx];
	}

	__forceinline constexpr bool operator==(const color_t& other) const {
		return other.u32() == this->u32();
	}

	const __forceinline constexpr bool& operator!=(const color_t& other) const {
		return !(*this == other);
	}

	__forceinline constexpr color_t& operator=(const color_t& other) {
		this->u32() = other.u32();
		return *this;
	}

	__forceinline constexpr uint32_t& u32() { return m_color.as_u32; }
	__forceinline constexpr uint32_t u32() const { return m_color.as_u32; }

	__forceinline constexpr uint32_t abgr() const {
		return IM_COL32_(r(), g(), b(), a());
	}

	__forceinline constexpr double hue() const {
		double r = m_color.rgba[0] / 255.f;
		double g = m_color.rgba[1] / 255.f;
		double b = m_color.rgba[2] / 255.f;

		double mx = std::max<double>(r, std::max<double>(g, b));
		double mn = std::min<double>(r, std::min<double>(g, b));
		if (mx == mn)
			return 0.f;

		double delta = mx - mn;

		double hue = 0.f;
		if (mx == r)
			hue = (g - b) / delta;
		else if (mx == g)
			hue = 2.f + (b - r) / delta;
		else
			hue = 4.f + (r - g) / delta;

		hue *= 60.f;
		if (hue < 0.f)
			hue += 360.f;

		return hue / 360.f;
	}

	__forceinline constexpr double saturation() const {
		double r = m_color.rgba[0] / 255.f;
		double g = m_color.rgba[1] / 255.f;
		double b = m_color.rgba[2] / 255.f;

		double mx = std::max<double>(r, std::max<double>(g, b));
		double mn = std::min<double>(r, std::min<double>(g, b));

		double delta = mx - mn;

		if (mx == 0.f)
			return delta;

		return delta / mx;
	}

	__forceinline constexpr double brightness() const {
		double r = m_color.rgba[0] / 255.f;
		double g = m_color.rgba[1] / 255.f;
		double b = m_color.rgba[2] / 255.f;

		return std::max<double>(r, std::max<double>(g, b));
	}

	static __forceinline constexpr color_t hsb(float hue, float saturation, float brightness) {
		hue = std::clamp<float>(hue, 0.f, 1.f);
		saturation = std::clamp<float>(saturation, 0.f, 1.f);
		brightness = std::clamp<float>(brightness, 0.f, 1.f);

		const auto h = (hue == 1.f) ? 0.f : (hue * 6.f);
		const auto f = h - (int)h;
		const auto p = brightness * (1.f - saturation);
		const auto q = brightness * (1.f - saturation * f);
		const auto t = brightness * (1.f - (saturation * (1.f - f)));

		if (h < 1.f)
			return { (int)(brightness * 255), (int)(t * 255), (int)(p * 255) };
		else if (h < 2.f)
			return { (int)(q * 255), (int)(brightness * 255), (int)(p * 255) };
		else if (h < 3.f)
			return { (int)(p * 255), (int)(brightness * 255), (int)(t * 255) };
		else if (h < 4)
			return { (int)(p * 255), (int)(q * 255), (int)(brightness * 255) };
		else if (h < 5)
			return { (int)(t * 255), (int)(p * 255), (int)(brightness * 255) };

		return { (int)(brightness * 255), (int)(p * 255), (int)(q * 255) };
	}

	__forceinline constexpr color_t lerp(const color_t& other, float lerp) const {
		if (*this == other)
			return *this;

		return color_t{
			(int)std::lerp((float)m_color.rgba[0], (float)other.r(), lerp),
			(int)std::lerp((float)m_color.rgba[1], (float)other.g(), lerp),
			(int)std::lerp((float)m_color.rgba[2], (float)other.b(), lerp)
		};
	}

	__forceinline constexpr color_t new_alpha(int alpha) const {
		return { m_color.rgba[0], m_color.rgba[1], m_color.rgba[2], std::clamp(alpha, 0, 255) };
	}

	__forceinline constexpr color_t new_alpha(float alpha) const {
		return { m_color.rgba[0], m_color.rgba[1], m_color.rgba[2], std::clamp((int)(alpha * 255.f), 0, 255) };
	}

	__forceinline constexpr color_t modify_alpha(float modifier) const {
		return { m_color.rgba[0], m_color.rgba[1], m_color.rgba[2], (uint8_t)((float)m_color.rgba[3] * modifier) };
	}

	__forceinline constexpr color_t increase(int value, bool consider_alpha = false) const {
		return { m_color.rgba[0] + value, m_color.rgba[1] + value, m_color.rgba[2] + value, m_color.rgba[3] + consider_alpha * value };
	}
	__forceinline constexpr color_t decrease(int value, bool consider_alpha = false) const {
		return increase(-value, consider_alpha);
	}

	__forceinline constexpr static color_t lerp(const color_t& v1, const color_t& v2, float lerp) {
		return v1.lerp(v2, lerp);
	}
};

namespace colors {
	inline constexpr color_t red = color_t(255, 0, 0, 255);
	inline constexpr color_t green = color_t(0, 255, 0, 255);
	inline constexpr color_t blue = color_t(0, 0, 255, 255);
	inline constexpr color_t black = color_t(0, 0, 0, 255);
	inline constexpr color_t white = color_t(255, 255, 255, 255);
} // namespace colors
