#pragma once
#include "../instance.hpp"
#include "../popups/popups.hpp"

namespace NS_GUI::instance {
	class window_t : public instance_t {
	public:
		using override_render_function_t = std::function<void(render_context_t&, window_t*)>;

	protected:
		static constexpr auto sidebar_width = 192.f;

		vec2d m_size{};
		vec2d m_position{};
		bool m_dragging{};
		bool m_can_drag{};
		e_items_align m_items_align{};
		std::vector<popups::popup_t*> m_popups{};

		callback_t m_update_callback{ nullptr };
		bool m_silent_update = false;
		animation_t m_show_animation{};
		bool m_loading{};
		float m_loading_cycle{ 0.5f };
		float m_loading_start{};
		std::string m_loading_text{};
		float m_alpha{};

		override_render_function_t m_override_render_function{};
		scroll_t m_scroll{};
		pulsating_t m_skeleton_pulsating{};
		std::vector<popups::popup_t*> m_popups_to_delete{};

	public:
		bool m_can_update{};
		bool m_opened{};

		window_t(vec2d size, vec2d position, e_items_align items_align = e_items_align::center) : m_size(size), m_position(position) {
			m_items_align = items_align;
		}

		ITEM_TYPE_CONTAINER;
		bool update();
		void render() override;
		bool hovered() const { return globals().is_mouse_in_region(m_position, m_size); }

		void add(i_renderable* r) { m_items.emplace_back(r); }
		void clear() { containers::recursive_delete(m_items); }
		const std::vector<i_renderable*>& get_items() const { return m_items; }

		// Setting the loading animation & hide all items
		// If it's already loading this function will change the text
		void set_loading(std::string loading_text = "") {
			this->reset();
			m_loading_text = loading_text;

			if (m_loading_start == 0.f)
				m_loading_start = globals().get_time();
		}

		void set_position(vec2d pos) {
			m_position = pos;
		}

		float get_alpha() const {
			return m_alpha;
		}

		void stop_loading() {
			m_loading = false;
			m_loading_start = 0.f;
		};

		void set_size(vec2d size) { m_size = size; }
		vec2d get_size() const { return m_size; }

		vec2d get_position() const override { return m_position; }

		// This function delete all items and run passed callback
		// If silent is enabled there won't the showing animation
		void update_state(std::function<void()> callback, bool silent = false) {
			m_update_callback = callback;
			m_silent_update = silent;
		}

		void reset() override {
			m_render_context.m_alpha = 0.f;
			m_show_animation = {};
			m_scroll.reset();
			m_loading = true;
		};

		popups::popup_t* create_popup() override {
			return m_popups.emplace_back(new popups::popup_t{ this });
		};

		popups::popup_t* active_popup() const override {
			if (m_popups.empty())
				return nullptr;

			return m_popups.back();
		};

		bool delete_popup(popups::popup_t* popup) override {
			auto it = std::find_if(m_popups.begin(), m_popups.end(), [&](popups::popup_t* p) { return p == popup; });
			if (it != m_popups.end()) {
				m_popups_to_delete.push_back(*it);
				m_popups.erase(it);
				return true;
			}

			return false;
		};

		bool has_popups() const override {
			return !m_popups.empty();
		}

		bool dragging() const { return m_dragging; }
		bool can_drag() const { return m_can_drag; }

		void override_render_function(override_render_function_t callback) { m_override_render_function = callback; }
	};
} // namespace NS_GUI::instance