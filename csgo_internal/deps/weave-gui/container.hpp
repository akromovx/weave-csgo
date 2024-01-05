#pragma once
#include "utils/common.hpp"

namespace NS_GUI {
	class i_container : public i_renderable {
	public:
		HELD_VALUE(nullptr);

		virtual void add(i_renderable* r) = 0;
		virtual void clear() = 0;
		virtual i_renderable* find(std::function<bool(i_renderable*)> cond) = 0;
		virtual const std::vector<i_renderable*>& get_items() const = 0;
	};
} // namespace NS_GUI