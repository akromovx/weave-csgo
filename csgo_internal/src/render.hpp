#pragma once
#include "../deps/imgui/imgui.h"
#include "base_includes.hpp"
#include "utils/color.hpp"
#include "utils/matrix.hpp"
#include <mutex>

#include <d3dx9.h>

using font_t = ImFont*;

namespace fonts {
	inline font_t esp_small;
	inline font_t esp_default;
	inline font_t menu_desc;
	inline font_t weapon_icons;
	inline font_t weapon_icons_big;

	inline font_t menu_main;
	inline font_t menu_main_weapons{};
	inline font_t menu_bold;
	inline font_t menu_big;
	inline font_t menu_small;
	inline font_t menu_small_bold;
	inline font_t menu_small_semibold;
} // namespace fonts

namespace render {
	inline bool can_render{};
	inline std::atomic_bool drawlists_done{ false };

	inline float current_load_animated{};
	inline std::atomic_int current_load_stage{};
	inline std::atomic_int total_load_stages{};
	inline std::string current_load_stage_name{};

	extern void global_update(std::function<void()> callback);

	STFI ImVec2 to_imvec2(const vec2d& v) { return { v.x, v.y }; }
	STFI vec2d from_imvec2(const ImVec2& v) { return { v.x, v.y }; }
	inline ImVec4 to_imvec4(color_t clr) {
		constexpr auto mult = 1.f / 255.f;
		return { clr.r() * mult, clr.g() * mult, clr.b() * mult, clr.a() * mult };
	}

	inline ImDrawList* draw_list = nullptr;
	inline ImDrawList* draw_list_fsn = nullptr;
	inline ImDrawList* draw_list_act = nullptr;
	inline ImDrawList* draw_list_rendering = nullptr;
	inline std::mutex mutex{};
	inline IDirect3DDevice9* device;

	extern void init(IDirect3DDevice9* device);
	extern void init_fonts();

	enum e_text_flags {
		none = 0,
		outline = 1 << 0,
		centered_x = 1 << 1,
		centered_y = 1 << 2,
		centered = centered_x | centered_y,
		align_left = 1 << 3,
		align_bottom = 1 << 4
	};

	extern void filled_rect(float x, float y, float w, float h, color_t color, float rounding = 0.f, ImDrawFlags flags = 15);
	extern void filled_rect(vec2d position, vec2d size, color_t clr, float rounding = 0.f, ImDrawCornerFlags rounding_corners = 15);

	extern void line(float x1, float y1, float x2, float y2, color_t clr, float thickness = 1.f);
	extern void rect(float x, float y, float w, float h, color_t clr, float rounding = 0.f, ImDrawFlags flags = 0, float thickness = 1.0f);
	extern void rect(vec2d position, vec2d size, color_t clr, float rounding = 0.f, ImDrawCornerFlags rounding_corners = 15);
	extern void image(vec2d position, vec2d size, void* texture, color_t override_color = {}, float rounding = 0.f, ImDrawCornerFlags rounding_corners = 15);
	extern void image_rotated(void* texture, vec2d center, vec2d size, float angle, color_t override_color = {});
	extern void filled_rect_gradient(float x, float y, float w, float h, color_t col_upr_left, color_t col_upr_right, color_t col_bot_right, color_t col_bot_left);
	extern void triangle(float x1, float y1, float x2, float y2, float x3, float y3, color_t clr, float thickness = 1.f);
	extern void triangle_filled(float x1, float y1, float x2, float y2, float x3, float y3, color_t clr);
	extern void triangle_filled_multicolor(float x1, float y1, float x2, float y2, float x3, float y3, color_t clr, color_t clr2, color_t clr3);
	extern void circle(float x1, float y1, float radius, color_t col, int segments, float thickness = 1.f);
	extern void circle_filled(float x1, float y1, float radius, color_t col, int segments);
	extern void text(float x, float y, color_t color, int flags, void* font, const std::string& message, float wrap_width = 0.f);
	extern void textf(float x, float y, color_t color, int flags, void* font, const char* message, ...);

	extern void cube(const vec3d& position, float size, color_t color, float thickness = 1.f);
	extern void filled_cube(const vec3d& position, float size, color_t color);

	class gif_t final {
		int m_gif_frame{};
		clock_t m_gif_last_time{};
		vec2d m_size{};

		struct gif_result_t {
			int delay;
			unsigned char* data;
			struct gif_result_t* next;
		};

	public:
		std::vector<std::pair<LPDIRECT3DTEXTURE9, int>> m_textures{};
		bool m_gif{};

		void render(ImDrawList* draw_list, vec2d position, vec2d size, color_t override_color, float rounding = 0.f, ImDrawCornerFlags rounding_corners = 15);
		gif_t(const utils::bytes_t& buf);
		~gif_t() {
			for (auto& [texture, delay]: m_textures)
				texture->Release();
		}

		vec2d get_original_size() const { return m_size; }
	};

	extern void gif(gif_t* gif_or_image, vec2d position, vec2d size, color_t override_color, float rounding = 0.f, ImDrawCornerFlags rounding_corners = 15);

	extern vec2d calc_text_size(std::string text, font_t font);
	extern matrix4x4_t& get_view_matrix();

	extern vec2d calc_text_size(const std::string& text, void* font, float wrap_width = 0.f);

	extern void lock(vec2d position, vec2d size);
	extern void unlock();

	extern vec2d get_locked_size();
	extern vec2d get_locked_pos();

	extern void ignore_lock(std::function<void()> render_fn);

	extern void begin();
	extern void end();
	extern void run();

	inline std::atomic_bool in_fsn{ false };
	inline float screen_width{};
	inline float screen_height{};
} // namespace render

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);