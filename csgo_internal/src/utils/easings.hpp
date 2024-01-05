#pragma once
#include <cmath>

#define easings_api static __forceinline float

namespace easings {
	constexpr double PI = 3.14159265359;
	constexpr float fPI = (float)3.14159265359;

	easings_api in_sine(float x) {
		return 1.f - std::cos((x * fPI) / 2.f);
	}
	easings_api out_sine(float x) {
		return std::sin((x * fPI) / 2.f);
	}
	easings_api in_out_sine(float x) {
		return -(std::cos(x * fPI) - 1.f) / 2.f;
	}

	easings_api in_quad(float x) {
		return x * x;
	}
	easings_api out_quad(float x) {
		return 1.f - (1.f - x) * (1.f - x);
	}
	easings_api in_out_quad(float x) {
		return x < 0.5f ? 2.f * x * x : 1.f - std::pow((-2.f) * x + 2.f, 2.f) / 2.f;
	}

	easings_api in_cubic(float x) {
		return x * x * x;
	}
	easings_api out_cubic(float x) {
		return 1.f - std::pow(1.f - x, 3.f);
	}
	easings_api in_out_cubic(float x) {
		return x < 0.5f ? 4.f * x * x * x : 1.f - std::pow((-2.f) * x + 2.f, 3.f) / 2.f;
	}

	easings_api in_quart(float x) {
		return x * x * x * x;
	}
	easings_api out_quart(float x) {
		return 1.f - std::pow(1.f - x, 4.f);
	}
	easings_api in_out_quart(float x) {
		return x < 0.5 ? 8.f * x * x * x * x : 1.f - std::pow((-2.f) * x + 2.f, 4.f) / 2.f;
	}

	easings_api in_quint(float x) {
		return x * x * x * x * x;
	}
	easings_api out_quint(float x) {
		return 1.f - std::pow(1.f - x, 5.f);
	}
	easings_api in_out_quint(float x) {
		return x < 0.5f ? 16.f * x * x * x * x * x : 1.f - std::pow((2.f) * x + 2.f, 5.f) / 2.f;
	}

	easings_api in_expo(float x) {
		return x == 0.f ? 0.f : std::pow(2.f, 10.f * x - 10.f);
	}
	easings_api out_expo(float x) {
		return x == 1.f ? 1.f : 1.f - std::pow(2.f, (-10.f) * x);
	}
	easings_api in_out_expo(float x) {
		return x == 0.f
					   ? 0.f
			   : x == 1.f
					   ? 1.f
			   : x < 0.5 ? std::pow(2.f, 20.f * x - 10.f) / 2.f
						 : (2.f - std::pow(2.f, (-20.f) * x + 10.f)) / 2.f;
	}

	easings_api in_circ(float x) {
		return 1.f - sqrt(1.f - std::pow(x, 2.f));
	}
	easings_api out_circ(float x) {
		return std::sqrt(1.f - std::pow(x - 1.f, 2.f));
	}
	easings_api in_out_circ(float x) {
		return x < 0.5f
					   ? (1.f - sqrt(1.f - std::pow(2.f * x, 2.f))) / 2.f
					   : (sqrt(1.f - std::pow((-2.f) * x + 2.f, 2.f)) + 1.f) / 2.f;
	}

	easings_api in_back(float x) {
		constexpr float c1 = 1.70158f;
		constexpr float c3 = c1 + 1.f;

		return c3 * x * x * x - c1 * x * x;
	}
	easings_api out_back(float x) {
		constexpr float c1 = 1.70158f;
		constexpr float c3 = c1 + 1.f;

		return 1.f + c3 * std::pow(x - 1.f, 3.f) + c1 * std::pow(x - 1.f, 2.f);
	}
	easings_api in_out_back(float x) {
		constexpr float c1 = 1.70158f;
		constexpr float c2 = c1 * 1.525f;

		return x < 0.5f
					   ? (std::pow(2.f * x, 2.f) * ((c2 + 1.f) * 2.f * x - c2)) / 2.f
					   : (std::pow(2.f * x - 2.f, 2.f) * ((c2 + 1.f) * (x * 2.f - 2.f) + c2) + 2.f) / 2.f;
	}

	easings_api in_elastic(float x) {
		constexpr float c4 = (2.f * fPI) / 3.f;

		return x == 0.f
					   ? 0.f
			   : x == 1.f
					   ? 1.f
					   : (-std::pow(2.f, 10.f * x - 10.f)) * std::sin((x * 10.f - 10.75f) * c4);
	}
	easings_api out_elastic(float x) {
		constexpr float c4 = (2.f * fPI) / 3.f;

		return x == 0.f
					   ? 0.f
			   : x == 1.f
					   ? 1.f
					   : std::pow(2.f, (-10.f) * x) * std::sin((x * 10.f - 0.75f) * c4) + 1.f;
	}
	easings_api in_out_elastic(float x) {
		constexpr float c5 = (2.f * fPI) / 4.5f;

		return x == 0.f
					   ? 0.f
			   : x == 1.f
					   ? 1.f
			   : x < 0.5f
					   ? -(std::pow(2.f, 20.f * x - 10.f) * std::sin((20.f * x - 11.1250f) * c5)) / 2.f
					   : (std::pow(2.f, (-20.f) * x + 10.f) * std::sin((20.f * x - 11.1250f) * c5)) / 2.f + 1.f;
	}

	easings_api out_bounce(float x) {
		constexpr float n1 = 7.5625;
		constexpr float d1 = 2.75;

		if (x < 1.f / d1)
			return n1 * x * x;
		else if (x < 2.f / d1)
			return n1 * (x - 1.5f / d1) * x + 0.75f;
		else if (x < 2.5f / d1)
			return n1 * (x - 2.25f / d1) * x + 0.9375f;
		else
			return n1 * (x - 2.625f / d1) * x + 0.984375f;
	}
	easings_api in_bounce(float x) {
		return 1.f - out_bounce(1.f - x);
	}
	easings_api in_out_bounce(float x) {
		return x < 0.5
					   ? (1.f - out_bounce(1.f - 2.0f * x)) / 2.f
					   : (1.f + out_bounce(2.f * x - 1.0f)) / 2.f;
	}
} // namespace easings