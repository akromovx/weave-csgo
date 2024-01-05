#pragma once
#include "../base_includes.hpp"
#include "../../sdk/prediction.hpp"

struct knifebot_t {
	void on_create_move(sdk::user_cmd_t* cmd);
};

GLOBAL_DYNPTR(knifebot_t, knifebot);