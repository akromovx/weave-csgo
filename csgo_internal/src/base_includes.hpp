#pragma once
#include <Windows.h>
#include <algorithm>
#include <array>
#include <chrono>
#include <d3d9.h>
#include <d3dx9.h>
#include <format>
#include <functional>
#include <iostream>
#include <memory>
#include <numeric>
#include <thread>
#include <vector>

#ifdef __NO_OBF
#define CHEAT_INIT inline
#define STFI static inline
#define STI static inline
#else
#define CHEAT_INIT __forceinline
#define STFI static __forceinline
#define STI static inline
#endif

#ifdef _DEBUG
#define GLOBAL_DYNPTR(struct_name, object_name) inline auto object_name = std::make_unique<struct_name>()
#else
// TODO: add pointer xor
#define GLOBAL_DYNPTR(struct_name, object_name) inline auto object_name = std::make_unique<struct_name>()
#endif

#include "utils/fnva1.hpp"
#include "utils/obfuscation.hpp"
#include "vars.hpp"

struct cheat_t {
	bool unload = false;
	HWND hwnd;
	HMODULE m_module;

	inline void perform_update() {
		m_last_update = clock();
	}

	bool is_valid() const;
	clock_t m_last_update{};
};

template<typename... args_t>
static __forceinline std::string dformat(std::string_view rt_fmt_str, args_t&&... args) {
	return std::vformat(rt_fmt_str, std::make_format_args(std::forward<args_t>(args)...));
}

namespace fs = std::filesystem;
namespace rv = std::ranges::views;
namespace ranges = std::ranges;

GLOBAL_DYNPTR(cheat_t, ctx);