#pragma once
#include "recv_props.hpp"
#include "../src/interfaces.hpp"

namespace sdk {
	void recv_prop_hook_t::hook(hash_t table, hash_t netvar, recv_var_proxy_fn user_proxy_fn) {
		for (auto client_class = interfaces::client->get_client_classes();
			 client_class != nullptr;
			 client_class = client_class->m_next_ptr) {

			if (fnva1(client_class->m_network_name) != table)
				continue;

			auto class_table = client_class->m_recvtable_ptr;

			for (int i = 0; i < class_table->m_props_count; ++i) {
				auto prop = &class_table->m_props[i];

				if (prop == nullptr)
					continue;

				if (fnva1(prop->m_prop_name) != netvar)
					continue;

				m_target_property = prop;
				m_original_proxy_fn = prop->m_proxy_fn;
				m_target_property->m_proxy_fn = user_proxy_fn;
				return;
			}
		}
	}
} // namespace sdk