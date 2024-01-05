#include "localization.hpp"

#include "../utils/hotkeys.hpp"
#include "../utils/encoding.hpp"

#include <format>
#include <stdexcept>

static network::localization_t strings{};

void localization::apply(const network::localization_t& l) {
	strings = l;

	for (auto hotkey: hotkeys->m_hotkeys)
		hotkey->translate();
}

std::string localization::get(const std::string& key) {
	if (strings.contains(key))
		return strings.at(key);

#ifdef _DEBUG
	printf("unknown localize key: %s\n", key.c_str());
#endif

	return key;
}