#pragma once
struct ImDrawList;
struct IDirect3DDevice9;

namespace shaders {
	void set_device(IDirect3DDevice9* device) noexcept;
	void clear_blur_textures() noexcept;
	void on_device_reset() noexcept;
	void new_frame() noexcept;
	void create_blur(ImDrawList* drawList, ImVec2 min, ImVec2 max, ImColor col = ImColor(255, 255, 255, 255), float rounding = 0.f, ImDrawCornerFlags round_flags = 15) noexcept;
	void glow_line(ImDrawList* drawList, ImVec2 min, ImVec2 max, ImVec4 line, ImColor col);
}