#pragma once
#include "../renderable.hpp"
#include <format>
#include <functional>
#include <string>

namespace NS_GUI::popups {
	class popup_t;
}

namespace NS_GUI::controls {
	struct i_control : public i_renderable {
		static constexpr float control_height = 32.f;

		i_renderable* clone() const override {
			return nullptr;
		}
	};

	struct image_t : public i_control {
	protected:
		void* m_texture{};
		float m_rotation{};
		bool m_rounded{};

	public:
		image_t(void* texture, vec2d size, bool rounded = false, float rotation = 0.f) {
			m_texture = texture;
			m_size = size;
			m_rotation = rotation;
			m_rounded = rounded;
		}

		HELD_VALUE(nullptr);
		ITEM_TYPE_CONTROL;
		CLONE_METHOD(image_t);

		void render(render_context_t& ctx) override;
	};

	struct loading_t : public i_control {
	protected:
		std::function<bool()> m_is_loading = []() { return true; };
		float m_cycle{};
		animation_t m_show_animation{};

	public:
		loading_t(std::function<bool()> is_loading = nullptr) {
			m_is_loading = is_loading;
		}

		HELD_VALUE(nullptr);
		ITEM_TYPE_CONTROL;
		CLONE_METHOD(loading_t);

		void render(render_context_t& ctx) override;
	};

	struct text_t : public i_control {
	protected:
		std::string m_value{};
		bool m_bold{};

	public:
		text_t(const std::string& text, bool bold = false) {
			m_value = text;
			m_bold = bold;
			m_low_height = true; // ???
		}

		HELD_VALUE(nullptr);
		ITEM_TYPE_CONTROL;
		CLONE_METHOD(text_t);

		void render(render_context_t& ctx) override;
	};

	struct checkbox_t : public i_control {
	protected:
		static constexpr vec2d field_size = { 28.f, 16.f };
		static constexpr float field_padding_x = 2.f;
		//static constexpr float checkmark_size = 6.f;
		static constexpr vec2d checkmark_size = { 6.f, 6.f };
		static constexpr float checkmark_animation_mult = 0.1f;

		bool* m_value{};
		std::string m_name{};
		animation_t m_hover_animation{}, m_switch_animation{};
		callback_t m_on_change{};

	public:
		checkbox_t(const std::string& name, bool* value, callback_t on_change = nullptr) {
			m_value = value;
			m_name = name;
			m_on_change = on_change;
			m_low_height = true;
		}

		HELD_VALUE(m_value);
		ITEM_TYPE_CONTROL;
		//CLONE_METHOD(checkbox_t);

		i_renderable* clone() const override {
			auto result = new checkbox_t{ m_name, m_value, m_on_change };
			result->m_style = this->m_style;
			return result;
		}

		void set_callback(callback_t callback) { m_on_change = callback; }
		void render(render_context_t& ctx) override;
	};

	struct button_t : public i_control {
	protected:
		callback_t m_callback{};
		std::string m_name{};
		animation_t m_animation{};
		e_items_align m_text_align{};
		void* m_icon{};
		bool m_colored{};

		vec2d m_original_size;

	public:
		button_t() = default;

		button_t(const std::string& name, callback_t callback, vec2d size = { 0.f, 0.f }, e_items_align text_align = e_items_align::center, void* icon = nullptr, bool colored = false) {
			m_callback = callback;
			m_name = name;
			m_size = size;
			m_text_align = text_align;
			m_icon = icon;
			m_colored = colored;

			m_original_size = size / dpi::_get_actual_scale();
		}

		void set_callback(callback_t callback) { m_callback = callback; }

		HELD_VALUE(nullptr);
		ITEM_TYPE_CONTROL;

		i_renderable* clone() const override {
			auto result = new button_t{ m_name, m_callback, m_original_size * dpi::_get_actual_scale(), m_text_align, m_icon, m_colored };
			result->m_style = this->m_style;
			return result;
		}

		void render(render_context_t& ctx) override;
	};

	struct collapse_t : public button_t {
	protected:
		i_renderable* m_item{};
		bool m_opened{};
		animation_t m_open_animation{};
		vec2d m_actual_size{};

	public:
		collapse_t(const std::string& name, callback_t callback, vec2d size = { 0.f, 0.f }, void* icon = nullptr) {
			m_callback = callback;
			m_name = name;
			m_actual_size = size;
			m_text_align = e_items_align::start;
			m_icon = icon;
			m_colored = false;
		}

		void set_item(i_renderable* item) { m_item = item; }
		void render(render_context_t& ctx) override;
	};

	struct selectables_t : public i_control {
	protected:
		int* m_value{};
		animation_t m_hover_animation{};
		std::vector<std::string> m_items{};
		std::vector<std::pair<animation_t, animation_t>> m_items_animation{};
		popups::popup_t* m_popup{};
		std::function<void(bool)> m_on_click{ nullptr };
		float m_width{};
		bool m_multiselect{};
		std::string* m_search_phrase{};

	public:
		selectables_t(int* value, const std::vector<std::string>& items, bool multiselect = false, float width = -1.f, std::function<void(bool)> on_click = nullptr, std::string* search_phrase = nullptr) {
			m_value = value;
			m_items = items;
			m_items_animation.resize(m_items.size());
			std::memset(m_items_animation.data(), 0, m_items_animation.size() * sizeof(std::pair<animation_t, animation_t>));
			m_width = width;
			m_on_click = on_click;
			m_multiselect = multiselect;
			m_search_phrase = search_phrase;
		}

		HELD_VALUE(m_value);
		ITEM_TYPE_CONTROL;
		CLONE_METHOD(selectables_t);

		void render(render_context_t& ctx) override;
	};

	struct dropbox_t : public i_control {
	protected:
		std::string m_name{};
		int* m_value{};
		animation_t m_hover_animation{}, m_open_animation{};
		std::vector<std::string> m_items{};
		popups::popup_t* m_popup{};
		std::function<void(bool)> m_on_click{ nullptr };
		bool m_multiselect{};
		std::string m_search_phrase{};

	public:
		dropbox_t(const std::string& name, int* value, const std::vector<std::string>& items, bool multiselect, std::function<void(bool)> on_click = nullptr) {
			if (multiselect && items.size() > 32) {
				MessageBoxA(0, STRSC("Multi-dropbox can not include more than 32 items!"), STRSC("Error"), MB_ICONERROR | MB_SETFOREGROUND);
				return;
			}

			m_value = value;
			m_name = name;
			m_items = items;
			m_on_click = on_click;
			m_multiselect = multiselect;
		}

		HELD_VALUE(m_value);
		ITEM_TYPE_CONTROL;
		CLONE_METHOD(dropbox_t);

		void render(render_context_t& ctx) override;
	};

	struct slider_t : public i_control {
	protected:
		std::string m_name{};
		int* m_value{};
		bool m_holding{};
		animation_t m_hover_animation{}, m_slider_animation{};
		int m_min{}, m_max{};
		float m_slider_value{};
		animation_t m_visual_progress{};

		static constexpr auto slider_size = vec2d{ 6.f, 12.f };
		std::function<std::string(int)> m_format = default_format;
		callback_t m_callback{};

	public:
		static std::string default_format(int value) { return std::format("{}", value); };

		slider_t(const std::string& name, int* value, int min, int max, std::function<std::string(int)> format = default_format) {
			m_value = value;
			m_name = name;
			m_min = min;
			m_max = max;
			m_format = format;
		}

		HELD_VALUE(m_value);
		ITEM_TYPE_CONTROL;
		i_renderable* clone() const override {
			auto result = new slider_t{ m_name, m_value, m_min, m_max, m_format };
			result->m_style = this->m_style;
			return result;
		}

		void set_format(std::function<std::string(int)> format) { m_format = format; }
		void set_callback(callback_t callback) { m_callback = callback; }

		void render(render_context_t& ctx) override;
	};

	struct text_input_t : public i_control {
	protected:
		std::string m_hint{};
		std::string* m_value{};
		animation_t m_animation{}, m_active_animation{};
		pulsating_t m_cursor_pulsating{};
		float m_cursor_animated{};
		float m_last_time_typed{};

		bool m_active{};
		bool m_password{};

		callback_t m_on_change_value{};
		std::function<void(bool)> m_on_change_state{};

		struct {
			size_t m_begin{}, m_end{}, m_cursor{};

			void shrink_to_begin() { m_end = m_begin; }
			void shrink_to_end() { m_begin = m_end; }
			bool is_active() const { return m_begin != m_end; }

			void clamp(size_t len) {
				m_begin = std::clamp(m_begin, 0u, len);
				m_end = std::clamp(m_end, 0u, len);
				if (m_begin > m_end) m_end = m_begin;
				if (m_end < m_begin) m_begin = m_end;
			}
		} m_selection;

	public:
		text_input_t(std::string* value, const std::string& hint = "Write a prase...", bool password = false) {
			m_hint = hint;
			m_value = value;
			m_password = password;
		}
		~text_input_t();

		void on_change_state(std::function<void(bool)> callback) { m_on_change_state = callback; }
		void on_change_value(callback_t callback) { m_on_change_value = callback; }

		HELD_VALUE(m_value);
		ITEM_TYPE_CONTROL;
		CLONE_METHOD(text_input_t);

		void render(render_context_t& ctx) override;

		float last_time_typed() const;
	};

	struct weapon_selector_t : public i_control {
	protected:
		animation_t m_hover_animation{}, m_open_animation{};
		popups::popup_t* m_popup{};

		int* m_group_id{};
		int* m_weapon_id{};

		int* m_weapon_array{};

		callback_t m_on_click{};

	public:
		weapon_selector_t(int* group_id, int* weapon_id, callback_t on_click) {
			m_group_id = group_id;
			m_weapon_id = weapon_id;
			m_on_click = on_click;

			m_weapon_array = new int[9]{};
			m_weapon_array[*m_group_id] = *m_weapon_id;
		}
		~weapon_selector_t() { delete[] m_weapon_array; }

		HELD_VALUE(nullptr);
		ITEM_TYPE_CONTROL;
		CLONE_METHOD(weapon_selector_t);

		void render(render_context_t& ctx) override;
	};

	struct slider_impl_t {
		static constexpr int m_min = 0, m_max = 1000;
		bool m_holding{};

		animation_t m_hover_animation{};
		float m_value{};
		int* m_pvalue{};

		float get_progress() const { return (float)(*m_pvalue - m_min) / (float)(m_max - m_min); }

		void update(render_context_t& ctx, i_renderable* parent, const vec2d& position, const vec2d& size, bool& hovered);
	};

	struct slider2d_impl_t {
		static constexpr int m_min = 0, m_max = 1000;
		bool m_holding{};

		animation_t m_hover_animation{};
		vec2d m_value{};

		int* m_px{};
		int* m_py{};

		float get_progress_x() const { return (float)(*m_px - m_min) / (float)(m_max - m_min); }
		float get_progress_y() const { return (float)(*m_py - m_min) / (float)(m_max - m_min); }

		void update(render_context_t& ctx, i_renderable* parent, const vec2d& position, const vec2d& size, bool& hovered);
	};

	struct colorpicker_impl_t : public i_control {
	protected:
		color_t* m_value{};

		bool m_modify_alpha{};
		callback_t m_on_change{};

		slider_impl_t m_hue_slider{}, m_alpha_slider{};
		slider2d_impl_t m_sb_slider{};
		int m_hue{}, m_saturation, m_brightness{}, m_alpha{};

	public:
		colorpicker_impl_t(color_t* value, bool modify_alpha) {
			m_value = value;
			m_modify_alpha = modify_alpha;

			// bind variables to slider impl
			m_hue_slider.m_pvalue = &m_hue;
			m_alpha_slider.m_pvalue = &m_alpha;
			m_sb_slider.m_px = &m_saturation;
			m_sb_slider.m_py = &m_brightness;

			update_cache();
		}

		void update_cache() {
			// set our temporary variables
			m_hue = (int)(m_value->hue() * 1000.f);
			m_saturation = (int)(m_value->saturation() * 1000.f);
			m_brightness = 1000 - (int)(m_value->brightness() * 1000.f);
			m_alpha = (int)(m_value->a() / 255.f * 1000.f);
		}

		void on_change(callback_t callback) { m_on_change = callback; }

		HELD_VALUE(m_value);
		ITEM_TYPE_CONTROL;

		i_renderable* clone() const override {
			auto result = new colorpicker_impl_t{ m_value, m_modify_alpha };
			result->m_style = this->m_style;
			return result;
		}

		void render(render_context_t& ctx) override;
	};

	struct colorpicker_t : public i_control {
	protected:
		std::string m_label{};
		color_t* m_value{};
		animation_t m_hover_animation{}, m_open_animation{};
		popups::popup_t* m_popup{};
		bool m_modify_alpha{};
		std::string m_color_text{};

	public:
		colorpicker_t(const std::string& label, color_t* value, bool modify_alpha = true) {
			m_label = label;
			m_value = value;
			m_modify_alpha = modify_alpha;
			m_low_height = true;
		}

		HELD_VALUE(m_value);
		ITEM_TYPE_CONTROL;
		i_renderable* clone() const override {
			auto result = new colorpicker_t{ m_label, m_value, m_modify_alpha };
			result->m_style = this->m_style;
			return result;
		}

		void render(render_context_t& ctx) override;
	};

	struct key_binder_t : public i_control {
	protected:
		uint16_t* m_value{};
		animation_t m_hover_animation{}, m_open_animation{};
		bool m_active{};
		std::function<void()> m_on_bind{};

	public:
		key_binder_t(uint16_t* value, std::function<void()> on_bind = nullptr) {
			m_value = value;
			m_on_bind = on_bind;
		}

		HELD_VALUE(m_value);
		ITEM_TYPE_CONTROL;
		CLONE_METHOD(key_binder_t);

		void render(render_context_t& ctx) override;
	};

	struct web_image_t : public i_control {
	protected:
		float m_cycle{};
		std::atomic_bool m_downloaded{};
		bool m_locked{};
		void* m_gif{};
		std::optional<vec2d> m_force_size = std::nullopt;
		bool m_rounded{};
		animation_t m_show_animation{};

	public:
		web_image_t(std::function<std::vector<uint8_t>()> download, float scale = 1.f, std::optional<vec2d> force_size = std::nullopt, bool rounded = false);
		virtual ~web_image_t() {
			if (m_downloaded) clear_gif(m_gif);
		}

		HELD_VALUE(nullptr);
		ITEM_TYPE_CONTROL;

		void render(render_context_t& ctx) override;
	};

	struct skin_selector_t : public i_control {
	protected:
		float m_cycle{};
		std::atomic_bool m_loaded{};
		LPDIRECT3DTEXTURE9 m_texture{};
		std::function<void()> m_callback{};
		int m_rarity{};
		animation_t m_hover_animation{};

	public:
		skin_selector_t(std::function<void*()> get_texture, std::function<void()> callback, int rarity = -1) {
			m_callback = callback;
			m_rarity = rarity;

			std::thread([this, get_texture]() {
				m_texture = (LPDIRECT3DTEXTURE9)get_texture();
				m_loaded = true;
			}).detach();
		}
		~skin_selector_t() { clear_texture(m_texture); }

		HELD_VALUE(nullptr);
		ITEM_TYPE_CONTROL;

		void render(render_context_t& ctx) override;
	};

	struct gif_t : public i_control {
	protected:
		void* m_gif{};
		float m_rotation{};
		bool m_rounded{};
		std::function<void()> m_on_click{};

	public:
		gif_t(void* gif, vec2d size, bool rounded = false, std::function<void()> on_click = nullptr) {
			m_gif = gif;
			m_size = size;
			m_rounded = rounded;
			m_on_click = on_click;
		}
		~gif_t() { clear_gif(m_gif); }

		HELD_VALUE(nullptr);
		ITEM_TYPE_CONTROL;
		CLONE_METHOD(gif_t);

		void render(render_context_t& ctx) override;
	};

	inline auto image(void* texture, vec2d size, bool rounded = false, float rotation = 0.f) {
		return new image_t{ texture, size, rounded, rotation };
	};

	inline auto loading(std::function<bool()> is_loading = nullptr) {
		return new loading_t{ is_loading };
	};

	inline auto text(const std::string& text, bool bold = false) {
		return new text_t{ text, bold };
	};

	inline auto checkbox(const std::string& name, bool* value, callback_t on_change = nullptr) {
		return new checkbox_t{ name, value, on_change };
	};

	inline auto button(const std::string& name, callback_t callback, vec2d size = { 0.f, 0.f }, e_items_align text_align = e_items_align::center, void* icon = nullptr, bool colored = false) {
		return new button_t{ name, callback, size, text_align, icon, colored };
	};

	inline auto collapse(const std::string& name, callback_t callback, vec2d size = { 0.f, 0.f }, void* icon = nullptr) {
		return new collapse_t{ name, callback, size, icon };
	};

	inline auto selectables(int* value, const std::vector<std::string>& items, bool multiselect = false, float width = -1.f, std::function<void(bool)> on_click = nullptr, std::string* search_phrase = nullptr) {
		return new selectables_t{ value, items, multiselect, width, on_click, search_phrase };
	};

	inline auto dropbox(const std::string& name, int* value, const std::vector<std::string>& items, bool multiselect, std::function<void(bool)> on_click = nullptr) {
		return new dropbox_t{ name, value, items, multiselect, on_click };
	};

	inline auto slider(const std::string& name, int* value, int min, int max, std::function<std::string(int)> format = slider_t::default_format) {
		return new slider_t{ name, value, min, max, format };
	};

	inline auto text_input(std::string* value, const std::string& hint = STRS("Write a prase..."), bool password = false) {
		return new text_input_t{ value, hint, password };
	};

	inline auto weapon_selector(int* group_id, int* weapon_id, callback_t on_click) {
		return new weapon_selector_t{ group_id, weapon_id, on_click };
	};

	inline auto colorpicker(const std::string& label, color_t* value, bool modify_alpha = true) {
		return new colorpicker_t{ label, value, modify_alpha };
	};

	inline auto key_binder(uint16_t* value, std::function<void()> on_bind = nullptr) {
		return new key_binder_t{ value, on_bind };
	};

	inline auto web_image(std::function<std::vector<uint8_t>()> download, float scale = 1.f, std::optional<vec2d> force_size = std::nullopt, bool rounded = false) {
		return new web_image_t{ download, scale, force_size, rounded };
	};

	inline auto skin_selector(std::function<void*()> get_texture, std::function<void()> callback, int rarity = -1) {
		return new skin_selector_t{ get_texture, callback, rarity };
	};

	inline auto gif(void* gif, vec2d size, bool rounded = false, std::function<void()> on_click = nullptr) {
		return new gif_t{ gif, size, rounded, on_click };
	};

	i_renderable* prompt(const std::string& name, const std::string& value);
	i_renderable* hotkey(const std::string& name, int* type, uint16_t* key, void* category = nullptr, std::function<void()> nested_items = nullptr, std::function<void()> on_bind = nullptr);
} // namespace NS_GUI::controls