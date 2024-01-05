#include "cheat.hpp"
#include "hooks/hooks.hpp"

using namespace std::chrono_literals;

static DWORD cheat_load(LPVOID thread_parameter) {
	while (!(ctx->hwnd = FindWindowA(STRSC("Valve001"), NULL)))
		std::this_thread::sleep_for(200ms);

	cheat::init(thread_parameter);

	while (!ctx->unload)
		std::this_thread::sleep_for(200ms);

	cheat::unload();

	SetWindowLongA(ctx->hwnd, XOR32S(GWL_WNDPROC), (LONG_PTR)hooks::old_wnd_proc);
	FreeLibraryAndExitThread(ctx->m_module, XOR32S(0));
	return 0;
}

BOOL APIENTRY DllMain(HMODULE hmodule, DWORD reason_for_call, LPVOID reserved) {
	switch (reason_for_call) {
		case DLL_PROCESS_ATTACH:
			CreateThread(0, XOR32S(0), (LPTHREAD_START_ROUTINE)cheat_load, reserved, XOR32S(0), 0);
			ctx->m_module = hmodule;
			break;
	}

	return TRUE;
}