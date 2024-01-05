#pragma once
#include "../container.hpp"
#include "../instance.hpp"
#include <string>

namespace NS_GUI::containers {
	class row_t : public i_container {
	protected:
		std::vector<i_renderable*> m_items{};
		e_items_align m_items_align{};

	public:
		row_t(e_items_align items_align = e_items_align::start) : m_items_align(items_align){};

		ITEM_TYPE_CONTAINER;

		i_renderable* clone() const override {
			auto this_clone = new row_t{ m_items_align };
			this_clone->m_style = this->m_style;

			for (auto item: m_items)
				this_clone->add(item->clone());

			return this_clone;
		}

		void render(render_context_t& ctx) override;
		void add(i_renderable* r) override { m_items.emplace_back(r); };
		const std::vector<i_renderable*>& get_items() const override { return m_items; }

		void clear() override {
			for (auto item: m_items) {
				if (item->get_item_type() == e_item_type::container)
					((i_container*)item)->clear();

				delete item;
			}

			m_items.clear();
		}

		i_renderable* find(std::function<bool(i_renderable*)> cond) override {
			for (auto item: m_items) {
				if (cond(item))
					return item;

				if (item->get_item_type() == e_item_type::container) {
					auto result = ((i_container*)item)->find(cond);
					if (result != nullptr)
						return result;
				}
			}

			return nullptr;
		}
	};

	class column_t : public i_container {
	protected:
		std::vector<i_renderable*> m_items{};
		e_items_align m_items_align{};
		callback_t m_update_callback{ nullptr };

	public:
		column_t(e_items_align items_align = e_items_align::start) : m_items_align(items_align){};

		ITEM_TYPE_CONTAINER;
		i_renderable* clone() const override {
			auto this_clone = new column_t{ m_items_align };
			this_clone->m_style = this->m_style;

			for (auto item: m_items)
				this_clone->add(item->clone());

			return this_clone;
		}

		void render(render_context_t& ctx) override;
		void add(i_renderable* r) override { m_items.emplace_back(r); };
		const std::vector<i_renderable*>& get_items() const override { return m_items; }

		void clear() override {
			for (auto item: m_items) {
				if (item->get_item_type() == e_item_type::container)
					((i_container*)item)->clear();

				delete item;
			}

			m_items.clear();
		}

		i_renderable* find(std::function<bool(i_renderable*)> cond) override {
			for (auto item: m_items) {
				if (cond(item))
					return item;

				if (item->get_item_type() == e_item_type::container) {
					auto result = ((i_container*)item)->find(cond);
					if (result != nullptr)
						return result;
				}
			}

			return nullptr;
		}

		void update_state(callback_t callback) { m_update_callback = callback; }
	};

	class group_t : public column_t {
	protected:
		std::string m_title{};
		bool m_auto_size_x{};
		bool m_auto_size_y{};

		scroll_t m_scroll{};
		callback_t m_update_callback{ nullptr };

		animation_t m_show_animation{};
		bool m_loading{};
		pulsating_t m_skeleton_pulsating{};

	public:
		group_t(std::string title = "", vec2d size = { 0.f, 0.f });

		// Setting the loading animation & hide all items
		void set_loading() { m_loading = true; }

		bool is_loading() const { return m_loading; }
		void stop_loading() { m_loading = false; };

		void render(render_context_t& ctx);
		void update_state(callback_t callback) { m_update_callback = callback; }

		i_renderable* clone() const override {
			auto this_clone = new group_t{ m_title, m_size };
			this_clone->m_loading = this->m_loading;
			this_clone->m_style = this->m_style;

			for (auto item: m_items)
				this_clone->add(item->clone());

			return this_clone;
		}
	};
} // namespace NS_GUI::containers