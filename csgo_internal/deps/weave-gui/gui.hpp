#pragma once
#include "../imgui/imgui.h"

#include "utils/enums.hpp"
#include "utils/macros.hpp"

#include "../../src/utils/animation_handler.hpp"
#include "../../src/utils/bits.hpp"
#include "../../src/utils/color.hpp"
#include "../../src/utils/easings.hpp"
#include "../../src/utils/encoding.hpp"
#include "../../src/utils/fnva1.hpp"
#include "../../src/utils/obfuscation.hpp"
#include "../../src/utils/vector.hpp"

#include "resources.hpp"
#include "style.hpp"

#include <Windows.h>
#include <concepts>
#include <future>
#include <optional>
#include <vector>

#define CREATE_TEXTURE_MEM(tex_name, bytes, bytes_size) \
	static LPDIRECT3DTEXTURE9 tex_name = NULL;          \
	if (tex_name == NULL)                               \
	D3DXCreateTextureFromFileInMemory(render::device, bytes, bytes_size, &tex_name)

#define CREATE_TEXTURE_FILE(tex_name, path)    \
	static LPDIRECT3DTEXTURE9 tex_name = NULL; \
	if (tex_name == NULL)                      \
	D3DXCreateTextureFromFileA(render::device, path, &tex_name)

namespace NS_GUI {
	static inline constexpr auto fPI = 3.14159f;
	static inline constexpr auto fPI2 = fPI * 0.5f;
	static inline constexpr auto f2PI = fPI * 2.f;

	using callback_t = std::function<void()>;

	class i_renderable;
	class instance_t;

	struct render_context_t {
		vec2d m_base_position{};
		vec2d m_cursor{};
		e_items_direction m_direction{};
		float m_alpha{};
		bool m_can_update{};
		bool m_in_popup{};

		float m_time_delta{};
		vec2d m_size{};
		float m_item_width{};

		instance_t* m_instance{};
		i_renderable* m_active_item{};
		i_renderable* m_hovered_item{};
		i_renderable* m_parent{};

		bool m_scrolled_this_frame{};

		inline void reset() {
			m_size.set();
			m_cursor.set();
			m_direction = e_items_direction::vertical;
			m_hovered_item = nullptr;
			m_scrolled_this_frame = false;
		}

		inline render_context_t() { reset(); }

		bool can_update(i_renderable* r) const;
		void set_hovered_element(i_renderable* r);
		bool out_of_bounds(vec2d rect1_pos, vec2d rect1_size, vec2d rect2_pos, vec2d rect2_size) {
			float left1 = rect1_pos.x;
			float right1 = rect1_pos.x + rect1_size.x;
			float top1 = rect1_pos.y;
			float bottom1 = rect1_pos.y + rect1_size.y;
			float left2 = rect2_pos.x;
			float right2 = rect2_pos.x + rect2_size.x;
			float top2 = rect2_pos.y;
			float bottom2 = rect2_pos.y + rect2_size.y;

			if (left1 > right2 || left2 > right1 || top1 > bottom2 || top2 > bottom1)
				return true;

			return false;
		}

		bool out_of_bounds(i_renderable* r);
		vec2d apply(i_renderable* r, vec2d padding);
	};

	struct animation_t {
		float m_value{};

		void lerp(render_context_t& ctx, const float target, const float scale = 2.5f) {
			if (m_value > target)
				m_value = std::clamp(m_value - ctx.m_time_delta * scale, target, 1.f);
			else if (m_value < target)
				m_value = std::clamp(m_value + ctx.m_time_delta * scale, 0.f, target);
		}

		void lerp(render_context_t& ctx, bool trigger) { this->lerp(ctx, trigger ? 1.f : 0.f); }

		// prevent user will accidentally changed m_value
		// we won't define non const operator
		operator float() const { return m_value; }
	};

	struct pulsating_t {
		animation_t m_animation{};
		bool m_switch{};

		void pulsate(render_context_t& ctx, float scale = 1.f) {
			if (m_animation >= 1.f || m_animation <= 0.f)
				m_switch = !m_switch;

			m_animation.lerp(ctx, m_switch, scale);
		}

		void stop(render_context_t& ctx, float scale = 1.f) { m_animation.lerp(ctx, false); }
		void trigger(render_context_t& ctx, bool t, float scale = 1.f) { return t ? this->pulsate(ctx, scale) : this->stop(ctx, scale); }

		operator float() const { return m_animation; }
	};

	struct scroll_t {
		vec2d m_offset{};
		vec2d m_offset_animated{};
		float m_limit_animation{};
		float m_last_scroll_time{};

		inline void reset() {
			m_offset = {};
			m_offset_animated = {};
		}
	};

	inline ImVec2 to_imvec2(vec2d v) { return { v.x, v.y }; }
	inline vec2d from_imvec2(ImVec2 v) { return { v.x, v.y }; }
	inline ImVec4 to_imvec4(color_t clr) {
		constexpr auto mult = 1.f / 255.f;
		return { clr.r() * mult, clr.g() * mult, clr.b() * mult, clr.a() * mult };
	}

	inline HWND hwnd{};

	extern void on_wnd_proc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam);
	extern void update();
	extern void render();
	extern void post_render();
	extern void add_instance(instance_t* instance);
	extern void render_item(i_renderable* item, render_context_t& ctx);

	extern std::future<void*> create_gif_from_buffer(const std::vector<uint8_t>& buf);
	extern std::future<void*> create_texture_from_buffer(const std::vector<uint8_t>& buf);
	extern void clear_texture(void* texture);
	extern void clear_gif(void* gif);

	namespace containers {
		extern vec2d render_items(render_context_t& ctx, const std::vector<i_renderable*>& items, e_items_direction items_direction);
		extern void recursive_delete(const std::vector<i_renderable*>& items);
		extern void render_loading(vec2d pos, float value, float alpha, const std::string& text = "");

		template<typename type_t>
		extern i_renderable* recursive_find(const std::vector<type_t>& items, std::function<bool(i_renderable*)> cond);
	} // namespace containers

	namespace dpi {
		inline std::optional<float> _new_scale{};

		inline float _scale = 1.25f;
		extern void on_scale_change();
		static bool change_scale(float new_scale) {
			const auto ret = _scale != new_scale;
			if (ret)
				_new_scale = new_scale;
			return ret;
		}

		template<typename type_t>
		static constexpr type_t scale(type_t n) {
			return n * _scale;
		}

		static float _get_actual_scale() {
			return _new_scale.has_value() ? *_new_scale : _scale;
		}
	} // namespace dpi
} // namespace NS_GUI