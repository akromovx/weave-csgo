#include "sdk.hpp"
#include "../features/visuals/logs.hpp"
#include "../utils/sha2.hpp"
#include "../vars.hpp"
#include "../cheat.hpp"

namespace lua {
	std::string convert_name(const std::string& name) {
		return name;
		//return STRS("_") + sha512::get(name + std::to_string(BASE_ADDRESS));
	}

	void perform_error(const std::string& text, bool critical) {
		cheat_logs->add_error(text);
		PUSH_LOG((text + "\n").c_str());
	}
} // namespace lua