#pragma once
#include "utils/common.hpp"

namespace NS_GUI {
	namespace msgbox {
		void render();
		void add(const std::string& str);

		inline std::atomic_bool is_opened = false;
	}// namespace msgbox
}// namespace NS_GUI