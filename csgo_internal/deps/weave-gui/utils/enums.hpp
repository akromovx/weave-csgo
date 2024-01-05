#pragma once
#include "../../../src/utils/fnva1.hpp"
#include "macros.hpp"

namespace NS_GUI {
	enum class e_item_type : uint32_t {
		shape = HASH("gui:item-shape"),
		control = HASH("gui:item-control"),
		container = HASH("gui:item-container"),
		popup = HASH("gui:item-popup"),
		navigation_node = HASH("gui:item-navigation_node"),
		navigation = HASH("gui:item-navigation")
	};

	enum class e_position_type : uint32_t {
		absolute = HASH("gui:position-absolute"),
		relative = HASH("gui:position-relative")
	};

	enum class e_items_direction : uint32_t {
		horizontal = HASH("gui:directional-horizontal"),
		vertical = HASH("gui:directional-vertical")
	};

	enum class e_items_align : uint32_t {
		start = HASH("gui:align-start"),
		end = HASH("gui:align-end"),
		center = HASH("gui:align-center"),
		stretch = HASH("gui:align-stretch")
	};

	enum e_container_flags : uint32_t {
		/* general containers' flags */

		container_flag_none = 0,
		container_flag_visible = 1 << 0,
		container_flag_can_resize = 1 << 1,
		container_flag_can_move = 1 << 2,
		container_flag_is_popup = 1 << 3,
		container_no_padding = 1 << 4,

		/* popups flags */

		popup_flag_close_on_click = 1 << 5,
		popup_flag_open_animation = 1 << 6,
		popup_flag_close_animation = 1 << 7,
		popup_flag_animation = popup_flag_open_animation | popup_flag_close_animation,
	};
}// namespace NS_GUI