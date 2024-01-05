/*

	if you want to use this menu, you need to check "cloud" folder and understand how it works
	i really don't recommend to recreate server logic for it
	the easier way is to uncomment this code and make client-side logic for it

	good luck :)

*/

//#include "menu.hpp"
//#include "../../deps/http/http.hpp"
//#include "../features/misc.hpp"
//#include "../features/visuals/logs.hpp"
//#include "../features/visuals/skin_changer.hpp"
//#include "../lua/api.hpp"
//#include "../render.hpp"
//#include "../utils/hotkeys.hpp"
//#include "../utils/sha2.hpp"
//#include "../utils/threading.hpp"
//#include "../vars.hpp"
//#include "localization.hpp"
//#include "network.hpp"
//#include <algorithm>
//#include <chrono>
//#include <ctime>
//#include <filesystem>
//#include <fstream>
//#include "../../deps/fontawesome/include.hpp"
//
//#define ICON(x) fa::icons.at(STRS(x))
//
//constexpr auto max_tabs = 11;
//
//using namespace gui;
//using namespace std::chrono_literals;
//
//static std::string search_phrase{};
//
//static std::string create_config_name{};
//static std::string import_config_name{};
//static int active_config_index{};
//static network::configs_t active_configs{};
//
//static std::string import_script_name{};
//static int active_script_index{};
//static network::scripts_t active_scripts{};
//
//void perform_error(std::string text) {
//	msgbox::add(text + STRS("\nThe game will close in 5 seconds.")); /*
//	std::this_thread::sleep_for(5s);
//	exit(0);*/
//}
//
//STFI utils::bytes_t read_file_bin(std::string path) {
//	utils::bytes_t result;
//
//	std::ifstream ifs{ path, std::ios::binary | std::ios::ate };
//	if (ifs) {
//		std::ifstream::pos_type pos = ifs.tellg();
//		result.resize((size_t)pos);
//
//		ifs.seekg(0, std::ios::beg);
//		ifs.read((char*)result.data(), pos);
//	}
//
//	return result;
//}
//
//STFI std::vector<fs::path> get_files_with_extension(const std::string& directory, const std::string& extension) {
//	std::vector<fs::path> filenames{};
//
//	for (const auto& entry: fs::directory_iterator{ directory })
//		if (entry.is_regular_file() && entry.path().extension() == extension)
//			filenames.push_back(entry.path());
//
//	return filenames;
//}
//
//STFI std::string remove_extension(const std::string& s, const std::string& ext) {
//	if (s.find(ext) == std::string::npos)
//		return s;
//
//	return s.substr(0, s.size() - ext.size());
//}
//
//void resources::download() {
//	downloaded = true;
//}
//
//static std::function<std::string(int)> get_slider_format(uint8_t type) {
//	switch (type) {
//		case 1:
//			return [](int value) { return dformat(STR("{}%"), value); };
//		case 2:
//			return [](int value) {
//				if (value > 100)
//					return dformat(STR("Health + {}hp"), value - 100);
//
//				return dformat(STR("{}hp"), value);
//			};
//		case 3:
//			return [](int value) {
//				if (value == 0)
//					return STRS("Auto");
//
//				return dformat("{}%", value);
//			};
//		case 4:
//			return [](int value) { return dformat((char*)u8"{}°", value); };
//		case 5:
//			return [](int value) { return dformat(STR("{}px"), value); };
//	}
//
//	return [](int value) { return std::format("{}", value); };
//}
//
//static void update_dpi_scale(bool updated) {
//	if (!updated)
//		return;
//
//	switch (settings->dpi_scale) {
//		case 0: dpi::change_scale(1.f); break;
//		case 1: dpi::change_scale(1.25f); break;
//		case 2: dpi::change_scale(1.5f); break;
//		case 3: dpi::change_scale(1.75f); break;
//		case 4: dpi::change_scale(2.f); break;
//	}
//
//	user_settings::save();
//}
//
//static i_renderable* subtabs(const std::vector<std::string>& items, int& i) {
//	auto column = new containers::group_t{};
//
//	c_style style{};
//	style.text_color = { 127, 127, 127 };
//	style.controls_text_padding = { 16.f, 4.f };
//	style.container_padding.x = 24.f;
//	style.transparent_clickable = true;
//	style.invisible_clickable = true;
//	style.small_text = false;
//	style.window_padding.y = 0.f;
//
//	column->set_style(style);
//
//	for (const auto& name: items) {
//		auto item = new controls::button_t{ name, [=]() { menu->change_tab(i); }, vec2d{ 164.f - 16.f, 26.f } * dpi::_get_actual_scale(), e_items_align::start };
//		style.small_text = true;
//
//		style.set_callback([=](c_style& s) {
//			const auto active = menu->m_active_tab == i;
//			s.bold_text = active;
//			s.text_color = active ? color_t{ 255, 255, 255 } : color_t{ 127, 127, 127 };
//		});
//
//		item->set_style(style);
//		column->add(item);
//		++i;
//	}
//
//	return column;
//}
//
//static void create_navigation(const std::vector<menu_t::nav_node_t>& nodes) {
//	auto nav = new containers::group_t{ "", vec2d{ 172.f, 316.f } * dpi::_get_actual_scale() };
//	nav->set_position_type(e_position_type::absolute);
//	nav->set_position(vec2d{ 16.f - styles::get().container_padding.x, 89.f } * dpi::_get_actual_scale());
//	const auto tab_size = vec2d{ 164.f, 32.f } * dpi::_get_actual_scale();
//
//	nav->override_style().bold_text = true;
//	nav->override_style().text_color = { 127, 127, 127 };
//	nav->override_style().transparent_clickable = true;
//
//	int i = 0;
//	for (auto& node: nodes) {
//		if (!node.m_children.empty()) {
//			auto tab = new controls::collapse_t{ node.m_name, []() {}, tab_size, node.m_icon };
//			tab->set_item(subtabs(node.m_children, i));
//			nav->add(tab);
//		} else {
//			auto tab = new controls::button_t{ node.m_name, [=]() { menu->change_tab(i); }, tab_size, e_items_align::start, node.m_icon };
//
//			tab->override_style().bold_text = true;
//			tab->override_style().text_color = { 127, 127, 127 };
//			tab->override_style().transparent_clickable = true;
//
//			tab->override_style().clickable_hovered = { 100, 100, 100 };
//			tab->override_style().set_callback([=](c_style& s) {
//				s.text_color = menu->m_active_tab == i ? color_t{ 255, 255, 255 } : color_t{ 127, 127, 127 };
//			});
//
//			if (node.m_colors.has_value()) {
//				tab->override_style().accent_color1 = node.m_colors->first;
//				tab->override_style().accent_color2 = node.m_colors->second;
//				tab->override_style().gradient_text_in_button = true;
//			}
//
//			nav->add(tab);
//			++i;
//		}
//	}
//
//	menu->window->add(nav);
//}
//
//static void create_navigation() {
//	std::vector<menu_t::nav_node_t> nodes{};
//
//	auto create_tab = [&](const std::string& name, void* icon, const std::vector<std::string>& subtabs = {}) -> menu_t::nav_node_t& {
//		auto& tab = nodes.emplace_back();
//		tab.m_name = name;
//		tab.m_children = subtabs;
//		tab.m_icon = icon;
//		return tab;
//	};
//
//	create_tab(LOCALIZE("tab.ragebot"), resources::get_texture(HASH("ragebot_icon")),
//			   { LOCALIZE("tab.ragebot.aimbot"),
//				 LOCALIZE("tab.ragebot.antiaim"),
//				 LOCALIZE("tab.ragebot.misc") });
//
//	create_tab(LOCALIZE("tab.visuals"), resources::get_texture(HASH("visuals_icon")),
//			   { LOCALIZE("tab.visuals.players"),
//				 LOCALIZE("tab.visuals.local"),
//				 LOCALIZE("tab.visuals.world"),
//				 LOCALIZE("tab.visuals.other") });
//
//	create_tab(LOCALIZE("tab.misc"), resources::get_texture(HASH("misc_icon")));
//	create_tab(LOCALIZE("tab.skins"), resources::get_texture(HASH("skins_icon")));
//	create_tab(LOCALIZE("tab.scripts"), resources::get_texture(HASH("scripts_icon")));
//	create_tab(LOCALIZE("tab.settings"), resources::get_texture(HASH("settings_icon")));
//	create_tab(LOCALIZE("tab.hotkeys"), resources::get_texture(HASH("hotkeys_icon")));
//
//	for (const auto& [id, tab]: menu->m_lua_tabs) {
//		auto& t = create_tab(tab.m_name, tab.m_icon);
//		t.m_colors = tab.m_colors;
//	}
//
//	return create_navigation(nodes);
//}
//
//static void create_footer() {
//	auto footer = new containers::group_t{ "", vec2d{ 0.f, 40.f } * dpi::_get_actual_scale() };
//	c_style style{};
//	style.container_padding = {};
//	style.disable_scroll = true;
//
//	footer->set_style(style);
//
//	footer->set_position_type(e_position_type::absolute);
//	footer->set_position(vec2d{ 20.f, 432.f } * dpi::_get_actual_scale());
//
//	auto row = new containers::row_t{};
//	row->add(controls::gif(new render::gif_t{ network::avatar }, vec2d{ 40.f, 40.f } * dpi::_get_actual_scale(), true,
//						   []() {
//							   menu->change_tab(-1);
//						   }));
//
//	row->set_style({});
//
//	auto column = new containers::column_t{ e_items_align::center };
//	column->add(new controls::text_t{ network::username, true });
//
//	c_style sub_style{};
//	sub_style.text_color = { 127, 127, 127 };
//	sub_style.small_text = true;
//
//	auto sub = new controls::text_t{ dformat(STRS("expires in: {}"), format_time_rel(network::expire_date - time(0))) };
//	sub->set_style(sub_style);
//
//	column->add(sub);
//
//	row->add(column);
//	footer->add(row);
//	menu->window->add(footer);
//}
//
//void menu_t::create_gui() {
//	menu->header = new containers::group_t{};
//	menu->content = new containers::group_t{ "", vec2d{ 532.f + 8.f, 372.f } * dpi::_get_actual_scale() };
//	menu->content->override_style().scroll_fade = true;
//
//	__menu::create_header();
//	create_footer();
//	__menu::on_tab_change();
//
//	menu->window->add(menu->header);
//	menu->window->add(menu->content);
//
//	create_navigation();
//}
//
//void menu_t::recreate_gui(std::function<void()> callback) {
//	static std::mutex mtx{};
//	THREAD_SAFE(mtx);
//
//	menu->window->update_state([callback]() {
//		if (callback != nullptr)
//			callback();
//
//		menu->window->clear();
//
//		menu->header = new containers::group_t{};
//		menu->content = new containers::group_t{ "", vec2d{ 532.f + 8.f, 372.f } * dpi::_get_actual_scale() };
//		menu->content->override_style().scroll_fade = true;
//
//		__menu::create_header();
//		create_footer();
//		__menu::on_tab_change();
//
//		menu->window->add(menu->header);
//		menu->window->add(menu->content);
//
//		create_navigation();
//	});
//}
//
//static auto create_popup(vec2d size, int flags) {
//	auto popup = menu->window->create_popup();
//	popup->m_flags.add(flags);
//	popup->set_position(globals().mouse_position - menu->window->get_position());
//	popup->set_size(size * dpi::_get_actual_scale());
//	return popup;
//}
//
//static void confirm_action(const std::string& text, callback_t action, bool prefer_no = false) {
//	auto popup = create_popup({ 0.f, 80.f }, container_flag_is_popup | popup_flag_animation | popup_flag_close_on_click | container_flag_visible);
//
//	auto group = new containers::group_t{};
//	group->add(controls::text(text));
//
//	auto row = new containers::row_t{};
//
//	group->override_style().container_padding = { 8.f, 8.f };
//	group->override_style().window_padding.y = 32.f;
//
//	row->add(controls::button(LOCALIZE("yes"), action, vec2d{ 0.f, 24.f } * dpi::_get_actual_scale(), e_items_align::center, nullptr, !prefer_no));
//	row->add(controls::button(
//			LOCALIZE("no"), [=]() { popup->close(); }, vec2d{ 0.f, 24.f } * dpi::_get_actual_scale(), e_items_align::center, nullptr, prefer_no));
//
//	group->add(row);
//	popup->add(group);
//}
//
//static void to_clipboard(const std::string& text) {
//	if (OpenClipboard(0)) {
//		EmptyClipboard();
//		auto clip_data = (char*)(GlobalAlloc(GMEM_FIXED, MAX_PATH));
//		lstrcpyA(clip_data, text.c_str());
//		SetClipboardData(CF_TEXT, (HANDLE)(clip_data));
//		LCID* lcid = (DWORD*)(GlobalAlloc(GMEM_FIXED, sizeof(DWORD)));
//		*lcid = MAKELCID(MAKELANGID(LANG_RUSSIAN, SUBLANG_NEUTRAL), SORT_DEFAULT);
//		SetClipboardData(CF_LOCALE, (HANDLE)(lcid));
//		CloseClipboard();
//	}
//}
//
//static i_renderable* config_actions() {
//	auto cfg_load = [=]() {
//		menu->content->update_state([=]() {
//			menu->content->set_loading();
//			menu->window->lock();
//			std::thread([=]() {
//				auto& cfg = active_configs[active_config_index];
//				utils::bytes_t buffer{};
//				const auto loaded = (network::load_config(buffer, cfg.m_id));
//
//				if (loaded)
//					menu->parse_items(buffer);
//
//				__menu::on_tab_change();
//				menu->window->unlock();
//				menu->content->stop_loading();
//
//				if (loaded)
//					cheat_logs->add_info(dformat(STRS("\"{}\" has been loaded!"), cfg.m_name));
//				else
//					cheat_logs->add_error(dformat(STRS("Can't load \"{}\"!"), cfg.m_name));
//
//				lua::config_callback(STRS("config_load"), cfg);
//			}).detach();
//		});
//	};
//
//	auto cfg_rollback = [=]() {
//		menu->content->update_state([=]() {
//			menu->content->set_loading();
//			menu->window->lock();
//			std::thread([=]() {
//				auto& cfg = active_configs[active_config_index];
//				utils::bytes_t buffer{};
//				const auto loaded = (network::rollback_config(buffer, cfg.m_id));
//
//				if (loaded)
//					menu->parse_items(buffer);
//
//				__menu::on_tab_change();
//				menu->window->unlock();
//				menu->content->stop_loading();
//
//				if (loaded)
//					cheat_logs->add_info(dformat(STRS("\"{}\" has been rolled back!"), cfg.m_name));
//				else
//					cheat_logs->add_error(dformat(STRS("Can't roll back \"{}\"!"), cfg.m_name));
//
//				lua::config_callback(STRS("config_load"), cfg);
//			}).detach();
//		});
//	};
//
//	auto cfg_save = [=]() {
//		confirm_action(LOCALIZE("config.save_confirmation"), []() {
//			menu->content->update_state([=]() {
//				menu->content->set_loading();
//				menu->window->lock();
//				std::thread([=]() {
//					auto& cfg = active_configs[active_config_index];
//					network::save_config(cfg.m_name, cfg.m_id, cfg.m_private);
//					__menu::on_tab_change();
//					menu->window->unlock();
//					menu->content->stop_loading();
//					cheat_logs->add_info(dformat(STRS("\"{}\" has been saved!"), cfg.m_name));
//
//					lua::config_callback(STRS("config_save"), cfg);
//				}).detach();
//			});
//		});
//	};
//
//	auto cfg_share = [=]() {
//		auto& cfg = active_configs[active_config_index];
//		to_clipboard(cfg.m_id);
//		cheat_logs->add_info(LOCALIZE("config.id_copied"));
//	};
//
//	auto cfg_delete = [=]() {
//		confirm_action(
//				LOCALIZE("config.delete_confirmation"),
//				[=]() {
//					menu->content->update_state([=]() {
//						menu->content->set_loading();
//						menu->window->lock();
//						std::thread([=]() {
//							auto& cfg = active_configs[active_config_index];
//							network::delete_config(cfg.m_id);
//							__menu::on_tab_change();
//							menu->window->unlock();
//							menu->content->stop_loading();
//						}).detach();
//					});
//				},
//				true);
//	};
//
//	auto group = new containers::group_t{};
//
//	c_style s{};
//	s.container_padding = { 0.f, 8.f };
//	s.window_padding.y = 32.f;
//	s.bold_text = true;
//
//	group->set_style(s);
//
//	auto btn_size = vec2d{ 116.f, 32.f } * dpi::_get_actual_scale();
//	s.container_padding = { 8.f, 8.f };
//
//	{
//		auto r = new containers::row_t{};
//		r->set_style(s);
//		r->add(controls::button(LOCALIZE("config.load"), cfg_load, btn_size, e_items_align::center, resources::get_texture(HASH("load")), true));
//		r->add(controls::button(LOCALIZE("config.save"), cfg_save, btn_size, e_items_align::center, resources::get_texture(HASH("save")), false));
//		group->add(r);
//	}
//
//	{
//		auto r = new containers::row_t{};
//		r->set_style(s);
//		r->add(controls::button(LOCALIZE("config.share"), cfg_share, btn_size, e_items_align::center, resources::get_texture(HASH("share")), false));
//		r->add(controls::button(LOCALIZE("config.delete"), cfg_delete, btn_size, e_items_align::center, resources::get_texture(HASH("delete")), false));
//		group->add(r);
//	}
//
//	{
//		auto r = new containers::row_t{};
//		r->set_style(s);
//		r->add(controls::button(LOCALIZE("config.rollback"), cfg_rollback, btn_size, e_items_align::center, resources::get_texture(HASH("rollback")), false));
//		group->add(r);
//	}
//
//	return group;
//}
//
//static i_renderable* script_actions() {
//	auto script_load = [=]() {
//		menu->content->update_state([=]() {
//			menu->content->set_loading();
//			menu->window->lock();
//			std::thread([=]() {
//				auto& active_script = active_scripts[active_script_index];
//				if (!active_script.m_exist_on_server || active_script.m_id.empty()) {
//					auto bytes = read_file_bin(STRS("weave\\lua\\") + active_script.m_name);
//					active_script.m_last_uploaded_hash = sha512::get(bytes);
//					if (!network::create_script(active_script, { bytes.begin(), bytes.end() }))
//						cheat_logs->add_error(STRS("Failed to create script!"));
//				} else if (!active_script.m_loaded_on_server) {
//					auto bytes = read_file_bin(STRS("weave\\lua\\") + active_script.m_name);
//					active_script.m_last_uploaded_hash = sha512::get(bytes);
//					if (!network::upload_script(active_script.m_id, active_script.m_name, { bytes.begin(), bytes.end() }, active_script.m_private))
//						cheat_logs->add_error(STRS("Failed to load script!"));
//				}
//
//				if (!active_script.m_id.empty()) {
//					active_script.m_loaded_on_server = true;
//					active_script.m_exist_on_server = true;
//
//					const auto script = network::get_script(active_script.m_id);
//					if (script.has_value())
//						lua::load_script(remove_extension(active_script.m_name, STRS(".lua")), active_script.m_id, *script);
//					else
//						lua::perform_error(STRS("Failed to load script: empty file"));
//				}
//
//				menu->recreate_gui();
//				menu->window->unlock();
//				menu->content->stop_loading();
//			}).detach();
//		});
//	};
//
//	auto script_unload = [=]() {
//		menu->content->update_state([=]() {
//			menu->content->set_loading();
//			menu->window->lock();
//			std::thread([=]() {
//				lua::unload_script(active_scripts[active_script_index].m_id);
//				menu->recreate_gui();
//				menu->window->unlock();
//				menu->content->stop_loading();
//			}).detach();
//		});
//	};
//
//	auto script_share = [=]() {
//		auto& cfg = active_scripts[active_script_index];
//		to_clipboard(cfg.m_id);
//		cheat_logs->add_info(STRS("Script ID has been copied!"));
//	};
//
//	auto script_delete = [=]() {
//		confirm_action(
//				STRS("Are you sure you want to delete this script?"), [=]() {
//					menu->content->update_state([=]() {
//						menu->content->set_loading();
//						menu->window->lock();
//						std::thread([=]() {
//							network::delete_script(active_scripts[active_script_index].m_id);
//							__menu::on_tab_change();
//							menu->window->unlock();
//							menu->content->stop_loading();
//						}).detach();
//					});
//				},
//				true);
//	};
//
//	auto group = new containers::group_t{};
//
//	c_style s{};
//	s.container_padding = { 0.f, 8.f };
//	s.window_padding.y = 32.f;
//	s.bold_text = true;
//
//	group->set_style(s);
//
//	auto btn_size = vec2d{ 116.f, 32.f } * dpi::_get_actual_scale();
//	s.container_padding = { 8.f, 8.f };
//
//	{
//		auto r = new containers::row_t{};
//		r->set_style(s);
//
//		if (!lua::is_loaded(active_scripts[active_script_index].m_id))
//			r->add(controls::button(LOCALIZE(("script.load")), script_load, btn_size, e_items_align::center, resources::get_texture(HASH("load")), true));
//		else
//			r->add(controls::button(LOCALIZE(("script.unload")), script_unload, btn_size, e_items_align::center, resources::get_texture(HASH("save")), false));
//
//		group->add(r);
//	}
//
//	auto& active_script = active_scripts[active_script_index];
//	if (!lua::is_loaded(active_script.m_id) && active_script.m_exist_on_server) {
//		auto r = new containers::row_t{};
//		r->set_style(s);
//
//		if (active_script.m_loaded_on_server)
//			r->add(controls::button(LOCALIZE(("script.share")), script_share, btn_size, e_items_align::center, resources::get_texture(HASH("share")), false));
//
//		r->add(controls::button(LOCALIZE(("script.delete")), script_delete, btn_size, e_items_align::center, resources::get_texture(HASH("delete")), false));
//		group->add(r);
//	}
//
//	return group;
//}
//
//static auto create_nested_items() {
//	auto popup = create_popup({}, gui::container_flag_is_popup | gui::popup_flag_animation | gui::container_flag_visible);
//	popup->override_style().clickable_color = { 24, 24, 24 };
//	popup->override_style().clickable_hovered = { 28, 28, 28 };
//
//	return popup;
//}
//
//static void hotkeys_tab() {
//	auto group = new containers::column_t{};
//
//	auto add_hotkey = [&](std::string name, hotkey_t& hotkey, void* icon = nullptr) {
//		auto nest_items = [h = &hotkey]() {
//			auto items = create_nested_items();
//			items->add(controls::checkbox(STRS("Show in hotkeys list"), &h->m_show_in_binds, user_settings::save));
//		};
//
//		group->add(controls::hotkey(name, (int*)&hotkey.m_type, &hotkey.m_key, icon, nest_items, user_settings::save));
//	};
//
//	add_hotkey(LOCALIZE(("hotkey.doubletap")), hotkeys->doubletap, resources::get_texture(HASH("ragebot_icon")));
//	add_hotkey(LOCALIZE(("hotkey.hide_shot")), hotkeys->hide_shot, resources::get_texture(HASH("ragebot_icon")));
//	add_hotkey(LOCALIZE(("hotkey.override_damage")), hotkeys->override_damage, resources::get_texture(HASH("ragebot_icon")));
//	add_hotkey(LOCALIZE(("hotkey.fake_duck")), hotkeys->fake_duck, resources::get_texture(HASH("ragebot_icon")));
//	add_hotkey(LOCALIZE(("hotkey.desync_inverter")), hotkeys->desync_inverter, resources::get_texture(HASH("ragebot_icon")));
//	add_hotkey(LOCALIZE(("hotkey.slow_walk")), hotkeys->slow_walk, resources::get_texture(HASH("ragebot_icon")));
//	add_hotkey(LOCALIZE(("hotkey.freestand")), hotkeys->freestand, resources::get_texture(HASH("ragebot_icon")));
//
//	add_hotkey(LOCALIZE(("hotkey.manual_right")), hotkeys->manual_right, resources::get_texture(HASH("ragebot_icon")));
//	add_hotkey(LOCALIZE(("hotkey.manual_left")), hotkeys->manual_left, resources::get_texture(HASH("ragebot_icon")));
//	add_hotkey(LOCALIZE(("hotkey.manual_back")), hotkeys->manual_back, resources::get_texture(HASH("ragebot_icon")));
//	add_hotkey(LOCALIZE(("hotkey.manual_forward")), hotkeys->manual_forward, resources::get_texture(HASH("ragebot_icon")));
//
//	add_hotkey(LOCALIZE(("hotkey.peek_assist")), hotkeys->peek_assist, resources::get_texture(HASH("misc_icon")));
//	add_hotkey(LOCALIZE(("hotkey.thirdperson")), hotkeys->thirdperson, resources::get_texture(HASH("visuals_icon")));
//
//	menu->content->add(group);
//}
//
//static i_renderable* configs_selectables(int* value, const std::vector<std::string>& items, void* texture, std::function<void(bool)> on_update) {
//	auto group = new containers::group_t{};
//	group->override_style().container_padding = { 0.f, 0.f };
//	group->override_style().text_color = { 127, 127, 127 };
//	group->override_style().transparent_clickable = true;
//
//	const auto button_size = vec2d{ 256.f - 8.f, 32.f } * dpi::_get_actual_scale();
//
//	int i = 0;
//	for (const auto& it: items) {
//		auto callback = [on_update, i, value]() {
//			const auto updated = i != *value;
//			on_update(updated);
//			*value = i;
//		};
//
//		auto btn = controls::button(it, callback, button_size, e_items_align::start, texture);
//		btn->override_style().container_padding.y = 8.f;
//		btn->override_style().transparent_clickable = true;
//		btn->override_style().set_callback([value, i](c_style& s) {
//			if (*value == i) {
//				s.text_color = color_t{ 255, 255, 255 };
//				s.bold_text = true;
//				s.transparent_clickable = false;
//				s.clickable_hovered = { 24, 24, 24 };
//			} else {
//				s.text_color = color_t{ 127, 127, 127 };
//				s.bold_text = false;
//				s.transparent_clickable = true;
//				s.clickable_hovered = { 75, 75, 75 };
//			}
//		});
//
//		group->add(btn);
//		++i;
//	}
//
//	return group;
//}
//
//static void settings_tab() {
//	active_configs.clear();
//	if (!network::get_configs(active_configs))
//		return;
//
//	menu_t::content_items_t items{};
//	std::vector<std::string> configs_list{};
//	configs_list.reserve(active_configs.size());
//	for (auto& config: active_configs)
//		configs_list.emplace_back(config.m_name);
//
//	auto configs_dyngroup = new containers::group_t{};
//
//	auto configs_update = [=](bool updated) {
//		if (!updated)
//			return;
//
//		if (active_configs.empty()) {
//			configs_dyngroup->update_state([=]() {
//				c_style s{};
//				s.text_color = { 127, 127, 127 };
//				auto t = controls::text(STRS("Please, choose some config to see details"));
//				t->set_style(s);
//				configs_dyngroup->add(t);
//			});
//
//			return;
//		}
//
//		menu->window->lock();
//		configs_dyngroup->set_loading();
//
//		std::thread([=]() {
//			network::config_t cfg{};
//			active_config_index = std::clamp<size_t>(active_config_index, 0, active_configs.size() - 1);
//			if (network::get_config(cfg, active_configs[active_config_index].m_id)) {
//				configs_dyngroup->update_state([=]() {
//					active_configs[active_config_index] = cfg;
//					configs_dyngroup->add(controls::text_input(&active_configs[active_config_index].m_name, STRS("Enter config name...")));
//					configs_dyngroup->add(controls::prompt(LOCALIZE("config.author") + STRS(": "), cfg.m_author));
//
//					if (cfg.m_author != cfg.m_last_update_by)
//						configs_dyngroup->add(controls::prompt(LOCALIZE("config.last_update_by") + STRS(": "), cfg.m_last_update_by));
//
//					configs_dyngroup->add(controls::prompt(LOCALIZE("config.last_update_at") + STRS(": "),
//														   dformat(STRS("{} ago"), format_time_rel(time(0) - cfg.m_last_update_at))));
//
//					configs_dyngroup->add(controls::prompt(LOCALIZE("config.created_at") + STRS(": "),
//														   dformat(STRS("{} ago"), format_time_rel(time(0) - cfg.m_created_at))));
//
//					configs_dyngroup->add(controls::checkbox(LOCALIZE("private"), &active_configs[active_config_index].m_private));
//					configs_dyngroup->add(config_actions());
//				});
//			}
//
//			menu->window->unlock();
//			configs_dyngroup->stop_loading();
//		}).detach();
//	};
//
//	configs_update(true);
//
//	if (configs_list.empty()) {
//		c_style s{};
//		s.text_color = { 127, 127, 127 };
//		auto t = controls::text(STRS("There's no configs at the moment"));
//		t->set_style(s);
//		items.first.emplace_back(t);
//	} else {
//		items.first.emplace_back(configs_selectables(&active_config_index, configs_list, resources::get_texture(HASH("settings_icon")), configs_update));
//	}
//
//	auto add_config = []() {
//		if (active_configs.size() >= 10) {
//			msgbox::add(LOCALIZE(("max_configs_error")));
//			return;
//		}
//
//		if (create_config_name.empty()) {
//			msgbox::add(STRS("Enter the config name!"));
//			return;
//		}
//
//		menu->content->update_state([]() {
//			menu->content->set_loading();
//			menu->window->lock();
//			std::thread([]() {
//				network::create_config(create_config_name);
//				create_config_name.clear();
//				__menu::on_tab_change();
//				menu->window->unlock();
//				menu->content->stop_loading();
//			}).detach();
//		});
//	};
//
//	auto import_config = []() {
//		if (active_configs.size() >= 10) {
//			msgbox::add(STRS("You can't create more than 10 configs!"));
//			return;
//		}
//
//		if (import_config_name.empty()) {
//			msgbox::add(STRS("Enter the config id!"));
//			return;
//		}
//
//		menu->content->update_state([]() {
//			menu->content->set_loading();
//			menu->window->lock();
//			std::thread([]() {
//				if (!network::import_config(import_config_name))
//					msgbox::add(STRS("Can not import config!"));
//
//				import_config_name.clear();
//				__menu::on_tab_change();
//				menu->window->unlock();
//				menu->content->stop_loading();
//			}).detach();
//		});
//	};
//
//	{
//		auto row = new containers::row_t{};
//
//		auto config_name = controls::text_input(&create_config_name, LOCALIZE("config.name_input"));
//
//		c_style s{};
//		s.item_width = 176.f;
//
//		config_name->set_style(s);
//		s.container_padding = { 8.f, 0.f };
//		row->set_style(s);
//		row->add(config_name);
//		row->add(controls::button(LOCALIZE(("config.create")), add_config, vec2d{ 64.f, 32.f } * dpi::_get_actual_scale(), e_items_align::center, nullptr, active_configs.empty()));
//
//		items.first.emplace_back(row);
//	}
//
//	{
//		auto row = new containers::row_t{};
//
//		auto config_name = controls::text_input(&import_config_name, LOCALIZE("config.id_input"));
//
//		c_style s{};
//		s.item_width = 176.f;
//
//		config_name->set_style(s);
//		s.container_padding = { 8.f, 0.f };
//		row->set_style(s);
//		row->add(config_name);
//		row->add(controls::button(LOCALIZE(("config.import")), import_config, vec2d{ 64.f, 32.f } * dpi::_get_actual_scale(), e_items_align::center, nullptr));
//
//		items.first.emplace_back(row);
//	}
//
//	items.second.emplace_back(configs_dyngroup);
//	menu->content->add(menu->create_content(items));
//}
//
//static void scripts_tab() {
//	active_scripts.clear();
//	if (!network::get_scripts(active_scripts))
//		return;
//
//	for (const auto& file: get_files_with_extension(STRS("weave\\lua"), STRS(".lua"))) {
//		const auto& filename = file.filename().u8string();
//		auto it = std::find_if(active_scripts.begin(), active_scripts.end(), [&](auto& script) { return script.m_name == std::string{ (char*)filename.c_str() }; });
//		if (it != active_scripts.end()) {
//			// mark it as not loaded so we can load it on next time when user will click load button
//			it->m_loaded_on_server = it->m_last_uploaded_hash == sha512::get(read_file_bin(STRS("weave\\lua\\") + it->m_name));
//			it->m_exist_on_server = true;
//			continue;
//		}
//
//		auto& script = active_scripts.emplace_back();
//		script.m_name = std::string{ (char*)filename.c_str() };
//		script.m_created_at = clock();
//		script.m_last_update_at = clock();
//
//		script.m_author = network::username;
//		script.m_last_update_by = network::username;
//		script.m_private = true;
//
//		// mark it as not loaded and not exist on server yet
//		script.m_exist_on_server = false;
//		script.m_loaded_on_server = false;
//	}
//
//	menu_t::content_items_t items{};
//	std::vector<std::string> scripts_list{};
//	scripts_list.reserve(active_scripts.size());
//	for (auto& script: active_scripts)
//		scripts_list.emplace_back(remove_extension(script.m_name, STRS(".lua")));
//
//	auto scripts_dyngroup = new containers::group_t{};
//	auto scripts_update = [=](bool updated) {
//		if (!updated)
//			return;
//
//		if (active_scripts.empty()) {
//			scripts_dyngroup->update_state([=]() {
//				c_style s{};
//				s.text_color = { 127, 127, 127 };
//				auto t = controls::text(STRS("Please, choose some script to see details"));
//				t->set_style(s);
//				scripts_dyngroup->add(t);
//			});
//
//			return;
//		}
//
//		active_script_index = std::clamp<size_t>(active_script_index, 0, active_scripts.size() - 1);
//		scripts_dyngroup->update_state([=]() {
//			auto& active_script = active_scripts[active_script_index];
//			scripts_dyngroup->add(controls::text_input(&active_script.m_name, STRS("Enter script name...")));
//			scripts_dyngroup->add(controls::prompt(STRS("Author: "), active_script.m_author));
//
//			if (active_script.m_author != active_script.m_last_update_by)
//				scripts_dyngroup->add(controls::prompt(STRS("Last update by: "), active_script.m_last_update_by));
//
//			scripts_dyngroup->add(controls::prompt(STRS("Updated: "), dformat(STRS("{} ago"), format_time_rel(time(0) - active_script.m_last_update_at))));
//
//			scripts_dyngroup->add(controls::prompt(STRS("Created: "), dformat(STRS("{} ago"), format_time_rel(time(0) - active_script.m_created_at))));
//
//			scripts_dyngroup->add(controls::checkbox(LOCALIZE("private"), &active_scripts[active_script_index].m_private));
//			scripts_dyngroup->add(script_actions());
//		});
//
//		//menu->window->lock();
//		//scripts_dyngroup->set_loading();
//
//		//std::thread([=]() {
//		//	active_script_index = std::clamp<size_t>(active_script_index, 0, active_scripts.size() - 1);
//		//	scripts_dyngroup->update_state([=]() {
//		//		auto& active_script = active_scripts[active_script_index];
//		//		scripts_dyngroup->add(controls::text_input(&active_script.m_name, STRS("Enter script name...")));
//		//		scripts_dyngroup->add(controls::prompt(STRS("Author: "), active_script.m_author));
//
//		//		if (active_script.m_author != active_script.m_last_update_by)
//		//			scripts_dyngroup->add(controls::prompt(STRS("Last update by: "), active_script.m_last_update_by));
//
//		//		scripts_dyngroup->add(controls::prompt(STRS("Updated: "), dformat(STRS("{} ago"), format_time_rel(time(0) - active_script.m_last_update_at))));
//
//		//		scripts_dyngroup->add(controls::prompt(STRS("Created: "), dformat(STRS("{} ago"), format_time_rel(time(0) - active_script.m_created_at))));
//
//		//		scripts_dyngroup->add(controls::checkbox(LOCALIZE("private"), &active_scripts[active_script_index].m_private));
//		//		scripts_dyngroup->add(script_actions());
//		//	});
//
//		//	menu->window->unlock();
//		//	scripts_dyngroup->stop_loading();
//		//}).detach();
//	};
//
//	scripts_update(true);
//
//	{
//		auto row = new containers::row_t{};
//		row->override_style().container_padding = { 4.f, 0.f };
//		//auto padding = new containers::group_t{ "", { 8.f, 1.f * dpi::_get_actual_scale() } };
//		//row->add(padding);
//
//		static float last_refresh_time{};
//
//		auto refresh_callback = []() {
//			const auto time = globals().get_time();
//			if (std::abs(last_refresh_time - time) < 2.f)
//				return;
//
//			last_refresh_time = time;
//			menu->recreate_gui();
//		};
//
//		auto refresh = controls::button(ICON("arrows-rotate"), refresh_callback, vec2d{ 16, 16 } * dpi::_get_actual_scale());
//		refresh->override_style().set_callback([](c_style& s) {
//			const auto lerp = std::min<float>(std::abs(last_refresh_time - globals().get_time()) / 2.f, 1.f);
//			s.text_color = color_t::lerp({ 60, 60, 60 }, { 200, 200, 200 }, lerp);
//			s.text_hovered = color_t::lerp({ 80, 80, 80 }, { 255, 255, 255 }, lerp);
//		});
//
//		refresh->override_style().tooltip = STRS("Refresh");
//		refresh->override_style().transparent_clickable = true;
//		refresh->override_style().invisible_clickable = true;
//
//		row->add(refresh);
//
//		auto open_folder = controls::button(
//				ICON("folder"),
//				[]() {
//					ShellExecuteA(0, 0, (fs::current_path() / fs::path(STRS("weave/lua"))).string().c_str(), 0, 0, SW_SHOWNORMAL);
//				},
//				vec2d{ 16, 16.f } * dpi::_get_actual_scale());
//
//		open_folder->override_style().tooltip = STRS("Open scripts location");
//		open_folder->override_style().transparent_clickable = true;
//		open_folder->override_style().invisible_clickable = true;
//
//		row->add(open_folder);
//
//		auto open_docs = controls::button(
//				ICON("book"), []() {
//					ShellExecuteA(0, 0, STRSC("https://docs.weave.su/"), 0, 0, SW_SHOWNORMAL);
//				},
//				vec2d{ 16, 16 } * dpi::_get_actual_scale());
//
//		open_docs->override_style().tooltip = STRS("Open documentation");
//		open_docs->override_style().transparent_clickable = true;
//		open_docs->override_style().invisible_clickable = true;
//
//		row->add(open_docs);
//		row->override_style().container_padding.y = 8;
//		items.first.emplace_back(row);
//	}
//
//	// padding
//	{
//		auto padding = new containers::group_t{ "", { 1.f, 8.f * dpi::_get_actual_scale() } };
//		items.first.emplace_back(padding);
//	}
//
//	if (scripts_list.empty()) {
//		auto t = controls::text(ICON("empty-set") + STRS(" There's no scripts at the moment"));
//		t->override_style().text_color = { 127, 127, 127 };
//		auto padding = new containers::group_t{ "", { 1.f, 8.f * dpi::_get_actual_scale() } };
//		items.first.emplace_back(padding);
//		items.first.emplace_back(t);
//	} else {
//		items.first.emplace_back(configs_selectables(&active_script_index, scripts_list, resources::get_texture(HASH("scripts_icon")), scripts_update));
//	}
//
//	auto import_script = []() {
//		if (active_scripts.size() >= 10) {
//			msgbox::add(LOCALIZE(("max_scripts_error")));
//			return;
//		}
//
//		if (import_script_name.empty()) {
//			msgbox::add(STRS("Enter the script id!"));
//			return;
//		}
//
//		menu->content->update_state([]() {
//			menu->content->set_loading();
//			menu->window->lock();
//			std::thread([]() {
//				if (!network::import_script(import_script_name))
//					msgbox::add(STRS("Can not import script!"));
//
//				import_script_name.clear();
//				__menu::on_tab_change();
//				menu->window->unlock();
//				menu->content->stop_loading();
//			}).detach();
//		});
//	};
//
//	{
//		auto row = new containers::row_t{};
//
//		auto script_name = controls::text_input(&import_script_name, STRS("Script id..."));
//
//		c_style s{};
//		s.item_width = 176.f;
//
//		script_name->set_style(s);
//		s.container_padding = { 8.f, 0.f };
//		row->set_style(s);
//		row->add(script_name);
//		row->add(controls::button(STRS("Import"), import_script, vec2d{ 64.f, 32.f } * dpi::_get_actual_scale(), e_items_align::center, nullptr));
//
//		items.first.emplace_back(row);
//	}
//
//	items.second.emplace_back(scripts_dyngroup);
//	menu->content->add(menu->create_content(items));
//}
//
//static void skins_tab() {
//	if (skin_changer->m_parsed) {
//		static int selected_weapon = 0;
//		static std::string weapon_search_phrase{};
//
//		menu->content->override_style().disable_scroll = true;
//		menu->content->override_style().scroll_fade = false;
//
//		auto grow = new containers::row_t{};
//		auto skins_holder = new containers::group_t{ "", vec2d{ 0.f, 372.f } * dpi::_get_actual_scale() };
//		skins_holder->override_style().container_padding = { 8.f, 12.f };
//		skins_holder->override_style().scroll_fade = true;
//		skins_holder->override_style().disable_scroll = false;
//
//		std::vector<std::string> weapon_icons;
//		for (const auto& [key, value]: sdk::weapon_names)
//			weapon_icons.emplace_back(dformat(STRS("{}\n{}"), value, (char*)misc->get_weapon_icon(key)));
//
//		auto update_skins = [skins_holder, weapon_icons](bool updated) {
//			if (!updated)
//				return;
//
//			skins_holder->update_state([skins_holder, weapon_icons]() {
//				auto new_line = [skins_holder]() {
//					auto row = new containers::row_t{};
//					skins_holder->add(row);
//					return row;
//				};
//
//				auto row = new_line();
//				int i = 0;
//
//				auto add_skin = [&](auto r) {
//					row->add(r);
//					if (i++ >= 2) {
//						row = new_line();
//						i = 0;
//					}
//				};
//
//				std::string active_name = weapon_icons[selected_weapon];
//				active_name = active_name.substr(0, active_name.find('\n'));
//
//				auto current_weapon = std::find_if(sdk::weapon_names.begin(), sdk::weapon_names.end(), [&](const auto& kv) {
//					return kv.second == active_name;
//				});
//
//				if (current_weapon != sdk::weapon_names.end()) {
//					auto items = skin_changer->m_paint_kits;
//					items.erase(std::remove_if(items.begin(), items.end(), [=](const auto& kit) { return kit.m_weapon_id != current_weapon->first; }), items.end());
//					std::sort(items.begin(), items.end(), [](const auto& k1, const auto& k2) { return k1.m_rarity > k2.m_rarity; });
//
//					for (auto& it: items)
//						add_skin(controls::skin_selector(
//								[it]() mutable -> void* {
//									auto buf = http::send(it.m_weapon_link, RESOURCES);
//									D3DXCreateTextureFromFileInMemory(render::device, buf.data(), buf.size(), &it.m_texture);
//									return it.m_texture;
//								},
//								[it, current_weapon]() {
//									settings->skins.items[current_weapon->first].fallback_paint_kit = it.m_id;
//								},
//								it.m_rarity));
//				}
//			});
//		};
//
//		update_skins(true);
//
//		auto weapon_selector = new containers::group_t{ "", vec2d{ 0.f, 372.f - 32.f } * dpi::_get_actual_scale() };
//		weapon_selector->add(controls::selectables(&selected_weapon, { weapon_icons }, false, dpi::scale(116.f), update_skins, &weapon_search_phrase));
//		weapon_selector->override_style().scroll_fade = true;
//		weapon_selector->override_style().disable_scroll = false;
//		weapon_selector->override_style().container_padding.x = 0.f;
//		weapon_selector->override_style().window_padding.y = 16.f;
//
//		auto sidebar = new containers::column_t{};
//
//		auto search_weapon = controls::text_input(&weapon_search_phrase, LOCALIZE("search"));
//		search_weapon->override_style().item_width = 116.f;
//		sidebar->add(search_weapon);
//		sidebar->add(weapon_selector);
//
//		grow->add(sidebar);
//		grow->add(skins_holder);
//		menu->content->add(grow);
//	} else {
//		menu->content->add(controls::text(STRS("Skins are parsing at the moment. Please come back later!")));
//	}
//}
//
//static void profile_tab() {
//	menu_t::content_items_t items{};
//
//	// welcome
//	{
//		auto welcome = controls::text(STRS("Welcome back!"), true);
//		welcome->override_style().container_padding.y = 16.f;
//		items.first.emplace_back(welcome);
//	}
//
//	// user info
//	{
//		auto user_info = new containers::group_t{ "", vec2d{ 0.f, 40.f } * dpi::_get_actual_scale() };
//		c_style style{};
//		style.container_padding = {};
//		style.disable_scroll = true;
//
//		user_info->set_style(style);
//		auto row = new containers::row_t{};
//
//		row->add(controls::web_image(
//				[]() {
//					return http::send(dformat(STRS("v0/social/profile/avatar/{}"), network::user_id), DEV);
//				},
//				1.f, vec2d{ 40.f, 40.f } * dpi::_get_actual_scale(), true));
//
//		row->set_style({});
//
//		auto column = new containers::column_t{ e_items_align::center };
//		column->add(new controls::text_t{ network::username, true });
//
//		c_style sub_style{};
//		sub_style.text_color = { 127, 127, 127 };
//		sub_style.small_text = true;
//
//		auto sub = new controls::text_t{ dformat(STRS("expires in: {}"), format_time_rel(network::expire_date - time(0))) };
//		sub->set_style(sub_style);
//
//		column->add(sub);
//
//		row->add(column);
//		user_info->add(row);
//		items.first.emplace_back(user_info);
//	}
//
//	// padding
//	{
//		auto padding = new containers::group_t{ "", { 1.f, 16.f * dpi::_get_actual_scale() } };
//		items.first.emplace_back(padding);
//	}
//
//	network::online = network::get_online();
//	const auto& session = network::get_session();
//	const auto& updates = json_t::parse(http::send(STRS("updates.json"), RESOURCES));
//
//	if (network::online.has_value() && session.has_value()) {
//		auto group = new containers::group_t{};
//
//		group->add(controls::prompt(STRS("Build date: "), dformat(STRS("{} {}"), STRC(__DATE__), STRC(__TIME__))));
//
//		if (network::online->m_total > 1)
//			group->add(controls::prompt(STRS("Users online: "), dformat(STRS("{}"), network::online->m_total - 1)));
//
//		if (session->m_total_time > 60)
//			group->add(controls::prompt(STRS("Total time played: "), dformat(STRS("{}"), format_time_rel(session->m_total_time))));
//
//		if (session->m_session_time > 60)
//			group->add(controls::prompt(STRS("Current session: "), dformat(STRS("{}"), format_time_rel(session->m_session_time))));
//
//		const std::vector<std::string> scales = { STRS("100%"), STRS("125%"), STRS("150%"), STRS("175%"), STRS("200%") };
//		group->add(controls::dropbox(STRS("DPI Scale"), &settings->dpi_scale, scales, false, update_dpi_scale));
//
//		items.first.emplace_back(group);
//	}
//
//	if (network::online.has_value()) {
//		if (network::online->m_total > 1) {
//			auto group = new containers::group_t{ STRS("Users online: ") };
//
//			for (auto& user: network::online->m_users) {
//				if (user.m_id == network::user_id)
//					continue;
//
//				auto user_info = new containers::group_t{ "", vec2d{ 0.f, 40.f } * dpi::_get_actual_scale() };
//				c_style style{};
//				style.container_padding = {};
//				style.disable_scroll = true;
//
//				user_info->set_style(style);
//				auto row = new containers::row_t{};
//				row->add(controls::web_image(
//						[&user]() {
//							return http::send(dformat(STRS("v0/social/profile/avatar/{}"), user.m_id), DEV);
//						},
//						1.f, vec2d{ 40.f, 40.f } * dpi::_get_actual_scale(), true));
//
//				row->override_style();
//
//				auto column = new containers::column_t{ e_items_align::center };
//				column->add(new controls::text_t{ user.m_name, true });
//
//				c_style sub_style{};
//				sub_style.text_color = { 127, 127, 127 };
//				sub_style.small_text = true;
//
//				auto sub = new controls::text_t{ user.m_activity };
//				sub->set_style(sub_style);
//
//				column->add(sub);
//
//				row->add(column);
//				user_info->add(row);
//
//				{
//					auto padding = new containers::group_t{ "", { 1.f, 16.f * dpi::_get_actual_scale() } };
//					group->add(padding);
//				}
//
//				group->add(user_info);
//			}
//
//			items.first.emplace_back(group);
//
//			if ((size_t)network::online->m_total > network::online->m_users.size()) {
//				items.first.emplace_back(controls::text(dformat(STRS("And {} more..."), network::online->m_total - network::online->m_users.size())));
//			}
//		}
//	}
//
//	{
//		for (const auto& update: updates[STRS("updates")]) {
//			auto group = new containers::group_t{};
//
//			{
//				auto title = controls::text(ICON("hashtag") + " " + update[STRS("title")].get<std::string>(), true);
//				title->override_style().text_color = { 127, 127, 127 };
//				title->override_style().container_padding.y = 16.f;
//				group->add(title);
//			}
//
//			const auto& update_list = update[STRS("list")];
//			{
//				for (const auto& log: update_list)
//					group->add(controls::text(ICON("arrow-right") + " " + log.get<std::string>()));
//			}
//
//			const auto& media = update[STRS("media")];
//
//			if (media.size() > 0) {
//				group->add(new containers::group_t{ "", { 1.f, 8.f } });
//
//				for (const auto& m: media) {
//					group->add(controls::web_image(
//							[&m]() {
//								return http::send(m.get<std::string>(), RESOURCES);
//							},
//							std::lerp(0.5f, 1.f, dpi::_get_actual_scale() - 1.f)));
//
//					group->add(new containers::group_t{ "", { 1.f, 8.f } });
//				}
//			}
//
//			{
//				auto padding = new containers::group_t{ "", { 1.f, 16.f * dpi::_get_actual_scale() } };
//				items.second.emplace_back(padding);
//			}
//
//			items.second.emplace_back(group);
//		}
//	}
//
//	menu->content->add(menu->create_content(items));
//}
//
//static void clone_items(std::vector<i_renderable*>& dst, const std::vector<i_renderable*>& src) {
//	for (auto item: src)
//		dst.emplace_back(item->clone());
//}
//
//static void tab_callback() {
//	if (menu->m_in_search) {
//		menu->set_tab_attributes(STRS("Search"), resources::get_texture(HASH("search_icon")));
//		std::vector<uint8_t> bytes{};
//		if (network::search(bytes, search_phrase)) {
//			menu->content->add(menu->create_content(menu->parse_items(bytes)));
//		}
//	} else {
//		if (menu->m_active_tab > XOR32S(max_tabs)) {
//			const auto& tab = menu->m_lua_tabs[menu->m_active_tab - max_tabs - 1].second;
//			menu->set_tab_attributes(tab.m_name, tab.m_icon);
//
//			menu_t::content_items_t items_copy{};
//			clone_items(items_copy.first, tab.m_items.first);
//			clone_items(items_copy.second, tab.m_items.second);
//
//			menu->content->add(menu->create_content(items_copy));
//		} else {
//			switch (menu->m_active_tab) {
//				case -1: menu->set_tab_attributes(LOCALIZE(("tab.profile")), resources::get_texture(HASH("profile"))); break;
//				case 0: menu->set_tab_attributes(LOCALIZE(("tab.ragebot")), resources::get_texture(HASH("ragebot_icon")), LOCALIZE(("tab.ragebot.aimbot"))); break;
//				case 1: menu->set_tab_attributes(LOCALIZE(("tab.ragebot")), resources::get_texture(HASH("ragebot_icon")), LOCALIZE(("tab.ragebot.antiaim"))); break;
//				case 2: menu->set_tab_attributes(LOCALIZE(("tab.ragebot")), resources::get_texture(HASH("ragebot_icon")), LOCALIZE(("tab.ragebot.misc"))); break;
//				case 3: menu->set_tab_attributes(LOCALIZE(("tab.visuals")), resources::get_texture(HASH("visuals_icon")), LOCALIZE(("tab.visuals.players"))); break;
//				case 4: menu->set_tab_attributes(LOCALIZE(("tab.visuals")), resources::get_texture(HASH("visuals_icon")), LOCALIZE(("tab.visuals.local"))); break;
//				case 5: menu->set_tab_attributes(LOCALIZE(("tab.visuals")), resources::get_texture(HASH("visuals_icon")), LOCALIZE(("tab.visuals.world"))); break;
//				case 6: menu->set_tab_attributes(LOCALIZE(("tab.visuals")), resources::get_texture(HASH("visuals_icon")), LOCALIZE(("tab.visuals.other"))); break;
//				case 7: menu->set_tab_attributes(LOCALIZE(("tab.misc")), resources::get_texture(HASH("misc_icon"))); break;
//				case 8: menu->set_tab_attributes(LOCALIZE(("tab.skins")), resources::get_texture(HASH("skins_icon"))); break;
//				case 9: menu->set_tab_attributes(LOCALIZE(("tab.scripts")), resources::get_texture(HASH("scripts_icon"))); break;
//				case 10: menu->set_tab_attributes(LOCALIZE(("tab.settings")), resources::get_texture(HASH("settings_icon"))); break;
//				case 11: menu->set_tab_attributes(LOCALIZE(("tab.hotkeys")), resources::get_texture(HASH("hotkeys_icon"))); break;
//			}
//
//			if (menu->m_active_tab == XOR32S(-1)) {
//				profile_tab();
//			} else if (menu->m_active_tab == XOR32S(11))
//				hotkeys_tab();
//			else if (menu->m_active_tab == XOR32S(10))
//				settings_tab();
//			else if (menu->m_active_tab == XOR32S(9)) {
//				scripts_tab();
//				//menu->content->add(controls::text(ICON("triangle-exclamation") + STRS("  Under construction")));
//			}
//			//else if (menu->m_active_tab == XOR32S(8))
//			//	skins_tab();
//			else {
//				//std::vector<uint8_t> response{};
//				//if (network::get(response, menu->m_active_tab))
//				//	menu->content->add(menu->create_content(menu->parse_items(response)));
//			}
//		}
//	}
//}
//
//void __menu::on_tab_change() {
//	static std::atomic_bool balls;
//
//	if (balls == true)
//		return;
//
//	balls = true;
//
//	//printf("%d\n", menu.m_active_tab);
//
//	menu->content->override_style().disable_scroll = false;
//	menu->content->override_style().scroll_fade = true;
//
//	menu->window->lock();
//
//	menu->content->clear();
//	menu->content->set_loading();
//
//	std::thread([]() {
//		tab_callback();
//
//		menu->window->unlock();
//		menu->content->stop_loading();
//		balls = false;
//	}).detach();
//}
//
//void __menu::create_header() {
//	auto column = new containers::column_t{};
//	auto row = new containers::row_t{};
//
//	if (menu->m_active_tab_icon != nullptr)
//		row->add(controls::image(menu->m_active_tab_icon, vec2d{ 12.f, 12.f } * dpi::_get_actual_scale()));
//
//	auto tab = controls::text(menu->m_active_tab_name);
//	c_style style{};
//	style.small_text = true;
//	style.text_color = { 127, 127, 127 };
//	style.container_padding.x = -4.f;
//	style.window_padding.y = 0.f;
//
//	tab->set_style(style);
//
//	row->add(tab);
//
//	if (!menu->m_active_subtab_name.empty()) {
//		auto arrow = controls::image(resources::get_texture(HASH("arrow")), vec2d{ 12.f, 12.f } * dpi::_get_actual_scale());
//		arrow->set_style(style);
//		row->add(arrow);
//
//		auto subtab = controls::text(menu->m_active_subtab_name);
//		subtab->set_style(style);
//		row->add(subtab);
//	}
//
//	row->add(controls::loading([]() { return menu->window->is_locked(); }));
//
//	column->add(row);
//
//	auto search = controls::text_input(&search_phrase, STRS("\xef\x80\x82   ") + LOCALIZE("search"));
//
//	search->on_change_value([]() {
//		menu->m_in_search = !search_phrase.empty();
//		//menu->change_tab(menu->m_active_tab);
//	});
//
//	search->on_change_state([](bool active) {
//		if (!active) {
//			menu->m_in_search = !search_phrase.empty();
//			menu->change_tab(menu->m_active_tab, true);
//		}
//	});
//
//	c_style search_style{};
//	search_style.item_width = 532.f;
//	search->set_style(search_style);
//
//	column->add(search);
//
//	menu->header->add(column);
//}
//
//menu_t::content_items_t menu_t::parse_items(const std::vector<uint8_t>& bytes) {
//	return unpacker.parse(bytes);
//}
//
//i_renderable* menu_t::create_content(const content_items_t& items) {
//	auto content_col1 = new containers::group_t{ "", vec2d{ 248.f + 16.f, 0.f } * dpi::_get_actual_scale() };
//	auto content_col2 = new containers::group_t{ "", vec2d{ 248.f + 16.f, 0.f } * dpi::_get_actual_scale() };
//	auto content_row = new containers::row_t{};
//
//	c_style group_style{};
//	group_style.container_padding.x = 0.f;
//	content_col1->set_style(group_style);
//	group_style.container_padding.x = 4.f;
//	content_col2->set_style(group_style);
//
//	for (auto item: items.first)
//		content_col1->add(item);
//
//	for (auto item: items.second)
//		content_col2->add(item);
//
//	content_row->add(content_col1);
//	content_row->add(content_col2);
//
//	return content_row;
//}
//
//static int wsel_group_id{}, wsel_weapon_id{};
//
//static void create_weapon_selector(containers::group_t* weapons) {
//	auto dynamic_items = new containers::group_t{};
//
//	c_style s{};
//	s.container_padding = { 4.f, 0.f };
//	s.window_padding = {};
//	weapons->set_style(s);
//
//	auto update_state = [=]() {
//		dynamic_items->update_state([=]() {
//			auto& group = settings->ragebot.weapons[wsel_group_id];
//			auto& weapon = group.settings[wsel_weapon_id];
//			const auto overrided = wsel_group_id == 0 || (wsel_weapon_id == 0 && group.override_default) || weapon.override_default;
//
//			if (!overrided)
//				return;
//
//			menu->window->lock();
//			dynamic_items->set_loading();
//
//			std::thread([=]() {
//				std::vector<uint8_t> bytes{};
//				if (network::get(bytes, XOR32S(12) /* rage_weapon tab */, wsel_group_id | (wsel_weapon_id << XOR32S(16)))) {
//					for (auto item: menu->parse_items(bytes).first)
//						dynamic_items->add(item);
//				}
//
//				menu->window->unlock();
//				dynamic_items->stop_loading();
//			}).detach();
//		});
//	};
//
//	auto& group = settings->ragebot.weapons[wsel_group_id];
//	auto& weapon = group.settings[wsel_weapon_id];
//
//	if (wsel_weapon_id == 0 && wsel_group_id != 0)
//		weapons->add(menu->create_group("", { controls::checkbox(LOCALIZE(("ragebot.override_group")), &group.override_default, update_state) }));
//	else if (wsel_weapon_id > 0 && wsel_group_id != 0)
//		weapons->add(menu->create_group("", { controls::checkbox(LOCALIZE(("ragebot.override_weapon")), &weapon.override_default, update_state) }));
//
//	update_state();
//	weapons->add(dynamic_items);
//}
//
//static int trigger_id{};
//static int chams_id{};
//
//static void create_antiaim_selector(containers::group_t* triggers) {
//	auto dynamic_items = new containers::group_t{};
//
//	c_style s{};
//	s.container_padding = { 4.f, 0.f };
//	s.window_padding = {};
//
//	triggers->set_style(s);
//
//	auto update_state = [=]() {
//		dynamic_items->update_state([=]() {
//			auto& trigger = settings->antiaim.triggers[trigger_id];
//			const auto overrided = trigger.override_default || trigger_id == 0;
//			if (!overrided)
//				return;
//
//			menu->window->lock();
//			dynamic_items->set_loading();
//
//			std::thread([=]() {
//				std::vector<uint8_t> bytes{};
//				if (network::get(bytes, XOR32S(13) /* antiaim_trigger tab */, trigger_id)) {
//					for (auto item: menu->parse_items(bytes).first)
//						dynamic_items->add(item);
//				}
//
//				menu->window->unlock();
//				dynamic_items->stop_loading();
//			}).detach();
//		});
//	};
//
//	if (trigger_id > 0)
//		triggers->add(menu->create_group("", { controls::checkbox(LOCALIZE(("override_default")), &settings->antiaim.triggers[trigger_id].override_default, update_state) }));
//
//	update_state();
//	triggers->add(dynamic_items);
//}
//
//static void create_chams_selector(containers::group_t* triggers) {
//	auto dynamic_items = new containers::group_t{};
//
//	c_style s{};
//	s.container_padding = { 4.f, 0.f };
//	s.window_padding = {};
//
//	triggers->set_style(s);
//
//	auto update_state = [=]() {
//		dynamic_items->update_state([=]() {
//			auto& trigger = settings->player_esp.chams[chams_id];
//			const auto overrided = trigger.enable;
//			if (!overrided)
//				return;
//
//			menu->window->lock();
//			dynamic_items->set_loading();
//
//			std::thread([=]() {
//				std::vector<uint8_t> bytes{};
//				if (network::get(bytes, XOR32S(14) /* antiaim_trigger tab */, chams_id)) {
//					if (chams_id == (int)incheat_vars::e_chams::chams_shot)
//						dynamic_items->add(controls::checkbox(LOCALIZE(("player_esp.chams.last_shot_only")), &settings->player_esp.shot_chams_last_only));
//
//					for (auto item: menu->parse_items(bytes).first)
//						dynamic_items->add(item);
//				}
//
//				menu->window->unlock();
//				dynamic_items->stop_loading();
//			}).detach();
//		});
//	};
//
//	triggers->add(menu->create_group("", { controls::checkbox(LOCALIZE("player_esp.chams.enable"), &settings->player_esp.chams[chams_id].enable, update_state) }));
//
//	update_state();
//	triggers->add(dynamic_items);
//}
//
//static auto esp_settings_impl(color_t* color0, color_t* color1, int* position, int* font = nullptr, std::vector<std::string>* items = nullptr) {
//	auto popup = create_popup({}, container_flag_is_popup | popup_flag_animation | container_flag_visible);
//
//	c_style s{};
//	s.clickable_color = { 24, 24, 24 };
//	s.clickable_hovered = { 28, 28, 28 };
//	popup->set_style(s);
//
//	if (color0 == &settings->player_esp.health.colors[0])
//		popup->add(controls::checkbox(LOCALIZE("esp.override_color"), &settings->player_esp.override_health_color));
//
//	popup->add(controls::colorpicker(LOCALIZE("esp.color[0]"), color0, false));
//	popup->add(controls::colorpicker(LOCALIZE("esp.color[1]"), color1, false));
//	//popup->add(controls::dropbox(STRS("Position"), position, { STRS("Bottom"), STRS("Top"), STRS("Left"), STRS("Right") }, false));
//
//	if (font != nullptr)
//		popup->add(controls::dropbox(LOCALIZE("esp.font"), font,
//									 {
//											 STRS("Default"),
//											 STRS("Bold"),
//											 STRS("Small pixel"),
//											 STRS("Menu main"),
//											 STRS("Menu bold"),
//											 STRS("Menu Big"),
//											 STRS("Small"),
//											 STRS("Small SemiBold"),
//											 STRS("Small Bold"),
//									 },
//									 false));
//}
//
//static auto recursive_item_impl(const menu_t::content_items_t& items) {
//	auto popup = create_popup({}, container_flag_is_popup | popup_flag_animation | container_flag_visible);
//
//	c_style s{};
//	s.clickable_color = { 24, 24, 24 };
//	s.clickable_hovered = { 28, 28, 28 };
//	popup->set_style(s);
//	for (auto item: items.first)
//		popup->add(item);
//}
//
//static auto recursive_item(i_renderable* label_item, const std::vector<uint8_t>& buf) {
//	auto row = new containers::row_t{};
//	row->override_style().container_padding.x = 12.f;
//	row->override_style().item_width -= 28.f;
//
//	row->add(label_item);
//
//	auto btn = controls::button(
//			"",
//			[&buf]() {
//				recursive_item_impl(menu->parse_items(buf));
//			},
//			vec2d{ 16.f, 32.f } * dpi::_get_actual_scale(),
//			e_items_align::center,
//			resources::get_texture(HASH("settings_icon")));
//
//	btn->override_style().transparent_clickable = true;
//	btn->override_style().invisible_clickable = true;
//	btn->override_style().text_color = { 60, 60, 60 };
//	btn->override_style().text_hovered = { 100, 100, 100 };
//
//	row->add(btn);
//	return row;
//}
//
//static auto create_esp_settings(const std::string& label, bool* value, color_t* color0, color_t* color1, int* position, int* font = nullptr, std::vector<std::string>* items = nullptr) {
//	auto row = new containers::row_t{};
//	row->override_style().container_padding.x = 12.f;
//	row->override_style().item_width -= 28.f;
//
//	if (items != nullptr)
//		row->add(controls::dropbox(label, (int*)value, *items, true));
//	else
//		row->add(controls::checkbox(label, value));
//
//	auto btn = controls::button(
//			"",
//			[=]() {
//				esp_settings_impl(color0, color1, position, font, items);
//			},
//			vec2d{ 16.f, items != nullptr ? 32.f : 20.f } * dpi::_get_actual_scale(),
//			e_items_align::center,
//			resources::get_texture(HASH("settings_icon")));
//
//	btn->override_style().transparent_clickable = true;
//	btn->override_style().invisible_clickable = true;
//	btn->override_style().text_color = { 60, 60, 60 };
//	btn->override_style().text_hovered = { 100, 100, 100 };
//
//	row->add(btn);
//
//	return row;
//}
//
//#define ITEM_ID(id) (XOR32S(MSVC_CONSTEXPR(xs32_random(menu_t::hash, id))))
//
//std::pair<i_renderable*, uint32_t> menu_t::c_unpacker::parse_item(byte_t*& ptr) {
//	auto item_type = read_int<uint32_t>(ptr);
//
//	if (item_type == ITEM_ID(dropdown_id) || item_type == ITEM_ID(multidropdown_id)) {
//		auto label = read_string(ptr);
//		auto items = read_string_array(ptr);
//		auto value = read_int<uint32_t>(ptr);
//
//		return { controls::dropbox(label, (int*)value, items, item_type == ITEM_ID(multidropdown_id)), item_type };
//	} else if (item_type == ITEM_ID(checkbox_id)) {
//		auto label = read_string(ptr);
//		auto value = read_int<uint32_t>(ptr);
//		return { controls::checkbox(label, (bool*)value), item_type };
//	} else if (item_type == ITEM_ID(slider_id)) {
//		auto label = read_string(ptr);
//		auto min = read_int<int16_t>(ptr);
//		auto max = read_int<int16_t>(ptr);
//		auto format_type = read_int<int8_t>(ptr);
//		auto value = read_int<uint32_t>(ptr);
//
//		return { controls::slider(label, (int*)value, min, max, get_slider_format(format_type)), item_type };
//	} else if (item_type == ITEM_ID(colorpicker3_id) || item_type == ITEM_ID(colorpicker4_id)) {
//		auto label = read_string(ptr);
//		auto value = read_int<uint32_t>(ptr);
//
//		return { controls::colorpicker(label, (color_t*)value, item_type == ITEM_ID(colorpicker4_id)), item_type };
//	} else if (item_type == ITEM_ID(weapon_selector)) {
//		auto col = new containers::column_t{};
//		auto weapons = new containers::group_t{ "" };
//
//		auto update_state = [=]() {
//			weapons->update_state([=]() {
//				create_weapon_selector(weapons);
//			});
//		};
//
//		update_state();
//		auto selector = menu->create_group("", { controls::weapon_selector(&wsel_group_id, &wsel_weapon_id, update_state) });
//
//		c_style s{};
//		//s.container_padding = { 8.f, 0.f };
//		s.window_padding = {};
//		selector->set_style(s);
//
//		col->add(selector);
//		col->add(weapons);
//
//		return { col, item_type };
//	} else if (item_type == ITEM_ID(antiaim_selector)) {
//		auto col = new containers::column_t{};
//		auto triggers = new containers::group_t{ "" };
//
//		auto update_state = [=](bool updated) {
//			if (!updated)
//				return;
//
//			triggers->update_state([=]() {
//				create_antiaim_selector(triggers);
//			});
//		};
//
//		update_state(true);
//
//		auto selector = menu->create_group("", {
//													   controls::dropbox(LOCALIZE("antiaim.trigger.setting_for"), &trigger_id, { LOCALIZE("antiaim.trigger.default"), LOCALIZE("antiaim.trigger.standing"), LOCALIZE("antiaim.trigger.moving"), LOCALIZE("antiaim.trigger.jumping"), LOCALIZE("antiaim.trigger.slow_walking"), LOCALIZE("antiaim.trigger.crouching"), LOCALIZE("antiaim.trigger.defensive") }, false, update_state),
//											   });
//
//		c_style s{};
//		s.container_padding = { 8.f, 0.f };
//		s.window_padding = {};
//		selector->set_style(s);
//
//		col->add(selector);
//		col->add(triggers);
//
//		return { col, item_type };
//	} else if (item_type == ITEM_ID(chams_selector)) {
//		auto col = new containers::column_t{};
//		auto triggers = new containers::group_t{ "" };
//
//		auto update_state = [=](bool updated) {
//			if (!updated)
//				return;
//
//			triggers->update_state([=]() {
//				create_chams_selector(triggers);
//			});
//		};
//
//		update_state(true);
//
//		auto selector = menu->create_group("", { controls::dropbox(STRS("Setting for"), &chams_id,
//																   {
//																		   STRS("Enemy Visible"),
//																		   STRS("Enemy XQZ"),
//																		   STRS("Local player"),
//																		   STRS("Desync model"),
//																		   STRS("Enemy Backtrack"),
//																		   STRS("Shot"),
//																		   STRS("Viewmodel arms"),
//																		   STRS("Viewmodel weapon"),
//																		   STRS("Local attachments"),
//																   },
//																   false, update_state) });
//
//		c_style s{};
//		s.container_padding = { 8.f, 0.f };
//		s.window_padding = {};
//		selector->set_style(s);
//
//		col->add(selector);
//		col->add(triggers);
//
//		return { col, item_type };
//	} else if (item_type == ITEM_ID(group_begin)) {
//		auto group_name = read_string(ptr);
//		return { new containers::group_t{ group_name }, item_type };
//	} else if (item_type == ITEM_ID(text)) {
//		auto text = read_string(ptr);
//		return { controls::text(text), item_type };
//	} else if (item_type == ITEM_ID(esp_bar_settings)) {
//		auto label = read_string(ptr);
//		auto value = read_int<uint32_t>(ptr);
//		auto color0 = read_int<uint32_t>(ptr);
//		auto color1 = read_int<uint32_t>(ptr);
//		auto position = read_int<uint32_t>(ptr);
//
//		return { create_esp_settings(label, (bool*)value, (color_t*)color0, (color_t*)color1, (int*)position), item_type };
//	} else if (item_type == ITEM_ID(esp_text_settings)) {
//		auto label = read_string(ptr);
//		auto value = read_int<uint32_t>(ptr);
//		auto color0 = read_int<uint32_t>(ptr);
//		auto color1 = read_int<uint32_t>(ptr);
//		auto position = read_int<uint32_t>(ptr);
//		auto font = read_int<uint32_t>(ptr);
//
//		return { create_esp_settings(label, (bool*)value, (color_t*)color0, (color_t*)color1, (int*)position, (int*)font), item_type };
//	} else if (item_type == ITEM_ID(esp_text_array_settings)) {
//		auto label = read_string(ptr);
//		auto value = read_int<uint32_t>(ptr);
//		auto color0 = read_int<uint32_t>(ptr);
//		auto color1 = read_int<uint32_t>(ptr);
//		auto position = read_int<uint32_t>(ptr);
//		auto font = read_int<uint32_t>(ptr);
//		auto items = read_string_array(ptr);
//
//		return { create_esp_settings(label, (bool*)value, (color_t*)color0, (color_t*)color1, (int*)position, (int*)font, &items), item_type };
//	} else if (item_type == ITEM_ID(dpi_scale_id)) {
//		const std::vector<std::string> scales = { STRS("100%"), STRS("125%"), STRS("150%"), STRS("175%"), STRS("200%") };
//		return { controls::dropbox(STRS("DPI Scale"), (int*)&settings->dpi_scale, scales, false, update_dpi_scale), item_type };
//	} else if (item_type == ITEM_ID(type_bool)) {
//		auto pointer = read_int<uint32_t>(ptr);
//		auto value = read_int<uint8_t>(ptr);
//
//		*(incheat_vars::types::bool_t*)pointer = value;
//	} else if (item_type == ITEM_ID(type_int)) {
//		auto pointer = read_int<uint32_t>(ptr);
//		auto value = read_int<uint32_t>(ptr);
//
//		*(incheat_vars::types::int_t*)pointer = value;
//	} else if (item_type == ITEM_ID(type_flag)) {
//		auto pointer = read_int<uint32_t>(ptr);
//		auto value = read_int<uint32_t>(ptr);
//
//		*(incheat_vars::types::flags_t*)pointer = value;
//	} else if (item_type == ITEM_ID(type_color)) {
//		auto pointer = read_int<uint32_t>(ptr);
//		auto value = read_int<uint32_t>(ptr);
//
//		*(incheat_vars::types::color_t*)pointer = value;
//	}
//
//	return { nullptr, item_type };
//}
//
//menu_t::content_items_t menu_t::c_unpacker::parse(const std::vector<byte_t>& bytes) {
//	if (bytes.empty())
//		return {};
//
//	// printf("PARSING BEGIN\n");
//
//	content_items_t result{};
//	containers::group_t* current_group = nullptr;
//	int col = 0;
//
//	const auto push_item = [&](i_renderable* r) {
//		return col == 0 ? result.first.emplace_back(r) : result.second.emplace_back(r);
//	};
//
//	i_renderable* previous_item = nullptr;
//
//	auto ptr = (byte_t*)&bytes[0];
//	while ((uintptr_t)ptr - (uintptr_t)&bytes[0] < bytes.size()) {
//		const auto& item = parse_item(ptr);
//		if (item.first != nullptr) {
//			if (item.second == ITEM_ID(group_begin)) {
//				current_group = (containers::group_t*)item.first;
//				push_item(current_group);
//			} else {
//				if (current_group != nullptr)
//					current_group->add(item.first);
//				else
//					push_item(item.first);
//			}
//		} else if (item.second == ITEM_ID(group_end)) {
//			current_group = nullptr;
//		} else if (item.second == ITEM_ID(next_column)) {
//			++col;
//		} else if (item.second == ITEM_ID(type_bool)) {
//			__asm { nop }
//		} else if (item.second == ITEM_ID(type_int)) {
//			__asm { nop }
//		} else if (item.second == ITEM_ID(type_flag)) {
//			__asm { nop }
//		} else if (item.second == ITEM_ID(type_color)) {
//			__asm { nop }
//		} else if (item.second == ITEM_ID(tooltip_id)) {
//			if (previous_item != nullptr && previous_item->get_item_type() == e_item_type::control)
//				previous_item->override_style().tooltip = read_string(ptr);
//		} else if (item.second == ITEM_ID(recursive_id)) {
//			const auto& label_item = parse_item(ptr);
//			if (label_item.first == nullptr)
//				goto ERR;
//
//			const auto& buf = read_vector(ptr);
//			push_item(recursive_item(label_item.first, buf));
//		} else {
//		ERR:
//#ifdef _DEBUG
//			printf("unknown item type: %X\n", item.second);
//#endif
//			cheat_logs->add_error(STRS("A server error has occurred! If this happens again, contact support."));
//			return result;
//		}
//
//		if (item.second != ITEM_ID(recursive_id))
//			previous_item = item.first;
//	}
//
//	return result;
//}