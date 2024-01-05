#pragma once
#include "utils/common.hpp"
#include <time.h>

namespace NS_GUI {
	namespace popups {
		class popup_t;
	}

	class instance_t {
	protected:
		std::vector<i_renderable*> m_items{};
		render_context_t m_render_context{};
		int m_locks{};

	public:
		virtual void render() = 0;
		virtual void reset() = 0;
		virtual popups::popup_t* create_popup() = 0;
		virtual bool delete_popup(popups::popup_t* p) = 0;
		virtual bool has_popups() const = 0;
		virtual vec2d get_position() const = 0;
		virtual popups::popup_t* active_popup() const = 0;

		void lock() { m_locks++; }
		bool is_locked() const { return m_locks != 0; }
		void unlock() { m_locks--; }
	};

	struct globals_t {
		float m_time_scale{ 1.0f };

		float time_delta{};
		float m_old_time{};

		float mouse_wheel{};
		vec2d mouse_position{};
		vec2d mouse_delta{};
		bool mouse_down[5]{};
		bool mouse_click[5]{};

		int arrow{};

		void reset() {
			arrow = 0;
		}

		bool is_mouse_in_region(vec2d position, vec2d size) const {
			return mouse_position.x >= position.x && mouse_position.x <= position.x + size.x && mouse_position.y >= position.y && mouse_position.y <= position.y + size.y;
		}

		void reset_mouse() {
			std::memset(mouse_down, 0, sizeof(mouse_down));
			std::memset(mouse_click, 0, sizeof(mouse_click));
		}

		float get_time() const {
			return (float)clock() / 1000.f;
		}
	};

	__forceinline globals_t& globals() {
		static globals_t g{};
		return g;
	}
} // namespace NS_GUI