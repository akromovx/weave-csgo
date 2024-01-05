#include "network.hpp"
//#include "../../deps/http-transmit/base64.h"
//#include "../../deps/http/http.hpp"
#include "../../deps/json/json.hpp"
#include "../../deps/weave-gui/include.hpp"
#include "../vars.hpp"

#include "../cheat.hpp"
#include "../interfaces.hpp"
#include "localization.hpp"
#include "menu.hpp"
#include "visuals/logs.hpp"
#include <format>
#include <future>
#include <regex>
#include <thread>

using namespace std::chrono_literals;
using json_t = nlohmann::json;

#ifdef _DEBUG

#define __CHEAT__ 4

#if __CHEAT__ == 3
#define CHEAT_VERSION "alpha"
#elif __CHEAT__ == 2
#define CHEAT_VERSION "beta"
#elif __CHEAT__ == 4
#define CHEAT_VERSION "lite"
#else
#define CHEAT_VERSION "release"
#endif

#define PRODUCT_ID XOR32S(__CHEAT__)

#else

#define __CHEAT__ 4

#if __CHEAT__ == 3
#define CHEAT_VERSION STRS("alpha")
#elif __CHEAT__ == 2
#define CHEAT_VERSION STRS("beta")
#elif __CHEAT__ == 4
#define CHEAT_VERSION "lite"
#else
#define CHEAT_VERSION STRS("release")
#endif

#define PRODUCT_ID XOR32S(__CHEAT__)

#endif

void network::on_frame_render_start() {

}

void network::get_subscriptions(int user_id) {
	expire_date = time(0) + 7200;
	username = "developer";
}

bool network::download_avatar(int user_id, utils::bytes_t& buffer) {
	/*buffer = http::send(dformat(STRS("v0/social/profile/avatar/{}?t={}"), user_id, (int64_t)time(0)), DEV);
	return !buffer.empty();*/
	return true;
}

bool network::get_user_id() {
	user_id = 1;
	return true;
}

bool network::setup() {
	if (!get_user_id()) {
		PUSH_LOG(STRSC("failed to auth\n"));
		return false;
	}

	get_subscriptions(user_id);
	PUSH_LOG(STRSC("auth4\n"));
	//download_avatar(user_id, network::avatar);
	PUSH_LOG(STRSC("auth5\n"));

	PUSH_LOG(STRSC("auth6\n"));

	auto l = get_localization();
	if (l.has_value())
		localization::apply(*l);

	return true;
}

std::vector<std::string> network::get_images() {
	return {
		"arrow_dropbox",
		"arrow",
		"arrow2",
		"loading",
		"loading_big",
		"save",
		"load",
		"delete",
		"share",
		"logo_100",
		"logo_125",
		"logo_150",
		"logo_175",
		"logo_200",
		"wlogo_100",
		"wlogo_125",
		"wlogo_150",
		"wlogo_175",
		"wlogo_200",
		"ragebot_icon",
		"visuals_icon",
		"skins_icon",
		"misc_icon",
		"scripts_icon",
		"settings_icon",
		"hotkeys_icon",
		"profile",
		"colorpicker_bg",
		"colorpicker_bg2",
		"updates",
		"online",
		"buy",
		"error",
		"info",
		"hit",
		"miss",
		"rollback",
		"more"
	};
}

std::optional<network::localization_t> network::get_localization() {
	const auto buf = utils::read_file_bin("C:\\Weave\\lang_native.json");
	const auto localization = json_t::parse(buf);
	return localization.get<network::localization_t>();
}