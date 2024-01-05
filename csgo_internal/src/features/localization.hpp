#pragma once
#include "network.hpp"
#include <map>
#include <string>

struct localization {
	static void apply(const network::localization_t& l);
	static std::string get(const std::string& key);
};

#define LOCALIZE(x) localization::get(STRS(x))