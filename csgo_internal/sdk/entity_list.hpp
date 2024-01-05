#pragma once
#include "../src/utils/utils.hpp"
#include "entities.hpp"
#include <stdint.h>

namespace sdk {
	struct entity_listener_t {
		virtual void on_entity_created(base_entity_t* entity){};
		virtual void on_entity_deleted(base_entity_t* entity){};
	};

	struct entity_list_t {
		virtual void fn0() = 0;
		virtual void fn1() = 0;
		virtual void fn2() = 0;

		virtual base_entity_t* get_client_entity(int idx) = 0;
		virtual base_entity_t* get_client_entity_handle(uint32_t handle) = 0;
		virtual int number_of_entities(bool include_non_networkable) = 0;
		virtual int get_highest_entity_index() = 0;

		__forceinline void add_listener(entity_listener_t* listener) { m_listeners.add_to_tail(listener); }
		__forceinline void remove_listener(entity_listener_t* listener) { m_listeners.find_and_remove(listener); }

	private:
		utils::utl_vector_t<entity_listener_t*> m_listeners = {};
	};
} // namespace sdk