#include "hooker.hpp"
#include "hooks.hpp"

#include "../../deps/imgui/imgui.h"
#include "../../deps/imgui/imgui_impl_dx9.h"
#include "../../deps/imgui/imgui_impl_win32.h"

//#include "../features/exmenu.hpp"
#include "../features/bullets.hpp"
#include "../features/menu.hpp"

#include "../features/visuals/esp.hpp"
#include "../features/visuals/hitmarker.hpp"
#include "../features/visuals/indicators.hpp"
#include "../features/visuals/logs.hpp"

#include "../blur/blur.hpp"
#include "../utils/animation_handler.hpp"
#include "../../deps/imgui/imgui_freetype.h"

using namespace gui;
using namespace std::chrono_literals;

namespace hooks::directx {
	HRESULT __stdcall end_scene(IDirect3DDevice9* device) {
		static auto original = vmt::directx->original<decltype(&end_scene)>(XOR32S(42));

		if (!resources::downloaded)
			return original(device);

		static bool balls = false;
		if (!balls) {
			render::can_render = false;
			std::thread(render::init, device).detach();
			balls = true;
		}

		if (!render::can_render)
			return original(device);

		DWORD colorwrite, srgbwrite;
		IDirect3DVertexDeclaration9* vert_dec = nullptr;
		IDirect3DVertexShader9* vert_shader = nullptr;

		device->GetRenderState(D3DRS_COLORWRITEENABLE, &colorwrite);
		device->GetRenderState(D3DRS_SRGBWRITEENABLE, &srgbwrite);
		device->SetRenderState(D3DRS_COLORWRITEENABLE, 0xffffffff);
		device->SetRenderState(D3DRS_SRGBWRITEENABLE, false);
		device->GetVertexDeclaration(&vert_dec);
		device->GetVertexShader(&vert_shader);
		device->SetRenderState(D3DRS_COLORWRITEENABLE, 0xffffffff);
		device->SetRenderState(D3DRS_SRGBWRITEENABLE, false);
		device->SetSamplerState(NULL, D3DSAMP_ADDRESSU, D3DTADDRESS_WRAP);
		device->SetSamplerState(NULL, D3DSAMP_ADDRESSV, D3DTADDRESS_WRAP);
		device->SetSamplerState(NULL, D3DSAMP_ADDRESSW, D3DTADDRESS_WRAP);
		device->SetSamplerState(NULL, D3DSAMP_SRGBTEXTURE, NULL);

		ImGui_ImplDX9_NewFrame();
		ImGui_ImplWin32_NewFrame();

		ImGui::NewFrame();

		render::draw_list = ImGui::GetBackgroundDrawList();

		auto& io = ImGui::GetIO();
		render::screen_width = io.DisplaySize.x;
		render::screen_height = io.DisplaySize.y;

		animations_direct->update();

		shaders::set_device(device);
		shaders::new_frame();

		gui::resources::create_textures(device, []() { /*menu->window->update_state([]() { menu->create_gui(); });*/ });

		hitmarker->render();
		bullet_tracer->render();
		esp::render();
		bullet_impacts->render();

		indicators->render();
		cheat_logs->render();

		gui::update();
		menu::render();
		gui::render();

		gui::msgbox::render();

		ImGui::EndFrame();
		ImGui::Render();

		render::run();

		ImGui_ImplDX9_RenderDrawData(ImGui::GetDrawData());

		device->SetRenderState(D3DRS_COLORWRITEENABLE, colorwrite);
		device->SetRenderState(D3DRS_SRGBWRITEENABLE, srgbwrite);
		device->SetVertexDeclaration(vert_dec);
		device->SetVertexShader(vert_shader);

		const auto ret = original(device);

		//if (dpi::_new_scale.has_value()) [[unlikely]] {
		//	if (dpi::change_scale(*dpi::_new_scale)) {
		//		menu->window->set_loading(STRS("Updating user settings..."));
		//		dpi::_scale = *dpi::_new_scale;
		//		dpi::on_scale_change();
		//		menu->window->set_size(dpi::scale(menu->window_size));
		//		menu->window->update_state([]() { menu->create_gui(); });

		//		std::thread([]() {
		//			std::this_thread::sleep_for(200ms);
		//			menu->window->stop_loading();
		//		}).detach();
		//	}

		//	gui::globals().time_delta = 0.f;
		//	gui::globals().m_old_time = globals().get_time();
		//	dpi::_new_scale = std::nullopt;
		//}

		gui::post_render();

		return ret;
	}

	HRESULT __stdcall reset(IDirect3DDevice9* device, D3DPRESENT_PARAMETERS* params) {
		static auto original = vmt::directx->original<decltype(&reset)>(XOR32S(16));

		shaders::on_device_reset();
		ImGui_ImplDX9_InvalidateDeviceObjects();
		long result = original(device, params);
		ImGui_ImplDX9_CreateDeviceObjects();

		return result;
	}
} // namespace hooks::directx