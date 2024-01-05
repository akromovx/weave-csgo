#include <d3d9.h>
#include <wrl/client.h>

using Microsoft::WRL::ComPtr;

#include "../../deps/imgui/imgui.h"

#ifdef _WIN32
// shaders are build during compilation and header files are created
#ifdef _DEBUG
#include "Shaders/Build/Debug/blur_x.h"
#include "Shaders/Build/Debug/blur_y.h"
#include "Shaders/Build/Debug/glow_line.h"
#else
#include "Shaders/Build/Release/blur_x.h"
#include "Shaders/Build/Release/blur_y.h"
#include "Shaders/Build/Debug/glow_line.h"
#endif

#endif

#include "blur.hpp"
#include "../utils/color.hpp"

static int backbuffer_width = 0;
static int backbuffer_height = 0;

using Microsoft::WRL::ComPtr;
static IDirect3DDevice9* device;

static IDirect3DTexture9* create_texture(int width, int height) noexcept {
	IDirect3DTexture9* texture;
	device->CreateTexture(width, height, 1, D3DUSAGE_RENDERTARGET, D3DFMT_X8R8G8B8, D3DPOOL_DEFAULT, &texture, nullptr);
	return texture;
}

static void copy_backbuffer_to_texture(IDirect3DTexture9* texture, D3DTEXTUREFILTERTYPE filtering) noexcept {
	ComPtr<IDirect3DSurface9> backBuffer;
	if (device->GetBackBuffer(0, 0, D3DBACKBUFFER_TYPE_MONO, backBuffer.GetAddressOf()) == D3D_OK) {
		ComPtr<IDirect3DSurface9> surface;
		if (texture->GetSurfaceLevel(0, surface.GetAddressOf()) == D3D_OK)
			device->StretchRect(backBuffer.Get(), nullptr, surface.Get(), nullptr, filtering);
	}
}

static void set_render_target(IDirect3DTexture9* rtTexture) noexcept {
	ComPtr<IDirect3DSurface9> surface;
	if (rtTexture->GetSurfaceLevel(0, surface.GetAddressOf()) == D3D_OK)
		device->SetRenderTarget(0, surface.Get());
}

struct shader_program {
	void use(float uniform, int location) const noexcept {
		device->SetPixelShader(pixel_shader.Get());
		const float params[4] = { uniform, 0.f, 0.f, 0.f };
		device->SetPixelShaderConstantF(location, params, 1);
	}

	void init(const BYTE* pixelShaderSrc) noexcept {
		if (initialized)
			return;

		initialized = true;
		device->CreatePixelShader((const DWORD*)pixelShaderSrc, pixel_shader.GetAddressOf());
	}

	ComPtr<IDirect3DPixelShader9> pixel_shader;
	bool initialized = false;
};

struct blur_effect {
	static void draw(ImDrawList* drawList, ImVec2 min, ImVec2 max, ImColor col, float rounding, ImDrawCornerFlags round_flags) noexcept {
		instance()._draw(drawList, min, max, col, rounding, round_flags);
	}

	static void clear_textures() noexcept {
		if (instance().blurTexture1 != nullptr) {
			instance().blurTexture1->Release();
			instance().blurTexture1 = nullptr;
		}

		if (instance().blurTexture2 != nullptr) {
			instance().blurTexture2->Release();
			instance().blurTexture2 = nullptr;
		}
	}

private:
	D3DMATRIX proj_backup{};

	IDirect3DSurface9* rtBackup = nullptr;
	IDirect3DTexture9* blurTexture1 = nullptr;
	IDirect3DTexture9* blurTexture2 = nullptr;

	shader_program blurShaderX;
	shader_program blurShaderY;
	static constexpr auto blurDownsample = 4;

	blur_effect() = default;
	blur_effect(const blur_effect&) = delete;

	~blur_effect()
	{
		if (rtBackup)
			rtBackup->Release();
		if (blurTexture1)
			blurTexture1->Release();
		if (blurTexture2)
			blurTexture2->Release();
	}

	static blur_effect& instance() noexcept
	{
		static blur_effect blurEffect;
		return blurEffect;
	}

	static void begin(const ImDrawList*, const ImDrawCmd*) noexcept { instance()._begin(); }
	static void firstPass(const ImDrawList*, const ImDrawCmd*) noexcept { instance()._firstPass(); }
	static void secondPass(const ImDrawList*, const ImDrawCmd*) noexcept { instance()._secondPass(); }
	static void end(const ImDrawList*, const ImDrawCmd*) noexcept { instance()._end(); }

	void createTextures() noexcept {
		if (!blurTexture1)
			blurTexture1 = create_texture(backbuffer_width / blurDownsample, backbuffer_height / blurDownsample);
		if (!blurTexture2)
			blurTexture2 = create_texture(backbuffer_width / blurDownsample, backbuffer_height / blurDownsample);
	}

	void createShaders() noexcept {
		blurShaderX.init(blur_x);
		blurShaderY.init(blur_y);
	}

	void _begin() noexcept {
		device->GetRenderTarget(0, &rtBackup);
		copy_backbuffer_to_texture(blurTexture1, D3DTEXF_LINEAR);
		device->SetSamplerState(0, D3DSAMP_ADDRESSU, D3DTADDRESS_CLAMP);
		device->SetSamplerState(0, D3DSAMP_ADDRESSV, D3DTADDRESS_CLAMP);
		device->SetRenderState(D3DRS_SCISSORTESTENABLE, false);

		device->GetVertexShaderConstantF(0, &proj_backup.m[0][0], 4);

		const D3DMATRIX projection{ {{
				   1.0f, 0.0f, 0.0f, 0.0f,
				   0.0f, 1.0f, 0.0f, 0.0f,
				   0.0f, 0.0f, 1.0f, 0.0f,
				   -1.0f / (backbuffer_width / blurDownsample), 1.0f / (backbuffer_height / blurDownsample), 0.0f, 1.0f
			   }} };
		device->SetVertexShaderConstantF(0, &projection.m[0][0], 4);
	}

	void _firstPass() noexcept {
		blurShaderX.use(1.0f / (backbuffer_width / blurDownsample), 0);
		set_render_target(blurTexture2);
	}

	void _secondPass() noexcept {
		blurShaderY.use(1.0f / (backbuffer_height / blurDownsample), 0);
		set_render_target(blurTexture1);
	}

	void _end() noexcept {
		device->SetRenderTarget(0, rtBackup);
		rtBackup->Release();

		device->SetPixelShader(nullptr);
		device->SetRenderState(D3DRS_SCISSORTESTENABLE, true);
	}

	void _draw(ImDrawList* drawList, ImVec2 min, ImVec2 max, ImColor col, float rounding, ImDrawCornerFlags round_flags = 15) noexcept {
		createTextures();
		createShaders();

		if (!blurTexture1 || !blurTexture2)
			return;

		drawList->AddCallback(&begin, nullptr);

		for (int i = 0; i < 8; ++i) {
			drawList->AddCallback(&firstPass, nullptr);
			drawList->AddImageRounded(blurTexture1, { -1.0f, -1.0f }, { 1.0f, 1.0f }, { 0.f, 0.f }, { 1.f, 1.f }, color_t{}.u32(), rounding, round_flags);
			drawList->AddCallback(&secondPass, nullptr);
			drawList->AddImageRounded(blurTexture2, { -1.0f, -1.0f }, { 1.0f, 1.0f }, { 0.f, 0.f }, { 1.f, 1.f }, color_t{}.u32(), rounding, round_flags);
		}

		drawList->AddCallback(&end, nullptr);
		drawList->AddCallback(ImDrawCallback_ResetRenderState, nullptr);
		drawList->AddImageRounded(blurTexture1, min, max, { min.x / backbuffer_width, min.y / backbuffer_height }, { max.x / backbuffer_width, max.y / backbuffer_height },
			ImGui::GetColorU32(col.Value), rounding, round_flags);
	}
};

void shaders::set_device(IDirect3DDevice9* device) noexcept {
	::device = device;
}

void shaders::clear_blur_textures() noexcept {
	blur_effect::clear_textures();
}

void shaders::on_device_reset() noexcept {
	blur_effect::clear_textures();
}

void shaders::new_frame() noexcept {
	const int width = (int)ImGui::GetIO().DisplaySize.x;
	const int height = (int)ImGui::GetIO().DisplaySize.y;
	if (backbuffer_width != width || backbuffer_height != height) {
		blur_effect::clear_textures();
		backbuffer_width = width;
		backbuffer_height = height;
	}
}

void shaders::create_blur(ImDrawList* drawList, ImVec2 min, ImVec2 max, ImColor col, float rounding, ImDrawCornerFlags round_flags) noexcept {
	blur_effect::draw(drawList, min, max, col, rounding, round_flags);
}

void shaders::glow_line(ImDrawList* drawList, ImVec2 min, ImVec2 max, ImVec4 line, ImColor col) {

}