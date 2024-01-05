#pragma once
#include "../render.hpp"
#include "../utils/hotkeys.hpp"
#include "../../deps/weave-gui/include.hpp"

namespace menu {
	inline bool open = true;
	extern void render();
	inline constexpr auto max_item_width = 160.f;
	extern std::string get_time_str();

	enum class e_tabs {
		min_limit = -1,
		ragebot,
		players,
		world,
		hotkeys,
		misc,
		configs,
		scripts,
		max_limit
	};

	inline e_tabs current_tab = e_tabs::min_limit;

	inline const std::vector<std::string> tabs_list = {
		STR("Ragebot"),
		STR("Players"),
		STR("World"),
		STR("Hotkeys"),
		STR("Misc"),
		STR("Settings"),
		STR("LUA"),
	};

	static std::string get_time_str() {
		time_t now = time(0);
		tm t;
		localtime_s(&t, &now);

		static auto format_number = [](int num) {
			std::string str = std::to_string(num);

			if (str.size() == 1)
				str.insert(str.begin(), '0');

			return str;
		};

		return format_number(t.tm_mday) + STR(".") + format_number(t.tm_mon + 1) + STR(".") + format_number(1900 + t.tm_year)
			+ " " + format_number(t.tm_hour) + STR(":") + format_number(t.tm_min) + STR(":") + format_number(t.tm_sec);
	}

	__forceinline std::string get_tab_name() {
		if (current_tab > e_tabs::min_limit && current_tab < e_tabs::max_limit)
			return tabs_list[(int)current_tab];

		return STR("");
	}

	static void begin_child(std::string name, float height = 0.f) {
		ImGui::Text(name.c_str());
		ImGui::BeginChild((STR("##") + name).c_str(), ImVec2(0.f, height), true, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoScrollbar);
	}

	static void end_child() {
		ImGui::EndChild();
	}

	static void combo(std::string label, int* value, std::vector<std::string> elements) {
		ImGui::PushItemWidth(gui::dpi::scale(max_item_width));
		ImGui::Text(label.c_str());
		if (ImGui::BeginCombo((STR("##") + label).c_str(), elements[*value].c_str())) {
			for (size_t i = 0u; i < elements.size(); ++i)
				if (ImGui::Selectable(elements[i].c_str(), i == *value))
					*value = i;

			ImGui::EndCombo();
		}
		ImGui::PopItemWidth();
	}

	static void multicombo(std::string label, flags_t* value, std::vector<std::string> elements) {
		std::string preview;

		int t = 0;
		for (size_t i = 0u; i < elements.size(); i++) {
			if (value->at(i)) {
				if (t++ > 0) preview += STR(", ");
				preview += elements[i];
			}
		}

		if (preview.empty())
			preview = STR("...");

		ImGui::PushItemWidth(gui::dpi::scale(max_item_width));
		ImGui::Text(label.c_str());
		if (ImGui::BeginCombo((STR("##") + label).c_str(), preview.c_str())) {
			for (size_t i = 0u; i < elements.size(); i++) {
				std::string it = elements[i];
				if (ImGui::Selectable(it.c_str(), *value & (1 << i), ImGuiSelectableFlags_DontClosePopups))
					value->set(i, !value->at(i));
			}

			ImGui::EndCombo();
		}
		ImGui::PopItemWidth();
	}

	static void hotkey(std::string label, hotkey_t* value) {
		static const std::vector<std::string> key_strings = {
			STRS("None"), STRS("m1"), STRS("m2"), STRS("c+b"), STRS("m3"), STRS("m4"), STRS("m5"),
			STRS("unk"), STRS("bkspc"), STRS("tab"), STRS("unk"), STRS("unk"), STRS("unk"),
			STRS("enter"), STRS("unk"), STRS("unk"), STRS("shift"), STRS("ctrl"), STRS("alt"), STRS("pause"),
			STRS("caps"), STRS("unk"), STRS("unk"), STRS("unk"), STRS("unk"), STRS("unk"), STRS("unk"),
			STRS("esc"), STRS("unk"), STRS("unk"), STRS("unk"), STRS("unk"), STRS("space"), STRS("pg up"),
			STRS("pg down"), STRS("end"), STRS("home"), STRS("left"), STRS("up"), STRS("right"), STRS("down"),
			STRS("unk"), STRS("print"), STRS("unk"), STRS("ps"), STRS("ins"), STRS("del"), STRS("unk"),
			STRS("0"), STRS("1"), STRS("2"), STRS("3"), STRS("4"), STRS("5"), STRS("6"), STRS("7"),
			STRS("8"), STRS("9"), STRS("unk"), STRS("unk"), STRS("unk"), STRS("unk"), STRS("unk"), STRS("unk"),
			STRS("unk"), STRS("a"), STRS("b"), STRS("c"), STRS("d"), STRS("e"), STRS("f"), STRS("g"),
			STRS("h"), STRS("i"), STRS("j"), STRS("k"), STRS("l"), STRS("m"), STRS("n"), STRS("o"),
			STRS("p"), STRS("q"), STRS("r"), STRS("s"), STRS("t"), STRS("u"), STRS("v"), STRS("w"), STRS("x"),
			STRS("y"), STRS("z"), STRS("l-win"), STRS("r-win"), STRS("unk"), STRS("unk"), STRS("unk"),
			STRS("num0"), STRS("num1"), STRS("num2"), STRS("num 3"), STRS("num4"), STRS("num5"), STRS("num6"),
			STRS("num7"), STRS("num8"), STRS("num9"), STRS("*"), STRS("+"), STRS("_"), STRS("-"), STRS("."),
			STRS("/"), STRS("f1"), STRS("f2"), STRS("f3"), STRS("f4"), STRS("f5"), STRS("f6"), STRS("f7"),
			STRS("f8"), STRS("f9"), STRS("f10"), STRS("f11"), STRS("f12"), STRS("f13"), STRS("f14"), STRS("f15"),
			STRS("f16"), STRS("f17"), STRS("f18"), STRS("f19"), STRS("f20"), STRS("f21"), STRS("f22"), STRS("f23"),
			STRS("f24"), STRS("unk"), STRS("unk"), STRS("unk"), STRS("unk"), STRS("unk"), STRS("unk"), STRS("unk"),
			STRS("unk"), STRS("num lock"), STRS("slock"), STRS("unk"), STRS("unk"), STRS("unk"), STRS("unk"), STRS("unk"),
			STRS("unk"), STRS("unk"), STRS("unk"), STRS("unk"), STRS("unk"), STRS("unk"), STRS("unk"), STRS("unk"),
			STRS("unk"), STRS("lshift"), STRS("rshift"), STRS("lctrl"), STRS("rctrl"), STRS("lmenu"), STRS("rmenu"),
			STRS("unk"), STRS("unk"), STRS("unk"), STRS("unk"), STRS("unk"), STRS("unk"), STRS("unk"), STRS("unk"),
			STRS("unk"), STRS("unk"), STRS("nxt"), STRS("prv"), STRS("stop"), STRS("play"), STRS("unk"), STRS("unk"),
			STRS("unk"), STRS("unk"), STRS("unk"), STRS("unk"), STRS(";"), STRS("+"), STRS(","), STRS("-"), STRS("."),
			STRS("/?"), STRS("~"), STRS("unk"), STRS("unk"), STRS("unk"), STRS("unk"), STRS("unk"), STRS("unk"),
			STRS("unk"), STRS("unk"), STRS("unk"), STRS("unk"), STRS("unk"), STRS("unk"), STRS("unk"), STRS("unk"),
			STRS("unk"), STRS("unk"), STRS("unk"), STRS("unk"), STRS("unk"), STRS("unk"), STRS("unk"), STRS("unk"),
			STRS("unk"), STRS("unk"), STRS("unk"), STRS("unk"), STRS("[{"), STRS("\\|"), STRS("}]"), STRS("'\""),
			STRS("unk"), STRS("unk"), STRS("unk"), STRS("unk"), STRS("unk"), STRS("unk"), STRS("unk"), STRS("unk"),
			STRS("unk"), STRS("unk"), STRS("unk"), STRS("unk"), STRS("unk"), STRS("unk"), STRS("unk"), STRS("unk"),
			STRS("unk"), STRS("unk"), STRS("unk"), STRS("unk"), STRS("unk"), STRS("unk"), STRS("unk"), STRS("unk"),
			STRS("unk"), STRS("unk"), STRS("unk"), STRS("unk"), STRS("unk"), STRS("unk"), STRS("unk"), STRS("unk"), STRS("unk")
		};
		static std::unordered_map<std::string, bool> actives{};

		static const std::vector<std::string> types = { STRS("Hold"), STRS("Toggle"), STRS("Always on") };

		ImGui::Text(label.c_str());

		ImGui::PushItemWidth(gui::dpi::scale(100.f));

		if (ImGui::BeginCombo((STR("##") + label).c_str(), types[(int)value->m_type].c_str())) {
			for (size_t i = 0u; i < types.size(); ++i)
				if (ImGui::Selectable(types[i].c_str(), i == (int)value->m_type))
					value->m_type = (hotkey_t::e_type)i;

			ImGui::EndCombo();
		}

		ImGui::SameLine();

		auto& active = actives[label];

		std::string lbl{};
		if (active)
			lbl = STR("...");
		else
			lbl = key_strings[value->m_key];

		lbl += STR("##") + label;

		if (ImGui::Button(lbl.c_str(), ImVec2(56, 0)) && !active)
			active = true;

		if (active) {
			ImGui::GetIO().WantTextInput = true;
			hotkeys->m_key_binder_active = true;

			if (hotkeys->m_last_key_pressed > 0) {
				if (hotkeys->m_last_key_pressed == VK_ESCAPE) {
					value->m_key = 0;

					hotkeys->m_last_key_pressed = 0;
					hotkeys->m_key_binder_active = false;
				}
				else {
					value->m_key = hotkeys->m_last_key_pressed;
					hotkeys->m_last_key_pressed = 0;
					hotkeys->m_key_binder_active = false;
				}
				active = false;
			}
		}

		ImGui::PopItemWidth();
	}

	static bool checkbox_with_color(std::string label, bool* v, float* clr, bool alpha_slider = true) {
		bool ret = ImGui::Checkbox(label.c_str(), v);

		if (*v) {
			ImGui::SameLine();
			if (alpha_slider)
				ImGui::ColorEdit4((STR("##") + label).c_str(), clr, ImGuiColorEditFlags_AlphaBar);
			else
				ImGui::ColorEdit3((STR("##") + label).c_str(), clr);
		}

		return ret;
	}

	static void slider_int(std::string label, int* v, int min, int max, std::string format = STR("%d")) {
		ImGui::PushItemWidth(gui::dpi::scale(max_item_width));
		ImGui::Text(label.c_str());
		ImGui::SliderInt((STR("##") + label).c_str(), v, min, max, format.c_str());
		ImGui::PopItemWidth();
	}

	static void slider_float(std::string label, float* v, float min, float max, std::string format = STR("%.2f")) {
		ImGui::PushItemWidth(gui::dpi::scale(max_item_width));
		ImGui::Text(label.c_str());
		ImGui::SliderFloat((STR("##") + label).c_str(), v, min, max, format.c_str());
		ImGui::PopItemWidth();
	}
}