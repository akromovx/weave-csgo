#include "cheat.hpp"

#include "../deps/weave-gui/include.hpp"

#include "hooks/hooks.hpp"
#include "interfaces.hpp"

#include "game/events.hpp"
#include "game/netvar_manager.hpp"
#include "game/override_entity_list.hpp"

#include "utils/displacement.hpp"
#include "utils/encoding.hpp"
#include "utils/threading.hpp"

#include "features/bullets.hpp"
#include "features/discord.hpp"
#include "features/menu.hpp"
#include "features/network.hpp"
#include "features/visuals/chams.hpp"
#include "features/visuals/logs.hpp"
#include "features/visuals/skin_changer.hpp"

#include "lua/api.hpp"
#include "lua/script.hpp"

#pragma comment(lib, "d3d9.lib")
#pragma comment(lib, "d3dx9.lib")
#pragma comment(lib, "winmm.lib")
#pragma comment(lib, "urlmon")
#pragma comment(lib, "ws2_32")

using namespace std::chrono_literals;

namespace cheat {
	STFI bool is_dir(std::string dir) {
		DWORD flag = GetFileAttributesA(dir.c_str());
		if (flag == XOR32S(0xFFFFFFFFUL))
			if (GetLastError() == XOR32S(ERROR_FILE_NOT_FOUND))
				return false;

		if (!(flag & XOR32S(FILE_ATTRIBUTE_DIRECTORY)))
			return false;

		return true;
	}

	STFI void create_dir_if_not_exists(std::string dir) {
		if (!is_dir(dir))
			CreateDirectoryA(dir.c_str(), NULL);
	}

#ifdef _DEBUG
	STFI void create_console() {
		AllocConsole();
		SetConsoleTitle(STRSC("csgo"));
		freopen_s((FILE**)stdout, STRSC("CONOUT$"), STRSC("w"), stdout);
	}

	STFI void destroy_console() {
		HWND console = GetConsoleWindow();

		fclose((FILE*)stdout);
		FreeConsole();

		PostMessage(console, XOR32S(WM_CLOSE), 0, 0);
	}
#endif

	CHEAT_INIT void init(LPVOID thread_parameter) {
#ifdef _DEBUG
		create_console();
#endif
		PUSH_LOG(STRSC("===========================\n"));
		PUSH_LOG(STRSC("new log started\n"));

		//if (!network::create_connection()) {
		//	PUSH_LOG(STRSC("failed to connect\n"));
		//	return;
		//}

		render::current_load_stage_name = STRS("Setting up...");

		//menu->window->set_size(gui::dpi::scale(menu->window_size));
		//menu->window->set_loading(STRS("Preparing..."));

		//gui::add_instance(menu->window);

		create_dir_if_not_exists(STRS("weave\\"));
		create_dir_if_not_exists(STRS("weave\\settings\\"));
		create_dir_if_not_exists(STRS("weave\\lua\\"));

		create_dir_if_not_exists(STRS("C:\\Weave"));
		create_dir_if_not_exists(STRS("C:\\Weave\\assets"));
		create_dir_if_not_exists(STRS("C:\\Weave\\fonts"));

		PUSH_LOG(STRSC("getting modules\n"));

		utils::wait_for_module(STRS("serverbrowser.dll"));

		//PUSH_LOG(STRSC("downloading resources\n"));
		//{
		//	if (!download_asset(STRS("logo_79x79"), STRS("csgo\\materials\\panorama\\images\\icons\\xp\\level4444.png")))
		//		PUSH_LOG(STRSC("unable to download logo_79x79\n"));

		//	if (!download_asset(STRS("airflow_logo"), STRS("csgo\\materials\\panorama\\images\\icons\\xp\\level2512.png")))
		//		PUSH_LOG(STRSC("unable to download airflow_logo\n"));

		//	if (!download_asset(STRS("logo_79x79rus"), STRS("csgo\\materials\\panorama\\images\\icons\\xp\\level44440.png")))
		//		PUSH_LOG(STRSC("unable to download logo_79x79rus\n"));

		//	if (!download_asset(STRS("karnazity_logo"), STRS("csgo\\materials\\panorama\\images\\icons\\xp\\level2001.png")))
		//		PUSH_LOG(STRSC("unable to download karnazity_logo\n"));

		//	if (!download_asset(STRS("airflow_crack"), STRS("csgo\\materials\\panorama\\images\\icons\\xp\\level25120.png")))
		//		PUSH_LOG(STRSC("unable to download airflow_crack\n"));
		//}

		PUSH_LOG(STRSC("resources downloaded\n"));

		PUSH_LOG(STRSC("network passed\n"));

		patterns::init();
		PUSH_LOG(STRSC("patterns passed\n"));
		interfaces::init();
		PUSH_LOG(STRSC("interfaces passed\n"));
		netvars->init();
		PUSH_LOG(STRSC("netvars passed\n"));
		hooks::init();
		PUSH_LOG(STRSC("hooks passed\n"));

		esp::chams->init();
		PUSH_LOG(STRSC("chams passed\n"));
		entities->init();
		PUSH_LOG(STRSC("entity list passed\n"));
		default_settings->init();
		settings->init();
		PUSH_LOG(STRSC("settings passed\n"));
		game_events->init();
		PUSH_LOG(STRSC("game events passed\n"));

		// akatsuki's dumb promise
		if (auto cvar = GET_CVAR("zoom_sensitivity_ratio_mouse"); cvar != nullptr)
			cvar->set_value(0.f);

		if (!threading->init()) {
			PUSH_LOG(STRSC("threading error\n"));
			exit(0);
			return;
		}
		PUSH_LOG(STRSC("threading passed\n"));

		gui::resources::add_pngs(network::get_images());
		gui::resources::downloaded = true;
		render::can_render = true;

		PUSH_LOG(STRSC("resources downloaded\n"));
		//menu->window->stop_loading();
		user_settings::load("settings");
		PUSH_LOG(STRSC("injected\n"));

		constexpr std::pair<color_t, color_t> colors = {
			{ 255, 139, 59 },
			{ 255, 59, 80 }
		};

		const auto weave_logo = STRS(R"(WWWWWWWW                           WWWWWWWW                                                                       
W::::::W                           W::::::W                                                                       
W::::::W                           W::::::W                                                                       
W::::::W                           W::::::W                                                                       
 W:::::W           WWWWW           W:::::W eeeeeeeeeeee    aaaaaaaaaaaaavvvvvvv           vvvvvvv eeeeeeeeeeee    
  W:::::W         W:::::W         W:::::Wee::::::::::::ee  a::::::::::::av:::::v         v:::::vee::::::::::::ee  
   W:::::W       W:::::::W       W:::::We::::::eeeee:::::eeaaaaaaaaa:::::av:::::v       v:::::ve::::::eeeee:::::ee
    W:::::W     W:::::::::W     W:::::We::::::e     e:::::e         a::::a v:::::v     v:::::ve::::::e     e:::::e
     W:::::W   W:::::W:::::W   W:::::W e:::::::eeeee::::::e  aaaaaaa:::::a  v:::::v   v:::::v e:::::::eeeee::::::e
      W:::::W W:::::W W:::::W W:::::W  e:::::::::::::::::e aa::::::::::::a   v:::::v v:::::v  e:::::::::::::::::e 
       W:::::W:::::W   W:::::W:::::W   e::::::eeeeeeeeeee a::::aaaa::::::a    v:::::v:::::v   e::::::eeeeeeeeeee  
        W:::::::::W     W:::::::::W    e:::::::e         a::::a    a:::::a     v:::::::::v    e:::::::e           
         W:::::::W       W:::::::W     e::::::::e        a::::a    a:::::a      v:::::::v     e::::::::e          
          W:::::W         W:::::W       e::::::::eeeeeeeea:::::aaaa::::::a       v:::::v       e::::::::eeeeeeee  
           W:::W           W:::W         ee:::::::::::::e a::::::::::aa:::a       v:::v         ee:::::::::::::e  
            WWW             WWW            eeeeeeeeeeeeee  aaaaaaaaaa  aaaa        vvv            eeeeeeeeeeeeee)");

		game_console->clear();
		std::this_thread::sleep_for(200ms);
		game_console->print_colored_id(STR("\n\n\n"));
		game_console->print_gradient_text(weave_logo, colors);
		game_console->print_colored_id(STR("\n\n\n"));

		skin_changer->parse();
		PUSH_LOG(STRSC("skins parsed\n"));
#ifndef _DEBUG
		PUSH_LOG(STRSC("discord-rpc\n"));
#endif
	}

	CHEAT_INIT void unload() {
		hooks::unload();
		entities->remove();
		threading->remove();

#ifdef _DEBUG
		destroy_console();
#endif
	}

	void debug_log(const char* message, ...) {
		static std::mutex mutex{};
		THREAD_SAFE(mutex);

		FILE* file = fopen(STRSC("C:\\Weave\\log.txt"), STRSC("ab"));
		_fseeki64(file, 0, SEEK_END);

		char output[4096] = {};
		va_list args;
		va_start(args, message);
		vsprintf_s(output, message, args);
		va_end(args);

		fwrite(output, 1, strlen(output), file);
		fclose(file);
	}
} // namespace cheat
