#include "render.hpp"
#include "hooks/hooks.hpp"
#include "utils/displacement.hpp"

#include "../deps/imgui/imgui_freetype.h"
#include "../deps/imgui/imgui_impl_dx9.h"
#include "../deps/imgui/imgui_impl_win32.h"
#include "../deps/weave-gui/include.hpp"

#include "../deps/fontawesome/include.hpp"

#include "../bytes/include.hpp"
#include "cheat.hpp"

#define STB_IMAGE_IMPLEMENTATION
#include "../deps/stb/stb_image.h"

using namespace gui;
extern ImTextureID ImGui_CreateTextureRGBA(int width, int height, const unsigned char* data);

namespace render {
	void render::global_update(std::function<void()> callback) {
		//
	}

	template<typename T>
	std::vector<uint8_t> copy_bytes(T* begin, T* end) {
		return { begin, end };
	}

	void merge_with_weapons(ImFontConfig& cfg, float size) {
		cfg.MergeMode = true;
		cfg.PixelSnapH = true;

		cfg.GlyphOffset.y = dpi::scale(2.f);

		static ImWchar icon_ranges[] = {
			0xE000, 0xF8FF, // private Use Area
			0
		};

		ImGui::GetIO().Fonts->AddFontFromMemoryCompressedTTF(
				weapon_icons_compressed_data,
				weapon_icons_compressed_size,
				size,
				&cfg,
				icon_ranges);

		cfg.PixelSnapH = false;
		cfg.GlyphOffset.y = 0.f;
		cfg.MergeMode = false;
	}

	void merge_with_fontawesome(ImFontConfig& cfg, float size, float original_size, bool bold = false) {
		cfg.MergeMode = true;
		cfg.GlyphMinAdvanceX = original_size; // Use if you want to make the icon monospaced
		cfg.PixelSnapH = true;

		static const ImWchar ranges[] = {
			0xe005,
			0xf8ff,
			0,
		};

		//ImGui::GetIO().Fonts->AddFontFromFileTTF(bold ? STRSC("C:\\Weave\\fonts\\fa-solid-900.ttf") : STRSC("C:\\Weave\\fonts\\fa-regular-400.ttf"), size, &cfg, ranges);
		ImGui::GetIO().Fonts->AddFontFromFileTTF(STRSC("C:\\Weave\\fonts\\fa-solid-900.ttf"), size, &cfg, ranges);
		ImGui::GetIO().Fonts->AddFontFromFileTTF(STRSC("C:\\Weave\\fonts\\fa-brands-400.ttf"), size, &cfg, ranges);

		cfg.PixelSnapH = false;
		cfg.GlyphMinAdvanceX = 0.f;
		cfg.MergeMode = false;
	}

	void merge_with_chinese(ImFontConfig& cfg, float size, bool bold = false) {
		cfg.MergeMode = true;

		static const ImWchar ranges[] = {
			0x4E00,
			0x9FAF, // CJK Unified Ideographs
			0,
		};

		ImGui::GetIO().Fonts->AddFontFromFileTTF(
				bold ? STRSC("C:\\Weave\\fonts\\Source-Han-Sans-CN-Bold.ttf") : STRSC("C:\\Weave\\fonts\\Source-Han-Sans-CN-Regular.ttf"),
				size, &cfg,
				ranges);

		cfg.MergeMode = false;
	}

	void init_fonts() {
		render::current_load_stage = 0;
		render::total_load_stages = 0;
		render::current_load_animated = 0.f;
		//ImGui::CreateContext();

		ImGuiIO& io = ImGui::GetIO();

		io.Fonts->Clear();

		ImFontConfig cfg;
		cfg.OversampleH = 3;
		cfg.OversampleV = 1;

		static const ImWchar ranges[] = {
			0x0020,
			0x00FF, // Basic Latin + Latin Supplement
			0x0400,
			0x052F, // Cyrillic + Cyrillic Supplement
			0x2DE0,
			0x2DFF, // Cyrillic Extended-A
			0xA640,
			0xA69F, // Cyrillic Extended-B
			0x20AC,
			0x20AC,
			0x2122,
			0x2122,
			0x2196,
			0x2196,
			0x21D6,
			0x21D6,
			0x221E,
			0x221E,
			0x2B01,
			0x2B01,
			0x2B09,
			0x2B09,
			0x2921,
			0x2922,
			0x263A,
			0x263A,
			0x266A,
			0x266A,
			0,
		};

		constexpr ImWchar icon_ranges[] = {
			0xE000, 0xF8FF, // private Use Area
			0
		};

		constexpr auto icon_size_mod = 3.f / 4.f;

		io.Fonts->TexDesiredWidth = 16384; // Increase as needed
		io.FontDefault = fonts::menu_main = io.Fonts->AddFontFromFileTTF(STRSC("C:\\Weave\\fonts\\RobotoFlex-Regular.ttf"), dpi::scale(14.5f), &cfg, ranges);
		merge_with_chinese(cfg, dpi::scale(14.5f), false);
		merge_with_fontawesome(cfg, dpi::scale(14.5f * icon_size_mod), dpi::scale(14.5f));

		fonts::menu_main_weapons = io.Fonts->AddFontFromFileTTF(STRSC("C:\\Weave\\fonts\\RobotoFlex-Regular.ttf"), dpi::scale(14.5f), &cfg, ranges);
		merge_with_weapons(cfg, dpi::scale(14.5f));

		fonts::menu_bold = io.Fonts->AddFontFromFileTTF(STRSC("C:\\Weave\\fonts\\RobotoFlex-Bold.ttf"), dpi::scale(14.5f), &cfg, ranges);
		merge_with_chinese(cfg, dpi::scale(14.5f), true);
		merge_with_fontawesome(cfg, dpi::scale(14.5f * icon_size_mod), dpi::scale(14.5f), true);

		fonts::menu_small = io.Fonts->AddFontFromFileTTF(STRSC("C:\\Weave\\fonts\\RobotoFlex-Regular.ttf"), dpi::scale(14.f), &cfg, ranges);
		merge_with_chinese(cfg, dpi::scale(14.f), false);

		fonts::menu_small_bold = io.Fonts->AddFontFromFileTTF(STRSC("C:\\Weave\\fonts\\RobotoFlex-Bold.ttf"), dpi::scale(14.f), &cfg, ranges);
		merge_with_chinese(cfg, dpi::scale(14.f), true);

		cfg.GlyphExtraSpacing = { 0.25f, 0.f };
		fonts::menu_big = io.Fonts->AddFontFromFileTTF(STRSC("C:\\Weave\\fonts\\RobotoFlex-Bold.ttf"), dpi::scale(18.f), &cfg, ranges);
		merge_with_chinese(cfg, dpi::scale(18.f), true);
		cfg.GlyphExtraSpacing = { 0.f, 0.f };

		fonts::menu_small_semibold = io.Fonts->AddFontFromFileTTF(STRSC("C:\\Weave\\fonts\\RobotoFlex-SemiBold.ttf"), dpi::scale(14.f), &cfg, ranges);
		//merge_with_chinese(cfg, dpi::scale(14.f), false);

		fonts::esp_default = io.Fonts->AddFontFromFileTTF(STRSC("C:\\Windows\\Fonts\\Verdana.ttf"), dpi::scale(12.f), &cfg, io.Fonts->GetGlyphRangesCyrillic());
		//merge_with_chinese(cfg, dpi::scale(12.f), false);

		{

			fonts::weapon_icons = io.Fonts->AddFontFromMemoryCompressedTTF(
					weapon_icons_compressed_data,
					weapon_icons_compressed_size,
					dpi::scale(14.f),
					&cfg,
					icon_ranges);

			fonts::weapon_icons_big = io.Fonts->AddFontFromMemoryCompressedTTF(
					weapon_icons_compressed_data,
					weapon_icons_compressed_size,
					dpi::scale(26.f),
					&cfg,
					icon_ranges);
		}

		if (dpi::_get_actual_scale() > 1.f) {
			fonts::esp_small = io.Fonts->AddFontFromFileTTF(STRSC("C:\\Weave\\fonts\\RobotoFlex-Regular.ttf"), dpi::scale(11.f), &cfg, io.Fonts->GetGlyphRangesCyrillic());
		} else {
			SET_AND_RESTORE(cfg.FontBuilderFlags, ImGuiFreeTypeBuilderFlags_MonoHinting | ImGuiFreeTypeBuilderFlags_Monochrome);
			fonts::esp_small = io.Fonts->AddFontFromFileTTF(STRSC("C:\\Weave\\fonts\\Smallest-Pixel7.ttf"), dpi::scale(10.f), &cfg, io.Fonts->GetGlyphRangesCyrillic());
		}

		fonts::menu_desc = io.Fonts->AddFontFromFileTTF(STRSC("C:\\Weave\\fonts\\Root-UI-Bold.ttf"), dpi::scale(14.f), &cfg, io.Fonts->GetGlyphRangesCyrillic());

		ImGuiFreeType::BuildFontAtlas(io.Fonts);
		ImGui_ImplDX9_CreateDeviceObjects();

		PUSH_LOG(STRSC("fonts created\n"));
		render::current_load_stage = 0;
		render::total_load_stages = 0;
		render::current_load_animated = 0.f;
		render::can_render = true;
	}

	CHEAT_INIT void init(IDirect3DDevice9* device) {
		can_render = false;
		ImGui::CreateContext();
		auto& io = ImGui::GetIO();

		io.ConfigFlags |= ImGuiConfigFlags_NoMouseCursorChange;
		ImGui::GetStyle().WindowRounding = 0.0f;

		if (!ImGui_ImplDX9_Init(device))
			return;

		if (!ImGui_ImplWin32_Init(ctx->hwnd))
			return;

		ImGuiStyle& style = ImGui::GetStyle();
		style.AntiAliasedFill = false;
		style.AntiAliasedLines = false;

		render::device = device;
		init_fonts();

		hooks::old_wnd_proc = SetWindowLongA(ctx->hwnd, GWLP_WNDPROC, (LONG_PTR)(hooks::wnd_proc));

		//draw_list = new ImDrawList(ImGui::GetDrawListSharedData());
		draw_list_fsn = new ImDrawList(ImGui::GetDrawListSharedData());
		draw_list_act = new ImDrawList(ImGui::GetDrawListSharedData());
		draw_list_rendering = new ImDrawList(ImGui::GetDrawListSharedData());
		can_render = true;
		drawlists_done = true;
	}

	static bool is_jpeg(const utils::bytes_t& buf) {
		return buf[0] == 0xFF && buf[1] == 0xD8;
	}

	static bool is_jpg(const utils::bytes_t& buf) {
		return buf[0] == 0xFF && buf[1] == 0xD8 && buf[2] == 0xFF;
	}

	static bool is_png(const utils::bytes_t& buf) {
		return std::memcmp(buf.data(), "\x89PNG\r\n\x1a\n", 8) == 0;
	}

	static bool is_gif(const utils::bytes_t& buf) {
		if (buf.size() < 6) {
			return false;
		}

		if (buf[0] == 'G' && buf[1] == 'I' && buf[2] == 'F' &&
			buf[3] == '8' && (buf[4] == '7' || buf[4] == '9') && buf[5] == 'a') {
			return true;
		}

		return false;
	}

	static vec2d get_png_dimensions(const std::vector<uint8_t>& buf) {
		// Verify PNG header
		if (buf.size() < 8 || std::memcmp(buf.data(), "\x89PNG\r\n\x1a\n", 8) != 0) {
			return INVALID_POS;
		}

		// Skip over the PNG header
		const uint8_t* png_data = buf.data() + 8;
		size_t png_data_size = buf.size() - 8;

		// Loop over chunks until we find the IHDR chunk
		while (png_data_size >= 8) {
			// Read chunk length and type
			uint32_t length = (png_data[0] << 24) | (png_data[1] << 16) | (png_data[2] << 8) | png_data[3];
			const char* type = reinterpret_cast<const char*>(png_data + 4);

			// Check if this is the IHDR chunk
			if (std::memcmp(type, "IHDR", 4) == 0) {
				// Read IHDR data (width, height, bit depth, etc.)
				int width = (png_data[8] << 24) | (png_data[9] << 16) | (png_data[10] << 8) | png_data[11];
				int height = (png_data[12] << 24) | (png_data[13] << 16) | (png_data[14] << 8) | png_data[15];
				return { static_cast<float>(width), static_cast<float>(height) };
			}

			// Skip over this chunk
			png_data += length + 12; // 12 bytes for length, type, and CRC
			png_data_size -= length + 12;
		}

		// If we didn't find the IHDR chunk, throw an error
		return INVALID_POS;
	}

	gif_t::gif_t(const utils::bytes_t& buf) {
		if (buf.size() <= 4)
			return;

		if (is_png(buf) || is_jpeg(buf) || is_jpg(buf)) {
			m_gif = false;
			auto& frame = m_textures.emplace_back();
			frame.second = 0;
			D3DXCreateTextureFromFileInMemory(render::device, buf.data(), buf.size(), &frame.first);
			m_size = get_png_dimensions(buf);
			return;
		} else if (is_gif(buf)) {
			m_gif = true;

			// Load a GIF image from memory using stbi_load_gif_from_memory
			int width, height, frames, comp;
			int* delays = nullptr;
			auto frames_data = stbi_load_gif_from_memory(buf.data(), buf.size(), &delays, &width, &height, &frames, &comp, STBI_rgb_alpha);

			m_size = { (float)width, (float)height };

			stbi__context s;
			stbi__start_mem(&s, buf.data(), buf.size());

			if (stbi__gif_test(&s)) {
				int c{};
				stbi__gif* g = new stbi__gif{};
				if (g == nullptr)
					return;

				memset(g, 0, sizeof(*g));

				while (auto data = stbi__gif_load_next(&s, g, &c, 4, nullptr)) {
					if (data == (unsigned char*)&s || data == nullptr)
						break;

					auto& [texture, delay] = m_textures.emplace_back();
					texture = (LPDIRECT3DTEXTURE9)ImGui_CreateTextureRGBA(width, height, data);
					delay = g->delay;
				}

				//if (stbi_failure_reason() != nullptr) {
				//	printf("%s\n", stbi_failure_reason());
				//}

				stbi_image_free(g->out);
				stbi_image_free(g->history);
				stbi_image_free(g->background);
				stbi_image_free(delays);

				delete g;
			}

			stbi_image_free(frames_data);
		}
	}

	void gif_t::render(ImDrawList* draw_list, vec2d position, vec2d size, color_t override_color, float rounding, ImDrawCornerFlags rounding_corners) {
		if (m_textures.empty()) {
			auto pfont = fonts::menu_main;

			draw_list->AddRectFilled(to_imvec2(position), to_imvec2(position + size), color_t{ 42, 42, 42 }.modify_alpha(override_color.a() / 255.f).abgr(), rounding, rounding_corners);
			draw_list->PushTextureID(pfont->ContainerAtlas->TexID);

			draw_list->AddText(fonts::menu_main, pfont->FontSize,
							   { position.x + size.x * 0.5f, position.y + size.y * 0.5f }, color_t{}.modify_alpha(override_color.a() / 255.f).abgr(), "Error");

			draw_list->PopTextureID();
			return;
		}

		if (m_gif) {
			auto& frame = m_textures[m_gif_frame % m_textures.size()];

			if (std::abs(m_gif_last_time - clock()) >= frame.second) {
				m_gif_frame++;
				m_gif_last_time = clock();
			}

			draw_list->AddImageRounded(
					frame.first, to_imvec2(position), to_imvec2(position + size), { 0.f, 0.f }, { 1.f, 1.f }, override_color.abgr(), rounding, rounding_corners);
		} else
			draw_list->AddImageRounded(m_textures[0].first, to_imvec2(position), to_imvec2(position + size), { 0.f, 0.f }, { 1.f, 1.f }, override_color.abgr(), rounding, rounding_corners);
	}

	void gif(gif_t* gif, vec2d position, vec2d size, color_t override_color, float rounding, ImDrawCornerFlags rounding_corners) {
		gif->render(draw_list, position, size, override_color, rounding, rounding_corners);
	}

	__forceinline void filled_rect(float x, float y, float w, float h, color_t color, float rounding, ImDrawFlags flags) {
		draw_list->AddRectFilled({ x, y }, { x + w, y + h }, color.abgr(), rounding, flags);
	}

	__forceinline void filled_rect(vec2d position, vec2d size, color_t clr, float rounding, ImDrawCornerFlags rounding_corners) {
		draw_list->AddRectFilled({ position.x, position.y }, { position.x + size.x, position.y + size.y }, clr.abgr(), rounding, rounding_corners);
	}

	__forceinline void line(float x1, float y1, float x2, float y2, color_t clr, float thickness) {
		draw_list->AddLine({ x1, y1 }, { x2, y2 }, clr.abgr(), thickness);
	}

	__forceinline void rect(float x, float y, float w, float h, color_t clr, float rounding, ImDrawFlags flags, float thickness) {
		draw_list->AddRect({ x, y }, { x + w, y + h }, clr.abgr(), rounding, flags, thickness);
	}

	__forceinline void rect(vec2d position, vec2d size, color_t clr, float rounding, ImDrawCornerFlags rounding_corners) {
		draw_list->AddRect({ position.x, position.y }, { position.x + size.x, position.y + size.y }, clr.abgr(), rounding, rounding_corners);
	}

	__forceinline void image(vec2d position, vec2d size, void* texture, color_t override_color, float rounding, ImDrawCornerFlags rounding_corners) {
		draw_list->AddImageRounded(texture, { position.x, position.y }, { position.x + size.x, position.y + size.y }, { 0.f, 0.f }, { 1.f, 1.f }, override_color.abgr(), rounding, rounding_corners);
	}

	void image_rotated(void* texture, vec2d center, vec2d size, float angle, color_t override_color) {
		const auto rotate = [](const vec2d& v, float cos_a, float sin_a) -> vec2d {
			return { v.x * cos_a - v.y * sin_a, v.x * sin_a + v.y * cos_a };
		};

		const auto cos_a = cosf(angle);
		const auto sin_a = sinf(angle);

		const vec2d pos[4] = {
			center + rotate({ -size.x * 0.5f, -size.y * 0.5f }, cos_a, sin_a),
			center + rotate({ +size.x * 0.5f, -size.y * 0.5f }, cos_a, sin_a),
			center + rotate({ +size.x * 0.5f, +size.y * 0.5f }, cos_a, sin_a),
			center + rotate({ -size.x * 0.5f, +size.y * 0.5f }, cos_a, sin_a)
		};

		const vec2d uvs[4] = { { 0.0f, 0.0f }, { 1.0f, 0.0f }, { 1.0f, 1.0f }, { 0.0f, 1.0f } };
		draw_list->AddImageQuad(texture,
								to_imvec2(pos[0]), to_imvec2(pos[1]), to_imvec2(pos[2]), to_imvec2(pos[3]),
								to_imvec2(uvs[0]), to_imvec2(uvs[1]), to_imvec2(uvs[2]), to_imvec2(uvs[3]), override_color.abgr());
	}

	__forceinline void filled_rect_gradient(float x, float y, float w, float h, color_t col_upr_left,
											color_t col_upr_right, color_t col_bot_right, color_t col_bot_left) {
		draw_list->AddRectFilledMultiColor({ x, y }, { x + w, y + h },
										   col_upr_left.abgr(), col_upr_right.abgr(), col_bot_right.abgr(), col_bot_left.abgr());
	}

	__forceinline void triangle(float x1, float y1, float x2, float y2, float x3, float y3, color_t clr, float thickness) {
		draw_list->AddTriangle({ x1, y1 }, { x2, y2 }, { x3, y3 }, clr.abgr(), thickness);
	}

	__forceinline void triangle_filled(float x1, float y1, float x2, float y2, float x3, float y3, color_t clr) {
		draw_list->AddTriangleFilled({ x1, y1 }, { x2, y2 }, { x3, y3 }, clr.abgr());
	}

	__forceinline void triangle_filled_multicolor(float x1, float y1, float x2, float y2, float x3, float y3, color_t clr, color_t clr2, color_t clr3) {
		draw_list->AddTriangleFilledMulticolor({ x1, y1 }, { x2, y2 }, { x3, y3 }, clr.abgr(), clr2.abgr(), clr3.abgr());
	}

	__forceinline void circle(float x1, float y1, float radius, color_t col, int segments, float thickness) {
		draw_list->AddCircle(ImVec2(x1, y1), radius, col.abgr(), segments, thickness);
	}

	__forceinline void circle_filled(float x1, float y1, float radius, color_t col, int segments) {
		draw_list->AddCircleFilled({ x1, y1 }, radius, col.abgr(), segments);
	}

	void textf(float x, float y, color_t color, int flags, void* font, const char* message, ...) {
		char output[4096] = {};
		va_list args;
		va_start(args, message);
		vsprintf_s(output, message, args);
		va_end(args);

		auto pfont = (font_t)font;

		draw_list->PushTextureID(pfont->ContainerAtlas->TexID);
		auto coord = ImVec2(x, y);
		auto size = calc_text_size(output, font);
		auto coord_out = ImVec2{ coord.x + 1.f, coord.y + 1.f };
		color_t outline_clr = color_t(0, 0, 0, color.a());
		ImVec2 pos = ImVec2{ std::round(coord.x), std::round(coord.y) };

		if (flags & align_left || flags & align_bottom) {
			if (flags & align_left)
				pos.x -= size.x;
			if (flags & align_bottom)
				pos.y -= size.y;
		} else {
			if (flags & centered_x)
				pos.x -= size.x * 0.5f;
			if (flags & centered_y)
				pos.y -= size.y * 0.5f;
		}

		if (flags & outline) {
			pos.y++;
			draw_list->AddText(pfont, pfont->FontSize, pos, outline_clr.abgr(), output);
			pos.x++;
			draw_list->AddText(pfont, pfont->FontSize, pos, outline_clr.abgr(), output);
			pos.y--;
			draw_list->AddText(pfont, pfont->FontSize, pos, outline_clr.abgr(), output);
			pos.x--;
			draw_list->AddText(pfont, pfont->FontSize, pos, outline_clr.abgr(), output);
		}

		draw_list->AddText(pfont, pfont->FontSize, pos, color.abgr(), output);
		draw_list->PopTextureID();
	}

	void text(float x, float y, color_t color, int flags, void* font, const std::string& message, float wrap_width) {
		const auto coord = ImVec2(x, y);
		const auto size = calc_text_size(message, font);
		const auto coord_out = ImVec2{ coord.x + 1.f, coord.y + 1.f };
		const auto outline_clr = color_t{ 0, 0, 0 }.modify_alpha((color.a() / 255.f) * (color.a() / 255.f));
		const auto pfont = (font_t)font;
		auto pos = ImVec2{ coord.x, coord.y };

		draw_list->PushTextureID(pfont->ContainerAtlas->TexID);

		if (flags & align_left || flags & align_bottom) {
			if (flags & align_left) {
				pos.x -= size.x;

				if (flags & centered_y)
					pos.y -= size.y * 0.5f;
			}

			if (flags & align_bottom) {
				pos.y -= size.y;

				if (flags & centered_x)
					pos.x -= size.x * 0.5f;
			}
		} else {
			if (flags & centered_x)
				pos.x -= size.x * 0.5f;
			if (flags & centered_y)
				pos.y -= size.y * 0.5f;
		}

		auto output = message.c_str();

		if (flags & outline) {
			pos.y++;
			draw_list->AddText(pfont, pfont->FontSize, pos, outline_clr.abgr(), output, nullptr, wrap_width);
			pos.x++;
			draw_list->AddText(pfont, pfont->FontSize, pos, outline_clr.abgr(), output, nullptr, wrap_width);
			pos.y--;
			draw_list->AddText(pfont, pfont->FontSize, pos, outline_clr.abgr(), output, nullptr, wrap_width);
			pos.x--;
			draw_list->AddText(pfont, pfont->FontSize, pos, outline_clr.abgr(), output, nullptr, wrap_width);
		}

		draw_list->AddText(pfont, pfont->FontSize, pos, color.abgr(), output, nullptr, wrap_width);
		draw_list->PopTextureID();
	}

	__forceinline vec2d calc_text_size(std::string text, font_t font) {
		if (font == nullptr)
			return {};

		auto size = font->CalcTextSizeA(font->FontSize, FLT_MAX, 0.0f, text.c_str());
		return { size.x, size.y };
	}

	__forceinline void cube(const vec3d& position, float size, color_t color, float thickness) {
		if (size <= 0.01f || color.a() == 0)
			return;

		vec3d forward{}, right{}, left{}, up{}, down{};

		math::angle_vectors({}, forward, up, right);

		down = up * vec3d(-1.f, -1.f, -1.f);
		left = right * vec3d(-1.f, -1.f, -1.f);

		auto new_up = up + left;
		auto new_left = down + left;
		auto new_right = up + right;
		auto new_down = down + right;

		up = new_up;
		left = new_left;
		right = new_right;
		down = new_down;

		enum {
			dir_left,
			dir_right,
			dir_up,
			dir_down
		};

		vec3d points_bottom[4] = {};
		points_bottom[dir_left] = position + (left * size) - (forward * size);
		points_bottom[dir_right] = position + (right * size) - (forward * size);
		points_bottom[dir_up] = position + (up * size) - (forward * size);
		points_bottom[dir_down] = position + (down * size) - (forward * size);

		size *= 2.f;

		vec3d points_top[4] = {};
		points_top[dir_left] = points_bottom[dir_left] + (forward * size);
		points_top[dir_right] = points_bottom[dir_right] + (forward * size);
		points_top[dir_up] = points_bottom[dir_up] + (forward * size);
		points_top[dir_down] = points_bottom[dir_down] + (forward * size);

		vec2d lines_bottom[4], lines_top[4];
		if (!math::world_to_screen(points_bottom[dir_left], lines_bottom[dir_left]))
			return;
		if (!math::world_to_screen(points_bottom[dir_up], lines_bottom[dir_right]))
			return;
		if (!math::world_to_screen(points_bottom[dir_down], lines_bottom[dir_down]))
			return;
		if (!math::world_to_screen(points_bottom[dir_right], lines_bottom[dir_up]))
			return;

		if (!math::world_to_screen(points_top[dir_left], lines_top[dir_left]))
			return;
		if (!math::world_to_screen(points_top[dir_up], lines_top[dir_right]))
			return;
		if (!math::world_to_screen(points_top[dir_down], lines_top[dir_down]))
			return;
		if (!math::world_to_screen(points_top[dir_right], lines_top[dir_up]))
			return;

		render::draw_list->PathLineTo(to_imvec2(lines_bottom[dir_left]));
		render::draw_list->PathLineTo(to_imvec2(lines_bottom[dir_down]));
		render::draw_list->PathLineTo(to_imvec2(lines_top[dir_down]));
		render::draw_list->PathLineTo(to_imvec2(lines_top[dir_left]));
		render::draw_list->PathLineTo(to_imvec2(lines_bottom[dir_left]));
		render::draw_list->PathStroke(color.abgr(), 0, thickness);

		render::draw_list->PathLineTo(to_imvec2(lines_bottom[dir_right]));
		render::draw_list->PathLineTo(to_imvec2(lines_bottom[dir_up]));
		render::draw_list->PathLineTo(to_imvec2(lines_top[dir_up]));
		render::draw_list->PathLineTo(to_imvec2(lines_top[dir_right]));
		render::draw_list->PathLineTo(to_imvec2(lines_bottom[dir_right]));
		render::draw_list->PathStroke(color.abgr(), 0, thickness);

		render::draw_list->PathLineTo(to_imvec2(lines_bottom[dir_up]));
		render::draw_list->PathLineTo(to_imvec2(lines_bottom[dir_down]));
		render::draw_list->PathLineTo(to_imvec2(lines_top[dir_down]));
		render::draw_list->PathLineTo(to_imvec2(lines_top[dir_up]));
		render::draw_list->PathLineTo(to_imvec2(lines_bottom[dir_up]));
		render::draw_list->PathStroke(color.abgr(), 0, thickness);

		render::draw_list->PathLineTo(to_imvec2(lines_bottom[dir_left]));
		render::draw_list->PathLineTo(to_imvec2(lines_bottom[dir_right]));
		render::draw_list->PathLineTo(to_imvec2(lines_top[dir_right]));
		render::draw_list->PathLineTo(to_imvec2(lines_top[dir_left]));
		render::draw_list->PathLineTo(to_imvec2(lines_bottom[dir_left]));
		render::draw_list->PathStroke(color.abgr(), 0, thickness);
	}

	__forceinline void filled_cube(const vec3d& position, float size, color_t color) {
		if (size <= 0.01f || color.a() == 0)
			return;

		vec3d forward{}, right{}, left{}, up{}, down{};

		math::angle_vectors({}, forward, up, right);

		down = up * vec3d(-1.f, -1.f, -1.f);
		left = right * vec3d(-1.f, -1.f, -1.f);

		auto new_up = up + left;
		auto new_left = down + left;
		auto new_right = up + right;
		auto new_down = down + right;

		up = new_up;
		left = new_left;
		right = new_right;
		down = new_down;

		enum {
			dir_left,
			dir_right,
			dir_up,
			dir_down
		};

		vec3d points_bottom[4] = {};
		points_bottom[dir_left] = position + (left * size) - (forward * size);
		points_bottom[dir_right] = position + (right * size) - (forward * size);
		points_bottom[dir_up] = position + (up * size) - (forward * size);
		points_bottom[dir_down] = position + (down * size) - (forward * size);

		size *= 2.f;

		vec3d points_top[4] = {};
		points_top[dir_left] = points_bottom[dir_left] + (forward * size);
		points_top[dir_right] = points_bottom[dir_right] + (forward * size);
		points_top[dir_up] = points_bottom[dir_up] + (forward * size);
		points_top[dir_down] = points_bottom[dir_down] + (forward * size);

		vec2d lines_bottom[4], lines_top[4];
		if (!math::world_to_screen(points_bottom[dir_left], lines_bottom[dir_left]))
			return;
		if (!math::world_to_screen(points_bottom[dir_up], lines_bottom[dir_right]))
			return;
		if (!math::world_to_screen(points_bottom[dir_down], lines_bottom[dir_down]))
			return;
		if (!math::world_to_screen(points_bottom[dir_right], lines_bottom[dir_up]))
			return;

		if (!math::world_to_screen(points_top[dir_left], lines_top[dir_left]))
			return;
		if (!math::world_to_screen(points_top[dir_up], lines_top[dir_right]))
			return;
		if (!math::world_to_screen(points_top[dir_down], lines_top[dir_down]))
			return;
		if (!math::world_to_screen(points_top[dir_right], lines_top[dir_up]))
			return;

		color.a() /= 2;

		render::draw_list->PathLineTo(to_imvec2(lines_bottom[dir_left]));
		render::draw_list->PathLineTo(to_imvec2(lines_bottom[dir_down]));
		render::draw_list->PathLineTo(to_imvec2(lines_top[dir_down]));
		render::draw_list->PathLineTo(to_imvec2(lines_top[dir_left]));
		render::draw_list->PathLineTo(to_imvec2(lines_bottom[dir_left]));
		render::draw_list->PathFillConvex(color.abgr());

		render::draw_list->PathLineTo(to_imvec2(lines_bottom[dir_right]));
		render::draw_list->PathLineTo(to_imvec2(lines_bottom[dir_up]));
		render::draw_list->PathLineTo(to_imvec2(lines_top[dir_up]));
		render::draw_list->PathLineTo(to_imvec2(lines_top[dir_right]));
		render::draw_list->PathLineTo(to_imvec2(lines_bottom[dir_right]));
		render::draw_list->PathFillConvex(color.abgr());

		render::draw_list->PathLineTo(to_imvec2(lines_bottom[dir_up]));
		render::draw_list->PathLineTo(to_imvec2(lines_bottom[dir_down]));
		render::draw_list->PathLineTo(to_imvec2(lines_top[dir_down]));
		render::draw_list->PathLineTo(to_imvec2(lines_top[dir_up]));
		render::draw_list->PathLineTo(to_imvec2(lines_bottom[dir_up]));
		render::draw_list->PathFillConvex(color.abgr());

		render::draw_list->PathLineTo(to_imvec2(lines_bottom[dir_left]));
		render::draw_list->PathLineTo(to_imvec2(lines_bottom[dir_right]));
		render::draw_list->PathLineTo(to_imvec2(lines_top[dir_right]));
		render::draw_list->PathLineTo(to_imvec2(lines_top[dir_left]));
		render::draw_list->PathLineTo(to_imvec2(lines_bottom[dir_left]));
		render::draw_list->PathFillConvex(color.abgr());

		render::draw_list->PathLineTo(to_imvec2(lines_bottom[dir_left]));
		render::draw_list->PathLineTo(to_imvec2(lines_bottom[dir_right]));
		render::draw_list->PathLineTo(to_imvec2(lines_bottom[dir_up]));
		render::draw_list->PathLineTo(to_imvec2(lines_bottom[dir_down]));
		render::draw_list->PathLineTo(to_imvec2(lines_bottom[dir_left]));
		render::draw_list->PathFillConvex(color.abgr());

		render::draw_list->PathLineTo(to_imvec2(lines_top[dir_left]));
		render::draw_list->PathLineTo(to_imvec2(lines_top[dir_right]));
		render::draw_list->PathLineTo(to_imvec2(lines_top[dir_up]));
		render::draw_list->PathLineTo(to_imvec2(lines_top[dir_down]));
		render::draw_list->PathLineTo(to_imvec2(lines_top[dir_left]));
		render::draw_list->PathFillConvex(color.abgr());
	}

	__forceinline matrix4x4_t& get_view_matrix() {
		static auto view_matrix = *patterns::screen_matrix.add(XOR32S(3)).as<uintptr_t*>();
		static auto ptr = view_matrix + XOR32S(176);
		return *(matrix4x4_t*)(ptr);
	}

	__forceinline void begin() {
		if (!render::drawlists_done.load())
			return;

		{
			THREAD_SAFE(mutex);
			if (draw_list_fsn == nullptr)
				return;

			draw_list_fsn->_ResetForNewFrame();
			draw_list_fsn->PushClipRectFullScreen();
		}

		in_fsn = true;
	}

	__forceinline void end() {
		if (!render::drawlists_done.load() || !in_fsn.load())
			return;

		{
			THREAD_SAFE(mutex);
			*draw_list_act = *draw_list_fsn;
		}

		in_fsn = false;
	}

	__forceinline void run() {
		if (mutex.try_lock()) {
			*draw_list_rendering = *draw_list_act;
			mutex.unlock();
		}

		ImDrawData drawData{};

		drawData.Valid = true;
		drawData.CmdLists = &draw_list_rendering;
		drawData.CmdListsCount = 1;
		drawData.TotalVtxCount = draw_list_rendering->VtxBuffer.size();
		drawData.TotalIdxCount = draw_list_rendering->IdxBuffer.size();

		drawData.DisplayPos = ImVec2(0.0f, 0.0f);
		drawData.DisplaySize = ImVec2(render::screen_width, render::screen_width);
		drawData.FramebufferScale = ImVec2(1.0f, 1.0f);

		ImGui_ImplDX9_RenderDrawData(&drawData);
	}

	__forceinline vec2d calc_text_size(const std::string& text, void* font, float wrap_width) {
		auto pfont = (ImFont*)font;
		auto size = pfont->CalcTextSizeA(pfont->FontSize, FLT_MAX, wrap_width, text.c_str());
		return { size.x, size.y };
	}

	__forceinline void lock(vec2d position, vec2d size) {
		draw_list->PushClipRect({ position.x, position.y }, { position.x + size.x, position.y + size.y }, true);
	}

	__forceinline vec2d get_locked_size() {
		return from_imvec2(draw_list->GetClipRectMax()) - from_imvec2(draw_list->GetClipRectMin());
	}

	__forceinline vec2d get_locked_pos() {
		return from_imvec2(draw_list->GetClipRectMin());
	}

	__forceinline void ignore_lock(std::function<void()> render_fn) {
		const ImVector<ImVec4> cliprects_before = draw_list->_ClipRectStack;
		auto ctn = draw_list->_ClipRectStack.size();

		for (int i = 0; i < ctn; ++i)
			draw_list->PopClipRect();

		draw_list->PushClipRectFullScreen();
		render_fn();
		draw_list->PopClipRect();

		for (int i = 0; i < ctn; ++i)
			draw_list->PushClipRect({ cliprects_before[i].x, cliprects_before[i].y }, { cliprects_before[i].z, cliprects_before[i].w });
	}

	__forceinline void unlock() {
		draw_list->PopClipRect();
	}
} // namespace render