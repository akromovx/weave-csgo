#pragma once
#include "../../sdk/recv_props.hpp"
#include "../base_includes.hpp"

class netvar_manager_t {
private:
	std::vector<sdk::recv_table_t*> m_tables;
	int get_prop(sdk::recv_table_t* table, uint32_t prop_name, sdk::recv_prop_t** prop = 0);
	sdk::recv_table_t* get_table(uint32_t table_name);

public:
	void init();
	uint32_t get_offset(uint32_t table_hash, uint32_t prop_hash, sdk::recv_prop_t** prop = 0);
	bool hook_prop(uint32_t table_name, uint32_t prop_name, sdk::recv_var_proxy_fn fn);
};

GLOBAL_DYNPTR(netvar_manager_t, netvars);