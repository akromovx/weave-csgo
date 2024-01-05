#pragma once
#include "../containers/containers.hpp"

namespace NS_GUI::popups {
	class popup_t : public containers::group_t {
	protected:
		instance_t* m_instance{};
		std::function<void()> m_on_delete{ nullptr };
		bool m_closed{};
		vec2d m_visual_size{};
		animation_t m_show_animation{};
		void remove();

	public:
		flags_t m_flags{};
		popup_t(instance_t* instance, flags_t flags = container_flag_none) {
			m_position_type = e_position_type::absolute;
			m_flags = flags;
			m_instance = instance;
		};

		void set_position(vec2d position) {
			m_position = position.round();
		}

		void set_size(vec2d size) {
			m_size = size.round();
		}

		void on_delete(std::function<void()> callback) {
			m_on_delete = callback;
		}

		ITEM_TYPE_POPUP;
		CLONE_METHOD(popup_t);

		void render(render_context_t& ctx) override;
		void close();
	};
} // namespace NS_GUI::popups