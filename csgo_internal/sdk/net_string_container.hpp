#pragma once
#include "../src/utils/displacement.hpp"

namespace sdk {
	struct net_string_table_t {
		VFUNC(add_string(bool is_server, const char* value, int length = -1, const void* userdata = nullptr),
			  int(__thiscall*)(decltype(this), bool, const char*, int, const void*), 8, is_server, value, length, userdata);
	};

	struct net_string_table_container_t {
		VFUNC(find_table(const char* name), net_string_table_t*(__thiscall*)(decltype(this), const char*), 3, name);
	};
} // namespace sdk