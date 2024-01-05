#include "hooks.hpp"
#include "../render.hpp"
//#include "../features/exmenu.hpp"
#include "../features/menu.hpp"
#include "../utils/hotkeys.hpp"

namespace hooks {
	LRESULT WINAPI wnd_proc(HWND hwnd, UINT msg, WPARAM w_param, LPARAM l_param) {
		if (gui::msgbox::is_opened) {
			if (w_param == VK_ESCAPE)
				gui::msgbox::is_opened = false;

			return true;
		}

		if ((w_param == VK_INSERT || w_param == VK_DELETE) && msg == WM_KEYUP)
			menu::open = !menu::open;

		//if (w_param == VK_F11 && msg == WM_KEYUP)
		//	ctx->unload = true;

		hotkeys->on_wnd_proc(msg, w_param);

		if (menu::open) {
			if (ImGui_ImplWin32_WndProcHandler(hwnd, msg, w_param, l_param) || ImGui::GetIO().WantTextInput)
				return true;
		}

		return CallWindowProc((WNDPROC)old_wnd_proc, hwnd, msg, w_param, l_param);
	}
} // namespace hooks