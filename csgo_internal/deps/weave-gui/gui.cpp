#include "gui.hpp"
#include "../../src/features/menu.hpp"
#include "instance.hpp"
#include "instance/window.hpp"
#include "render_wrapper.hpp"
#include "renderable.hpp"
#include <format>

namespace NS_GUI {
	bool render_context_t::can_update(i_renderable* r) const {
		if (m_instance->is_locked())
			return false;

		if (!globals().is_mouse_in_region(render::get_locked_pos(), render::get_locked_size()))
			return false;

		if (m_active_item != nullptr)
			return r == m_active_item;

		if (m_in_popup)
			return m_can_update && (m_hovered_item == nullptr || m_hovered_item == r);

		return m_can_update && (m_hovered_item == nullptr || m_hovered_item == r) && !m_instance->has_popups();
	}

	bool render_context_t::out_of_bounds(i_renderable* r) {
		//if (r->get_item_type() == e_item_type::navigation_node)
		//	return false;

		//if (r->get_position_type() == e_position_type::absolute)
		//	return false;

		//const auto size = r->get_size();
		//if (size.is_zero())
		//	return false;

		//return out_of_bounds(r->calculate_position(*this), r->get_size(), m_instance->get_position(), m_size);

		return false;
	}

	void render_context_t::set_hovered_element(i_renderable* r) {
		if (!m_can_update || m_hovered_item != nullptr && m_hovered_item->get_item_type() != e_item_type::popup)
			return;

		m_hovered_item = r;
	}

	vec2d render_context_t::apply(i_renderable* r, vec2d padding) {
		if (r->get_item_type() == e_item_type::navigation_node)
			return { 0.f, 0.f };

		if (r->get_position_type() == e_position_type::absolute)
			return { 0.f, 0.f };

		const auto size = r->get_size();
		if (size.is_zero())
			return { 0.f, 0.f };

		switch ((int)m_direction) {
			case (int)e_items_direction::horizontal:
				m_cursor.x += size.x + padding.x;
				return { size.x + padding.x, size.y };
			case (int)e_items_direction::vertical:
				m_cursor.y += size.y + padding.y;
				return { size.x, size.y + padding.y };
		}

		return INVALID_POS;
	}

	void update() {
		auto& io = ImGui::GetIO();
		globals().mouse_position = { io.MousePos.x, io.MousePos.y };
		globals().mouse_delta = { io.MouseDelta.x, io.MouseDelta.y };
		globals().mouse_wheel = io.MouseWheel;

		for (int i = 0; i < 5; ++i)
			globals().mouse_down[i] = io.MouseDown[i];

		for (int i = 0; i < 5; ++i)
			globals().mouse_click[i] = io.MouseClicked[i];

		const float time = globals().get_time();
		globals().time_delta = std::abs(time - globals().m_old_time) * globals().m_time_scale;
		globals().m_old_time = time;

		// calculate FPS

		//++render::framecount;
		//if (std::abs(time - render::last_frametime_update) >= 1.f / render::framerate_updates_per_second) {
		//	render::last_frametime_update = time;
		//	render::framerate = render::framecount * render::framerate_updates_per_second;
		//	render::framecount = 0;
		//}
	}

	static std::vector<instance::window_t*> instances{};

	void add_instance(instance_t* instance) {
		instances.emplace_back((instance::window_t*)instance);
	}

	static std::mutex gifs_mtx{};
	static std::vector<std::pair<std::vector<uint8_t>, std::promise<void*>>> gifs_to_create{};

	static std::vector<void*> textures_to_clear{};
	static std::vector<void*> gif_to_clear{};

	std::future<void*> create_gif_from_buffer(const std::vector<uint8_t>& buf) {
		THREAD_SAFE(gifs_mtx);
		auto& [buffer, promise] = gifs_to_create.emplace_back();
		buffer = buf;
		return promise.get_future();
	}

	std::future<void*> create_texture_from_buffer(const std::vector<uint8_t>& buf) {
		auto& [buffer, promise] = gifs_to_create.emplace_back();
		buffer = buf;
		return promise.get_future();
	}

	void render() {
		{
			THREAD_SAFE(gifs_mtx);
			for (auto& [buf, promise]: gifs_to_create)
				promise.set_value(new render::gif_t{ buf });

			gifs_to_create.clear();
		}

		if (instances.empty()) return;

		size_t wnds_size = instances.size();
		if (wnds_size == 1) {
			instances[0]->update();
			instances[0]->m_can_update = true;
			return instances[0]->render();
		}

		bool check = false;

		for (int i = wnds_size - 1; i >= 0; --i) {
			auto wnd = instances[i];
			wnd->m_can_update = true;
			if (check) {
				wnd->m_can_update = false;
				continue;
			}

			if (wnd->update()) {
				instances[i] = instances[wnds_size - 1];
				instances[wnds_size - 1] = wnd;
				check = true;
			}
		}

		instances.back()->m_can_update = true;
		for (int i = 0; i < wnds_size - 1; ++i)
			instances[i]->m_can_update = false;

		bool can_update = true;
		for (auto& wnd: instances)
			wnd->render();
	}

	void clear_texture(void* texture) {
		if (texture == nullptr)
			return;

		textures_to_clear.push_back(texture);
	}

	void clear_gif(void* gif) {
		if (gif == nullptr)
			return;

		gif_to_clear.push_back(gif);
	}

	void post_render() {
		for (auto texture: textures_to_clear)
			((LPDIRECT3DTEXTURE9)texture)->Release();

		for (auto g: gif_to_clear)
			delete (render::gif_t*)g;

		textures_to_clear.clear();
		gif_to_clear.clear();
	}

	void on_wnd_proc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) {

		switch (wparam) {
			case VK_RIGHT:
			case VK_LEFT: {
				if (GetKeyState(VK_SHIFT) & 0x8000) {
					if (wparam == VK_RIGHT)
						++globals().arrow;
					else if (wparam == VK_LEFT)
						--globals().arrow;
					break;
				}
			};
		}
	}

	void render_item(i_renderable* item, render_context_t& ctx) {
		if (ctx.out_of_bounds(item))
			return;

		auto& s = item->get_style();
		if (s.has_value()) {
			styles::call_and_restore(
					[&]() {
						s->callback();
						item->render(ctx);
					},
					*s);

			if (!s->tooltip.empty() && item->is_hovered()) {
				const auto& tt = s->tooltip;
				const auto pos = globals().mouse_position + dpi::scale(vec2d{ 16.f, 0.f });
				auto tooltip_size = render::calc_text_size(tt, fonts::menu_main, dpi::scale(200.f));

				SET_AND_RESTORE(render::draw_list, render::draw_list);
				render::draw_list = ImGui::GetForegroundDrawList();

				render::filled_rect(pos, vec2d{ std::min<float>(dpi::scale(216.f), tooltip_size.x + dpi::scale(16.f)), tooltip_size.y + dpi::scale(16.f) },
									color_t{ 35, 35, 35 }.modify_alpha(ctx.m_alpha), dpi::scale(6.f));

				render::text(pos.x + dpi::scale(8.f), pos.y + dpi::scale(8.f),
							 color_t{}.modify_alpha(ctx.m_alpha), render::none, fonts::menu_main, tt, dpi::scale(200.f));
			}

		} else
			item->render(ctx);
	}

	void dpi::on_scale_change() {
		//render::invalidate_objects();

		//THREAD_SAFE(render::can_render_mtx);
		//std::thread(
		//		[]() {
		//			//

		//			render::can_render = false;
		//			{
		//				std::unique_lock lock{ render::can_render_mtx };
		//				render::can_render_cond.wait(lock, []() { return !render::in_end_scene; });
		//			}

		//			render::init_fonts();
		//			render::can_render = true;
		//		})
		//		.detach();

		render::can_render = false;
		std::thread(
				[]() {
					render::init_fonts();
					render::can_render = true;
				})
				.detach();
	}

	static inline void render_gradient_text(std::string text, vec2d pos, bool centered, float alpha) {
		static float cycle{};
		cycle += globals().time_delta * 1.0f;
		float alpha_mod = cycle;

		ImGui::PushFont(fonts::menu_bold);
		auto text_size = ImGui::CalcTextSize(text.c_str());
		ImGui::PopFont();

		if (centered)
			pos.x -= text_size.x * 0.5f;

		vec2d current_pos = pos;

		for (auto c: text) {
			const auto char_str = std::format("{}", c);
			const auto c_alpha = std::clamp(1.f - std::sin(alpha_mod), 0.25f, 0.35f) * alpha;
			render::text(current_pos.x, current_pos.y, color_t{}.modify_alpha(c_alpha), render::none, fonts::menu_bold, char_str);

			const auto char_size = render::calc_text_size(char_str, fonts::menu_bold);

			if (c == '\n') {
				current_pos.x = pos.x;
				current_pos.y += char_size.y;
			} else
				current_pos.x += char_size.x;

			alpha_mod += 0.1f;
		}
	}

	inline void containers::render_loading(vec2d pos, float value, float alpha, const std::string& text) {
		auto alpha_mod = value;
		if (alpha_mod > 0.5f)
			alpha_mod = 1.f - alpha_mod;

		alpha_mod = std::clamp(alpha_mod, 0.25f, 1.f);

		float value1 = easings::in_out_expo(value) * f2PI;
		render::draw_list->PathArcTo(to_imvec2(pos), dpi::scale(32.f - 10 - 6 - 2), value1 - fPI, value1, 25);
		render::draw_list->PathStroke(color_t{ 255, 255, 255 }.modify_alpha(alpha * alpha_mod).abgr(), false, dpi::scale(3.f));

		value1 = easings::in_out_quart(value) * f2PI;
		render::draw_list->PathArcTo(to_imvec2(pos), dpi::scale(32.f - 10), -value1 - fPI2, -value1 + fPI2, 25);
		render::draw_list->PathStroke(color_t{ 255, 255, 255 }.modify_alpha(alpha * alpha_mod).abgr(), false, dpi::scale(6.f));

		value1 = easings::in_out_quad(value) * f2PI;
		render::draw_list->PathArcTo(to_imvec2(pos), dpi::scale(32.f), value1 - fPI2, value1 + fPI2, 25);
		render::draw_list->PathStroke(color_t{ 255, 255, 255 }.modify_alpha(alpha * alpha_mod).abgr(), false, dpi::scale(8.f));

		value1 = /*easings::in_out_sine*/ (value)*f2PI;
		render::draw_list->PathArcTo(to_imvec2(pos), dpi::scale(32.f + 8), -value1, -value1 + fPI, 25);
		render::draw_list->PathStroke(color_t{ 255, 255, 255 }.modify_alpha(alpha * alpha_mod).abgr(), false, dpi::scale(4.f));

		render_gradient_text(text, pos + vec2d{ 0.f, dpi::scale(40.f + 16.f) }, true, alpha);
	}
} // namespace NS_GUI