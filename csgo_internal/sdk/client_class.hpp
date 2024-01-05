#pragma once
#include "recv_props.hpp"
#include "class_id.hpp"

namespace sdk {
	struct client_class_t;
	struct networkable_t;
	struct client_mode_t;

	using create_client_class_t = networkable_t* (*)(int ent_number, int serial_number);
	using create_event_t = networkable_t* (*)();

	struct client_class_t {
		create_client_class_t m_create_fn;
		create_event_t m_create_event_fn;
		char* m_network_name;
		recv_table_t* m_recvtable_ptr;
		client_class_t* m_next_ptr;
		e_class_id m_class_id;
	};
} // namespace sdk