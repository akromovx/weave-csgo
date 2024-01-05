#pragma once
#include "../src/utils/color.hpp"

namespace sdk {
	enum e_font_feature {
		font_feature_antialiased_fonts = 1,
		font_feature_dropshadow_fonts = 2,
		font_feature_outline_fonts = 6
	};

	enum e_font_draw_type {
		font_draw_default = 0,
		font_draw_nonadditive,
		font_draw_additive,
		font_draw_type_count = 2
	};

	enum e_font_flags {
		font_flag_none,
		font_flag_italic = 0x001,
		font_flag_underline = 0x002,
		font_flag_strikeout = 0x004,
		font_flag_symbol = 0x008,
		font_flag_antialias = 0x010,
		font_flag_gaussianblur = 0x020,
		font_flag_rotary = 0x040,
		font_flag_dropshadow = 0x080,
		font_flag_additive = 0x100,
		font_flag_outline = 0x200,
		font_flag_custom = 0x400,
		font_flag_bitmap = 0x800
	};

	typedef void* (*create_interface_fn)(const char* p_name, int* p_code);

	struct app_system_t {
		virtual bool connect(create_interface_fn factory) = 0;
		virtual void disconnect() = 0;
		virtual void* query_interface(const char* p_interface_name) = 0;
		virtual int init() = 0;
		virtual void shutdown() = 0;
		virtual const void* dependencies() = 0;
		virtual int tier() = 0;
		virtual void reconnect(create_interface_fn factory, const char* p_interface_name) = 0;
		virtual void unk_func() = 0;
	};

	struct int_rect {
		int x0;
		int y0;
		int x1;
		int y1;
	};

	struct surface_t : app_system_t {
		virtual void run_frame() = 0;
		virtual uint32_t embedded_panel() = 0;
		virtual void set_embedded_panel(uint32_t p_panel) = 0;
		virtual void push_make_current(uint32_t panel, bool use_insets) = 0;
		virtual void pop_make_current(uint32_t panel) = 0;
		virtual void draw_set_color(int r, int g, int b, int a) = 0;
		virtual void draw_set_color(color_t col) = 0;
		virtual void draw_filled_rect(int x0, int y0, int x1, int y1) = 0;
		virtual void draw_filled_rect_int_array(int_rect* p_rects, int num_rects) = 0;
		virtual void draw_outline(int x0, int y0, int x1, int y1) = 0;
		virtual void draw_line(int x0, int y0, int x1, int y1) = 0;
		virtual void draw_poly_line(int* px, int* py, int num_points) = 0;
		virtual void draw_set_depth(float f) = 0;
		virtual void draw_clear_depth(void) = 0;
		virtual void draw_set_text_font(uint32_t font) = 0;
		virtual void draw_set_text_color(int r, int g, int b, int a) = 0;
		virtual void draw_set_text_color(color_t col) = 0;
		virtual void draw_set_text_pos(int x, int y) = 0;
		virtual void draw_text_pos(int& x, int& y) = 0;
		virtual void draw_print_text(const wchar_t* text, int text_len, e_font_draw_type draw_type = e_font_draw_type::font_draw_default) = 0;
		virtual void draw_unicode_character(wchar_t wch, e_font_draw_type draw_type = e_font_draw_type::font_draw_default) = 0;
		virtual void draw_flush_text() = 0;
		virtual void* create_html_window(void* events, uint32_t context) = 0;
		virtual void paint_html_window(void* htmlwin) = 0;
		virtual void delete_html_window(void* htmlwin) = 0;
		virtual int draw_texture_id(char const* filename) = 0;
		virtual bool draw_texture_file(int id, char* filename, int maxlen) = 0;
		virtual void draw_set_texture_file(int id, const char* filename, int hardware_filter, bool reload) = 0;
		virtual void draw_set_texture_rgba(int id, const unsigned char* rgba, int wide, int tall) = 0;
		virtual void draw_set_texture(int id) = 0;
		virtual void delete_texture_id(int id) = 0;
		virtual void draw_texture_size(int id, int& wide, int& tall) = 0;
		virtual void draw_textured_rect(int x0, int y0, int x1, int y1) = 0;
		virtual bool is_texture_id_valid(int id) = 0;
		virtual int create_new_texture_id(bool procedural = false) = 0;
		virtual void screen_size(int& wide, int& tall) = 0;
		virtual void set_as_top_most(uint32_t panel, bool state) = 0;
		virtual void bring_to_front(uint32_t panel) = 0;
		virtual void set_foreground_window(uint32_t panel) = 0;
		virtual void set_panel_visibility(uint32_t panel, bool state) = 0;
		virtual void set_panel_minimizised(uint32_t panel, bool state) = 0;
		virtual bool is_panel_minimized(uint32_t panel) = 0;
		virtual void flash_window(uint32_t panel, bool state) = 0;
		virtual void set_title(uint32_t panel, const wchar_t* title) = 0;
		virtual void set_as_toolbar(uint32_t panel, bool state) = 0;
		virtual void create_popup(uint32_t panel, bool minimised, bool show_taskbar_icon = true, bool disabled = false, bool mouse_input = true, bool keyboard_input = true) = 0;
		virtual void swap_buffers(uint32_t panel) = 0;
		virtual void invalidate(uint32_t panel) = 0;
		virtual void set_cursor(unsigned long cursor) = 0;
		virtual bool is_cursor_visible() = 0;
		virtual void apply_changes() = 0;
		virtual bool is_within(int x, int y) = 0;
		virtual bool has_focus() = 0;
		virtual bool supports_feature(int feature) = 0;
		virtual void restrict_paint_to_panel(uint32_t panel, bool allow_nonmodal_surface = false) = 0;
		virtual void set_modal_panel(uint32_t) = 0;
		virtual uint32_t modal_panel() = 0;
		virtual void unlock_cursor() = 0;

		VFUNC(create_font(), uint32_t(__thiscall*)(decltype(this)), 71);
		VFUNC(set_font_glyph_set(uint32_t font, const char* font_name, int tall, int weight, int blur, int scanlines, int flags, int range_min, int range_max),
			  uint32_t(__thiscall*)(decltype(this), uint32_t, const char*, int, int, int, int, int, int, int), 72,
			  font, font_name, tall, weight, blur, scanlines, flags, range_min, range_max);

		VFUNC(get_text_size(uint32_t font, const wchar_t* text, int& wide, int& tall),
			  uint32_t(__thiscall*)(decltype(this), uint32_t, const wchar_t*, int&, int&), 79,
			  font, text, wide, tall);
	};
} // namespace sdk