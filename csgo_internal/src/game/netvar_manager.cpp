#include "netvar_manager.hpp"
#include "../interfaces.hpp"

using namespace sdk;

void netvar_manager_t::init() {
	m_tables.clear();

	auto client_class = interfaces::client->get_client_classes();
	if (client_class == nullptr)
		return;

	while (client_class != nullptr) {
		m_tables.push_back(client_class->m_recvtable_ptr);
		client_class = client_class->m_next_ptr;
	}
}

uint32_t netvar_manager_t::get_offset(uint32_t table_hash, uint32_t prop_hash, recv_prop_t** prop) {
	return get_prop(get_table(table_hash), prop_hash, prop);
}

int netvar_manager_t::get_prop(recv_table_t* table, uint32_t prop_hash, recv_prop_t** prop) {
	if (table == nullptr)
		return 0;

	int offset = 0;
	for (int i = 0; i < table->m_props_count; ++i) {
		auto recv_prop = &table->m_props[i];
		auto child = recv_prop->m_data_table;

		if (child != nullptr && child->m_props_count > 0) {
			int extra_offset = get_prop(child, prop_hash, prop);
			if (extra_offset)
				offset += recv_prop->m_offset + extra_offset;
		}

		if (fnva1(recv_prop->m_prop_name) != prop_hash)
			continue;

		if (prop != nullptr)
			*prop = recv_prop;

		return recv_prop->m_offset + offset;
	}

	return offset;
}

recv_table_t* netvar_manager_t::get_table(uint32_t table_hash) {
	if (m_tables.empty())
		return nullptr;

	for (auto table: m_tables) {
		if (table == nullptr)
			continue;

		if (fnva1(table->m_table_name) == table_hash)
			return table;
	}

	return nullptr;
}

bool netvar_manager_t::hook_prop(uint32_t table_hash, uint32_t prop_hash, recv_var_proxy_fn fn) {
	auto table = get_table(table_hash);
	if (!table)
		return false;

	recv_prop_t* prop = nullptr;
	get_prop(table, prop_hash, &prop);

	if (prop == nullptr)
		return false;

	prop->m_proxy_fn = fn;
	return true;
}
