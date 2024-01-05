#pragma once
#include "../../../src/macros.hpp"

#define NS_GUI gui

#define INVALID_POS \
	vec2d { NAN, NAN }

#define ITEM_TYPE(type) \
	e_item_type get_item_type() { return type; }

#define ITEM_TYPE_SHAPE ITEM_TYPE(e_item_type::shape)
#define ITEM_TYPE_CONTAINER ITEM_TYPE(e_item_type::container)
#define ITEM_TYPE_CONTROL ITEM_TYPE(e_item_type::control)
#define ITEM_TYPE_POPUP ITEM_TYPE(e_item_type::popup)
#define ITEM_TYPE_NAVNODE ITEM_TYPE(e_item_type::navigation_node)
#define ITEM_TYPE_NAV ITEM_TYPE(e_item_type::navigation)

#define HELD_VALUE(x) \
	void* value_pointer() const override { return x; }

#define CLONE_METHOD(type) \
	i_renderable* clone() const override { return new type{ *this }; }