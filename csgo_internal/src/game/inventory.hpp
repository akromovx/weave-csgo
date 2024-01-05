#pragma once
#include "../base_includes.hpp"

struct inventory_t {
	void on_create_move();

	__forceinline auto& get_items() { return m_items; };
	__forceinline const auto& get_items() const { return m_items; };

private:
	std::vector<short> m_items{};
};

GLOBAL_DYNPTR(inventory_t, inventory);