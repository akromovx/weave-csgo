#pragma once
#include "base_includes.hpp"

#ifdef _DEBUG
#define PUSH_LOG(...) printf(__VA_ARGS__)
#else
#define PUSH_LOG(...) cheat::debug_log(__VA_ARGS__)
#endif

namespace cheat {
	extern void init(LPVOID thread_parameter);
	extern void unload();
	extern void debug_log(const char* fmt, ...);
} // namespace cheat