#pragma once
#include "inventory.hpp"
#include "../globals.hpp"

using namespace sdk;

void inventory_t::on_create_move() {
	m_items.clear();

	for (auto weapon: globals->m_local->get_weapons())
		m_items.emplace_back(weapon->item_definition_index());
}
