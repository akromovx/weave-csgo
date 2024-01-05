/*

	in this file i commented all stuff related to interracting with menu
	if you're interested in making the real weave menu you can uncomment it and fix all errors
	that's not as hard as fixing the menu :)

*/

#include "api.hpp"
#include "script.hpp"

#include "../../deps/fontawesome/include.hpp"

#include "../globals.hpp"
#include "../interfaces.hpp"

#include "../utils/hotkeys.hpp"

#include "../features/visuals/logs.hpp"

#include "../features/hvh/hvh.hpp"
#include "../features/hvh/exploits.hpp"

#include "../game/override_entity_list.hpp"

#define LUA_FN(name) static int name(lua_State* l)
#define LUA_GETTER(name, from) \
	LUA_FN(name) {             \
		LUA_STATE(l);          \
		s.push(from);          \
		return 1;              \
	}

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

namespace lua {
	template<typename Arg>
	static constexpr bool opt_has_value(Arg&& opt) {
		return opt.has_value();
	}

	template<typename Arg, typename... Args>
	static constexpr bool opt_has_value(Arg&& first, Args&&... rest) {
		return first.has_value() && opt_has_value(std::forward<Args>(rest)...);
	}

	static std::string get_unique_name(uintptr_t script_id, uintptr_t hash) {
		return std::format("{:X}{:X}", script_id, hash);
	}

#define _UNIQUE_NAME(base) get_unique_name(s.get_id(), HASH(base))

	void init() {
		// ...
	}

	static std::unordered_map<std::string, std::unique_ptr<script_t>> loaded_scripts{};

	static void unload_script(std::unique_ptr<script_t>& script) {
		script->callback(STRS("unload"), {});
		auto script_id = script->get_state().get_id();

		// erase script tabs
		{
			/*auto& tabs = menu->m_lua_tabs;
			tabs.erase(std::remove_if(tabs.begin(), tabs.end(), [script_id](const auto& kv) { return kv.first == script_id; }), tabs.end());*/
		}

		// erase script storage
		{
			scripts_storage.erase(script_id);
		}
	}

	void load_script(const std::string& name, const std::string& id, const std::string& encoded) {
		if (id.empty()) {
			lua::perform_error(STRS("script id is empty"));
			return;
		}

		if (loaded_scripts.contains(id)) {
			lua::perform_error(STRS("script is already loaded"));
			return;
		}

		auto script = std::make_unique<script_t>();
		scripts_storage[script->get_state().get_id()].m_name = name;
		const auto loaded = script->load(id, encoded);

		if (loaded)
			loaded_scripts.insert({ id, std::move(script) });
		else
			unload_script(script);
	}

	bool is_loaded(const std::string& name) {
		return loaded_scripts.contains(name);
	}

	void unload_script(const std::string& name) {
		while (render::in_fsn) std::this_thread::yield();

		if (!loaded_scripts.contains(name)) {
			perform_error(STRS("script doesn't loaded"));
			return;
		}

		unload_script(loaded_scripts.at(name));
		loaded_scripts.erase(name);
	}

	std::optional<std::string> get_script_id(lua_State* ptr) {
		auto it = std::find_if(loaded_scripts.begin(), loaded_scripts.end(), [ptr](const auto& kv) {
			return ptr == *kv.second->get_state();
		});

		if (it != loaded_scripts.end())
			return it->first;

		return std::nullopt;
	}

	void callback(const std::string& name, const table_t& args, std::function<void(state_t&)> callback) {
		static std::mutex mutex{};
		THREAD_SAFE(mutex);

		for (auto& [id, script]: loaded_scripts)
			script->callback(name, args, callback);
	}

	// namespace lua
	namespace api {
		namespace ui {
			enum e_item_type {
				item_unknown = 0,
				item_tab,
				item_container,
				item_checkbox,
				item_slider,
				item_dropbox,
				item_multidropbox,
				item_colorpicker,
				item_text,
			};

			/*static auto create_popup(vec2d size, int flags) {
				auto popup = menu->window->create_popup();
				popup->m_flags.add(flags);
				popup->set_position(gui::globals().mouse_position - menu->window->get_position());
				popup->set_size(size * gui::dpi::_get_actual_scale());
				return popup;
			}

			static auto nested_item_impl(gui::containers::column_t* column) {
				auto popup = create_popup({}, gui::container_flag_is_popup | gui::popup_flag_animation | gui::container_flag_visible);
				popup->override_style().clickable_color = { 24, 24, 24 };
				popup->override_style().clickable_hovered = { 28, 28, 28 };

				for (auto item: column->get_items())
					popup->add(item->clone());
			}*/

			static auto nested_item(state_t& s, gui::i_renderable* label_item, gui::containers::column_t* column) {
				auto row = new gui::containers::row_t{};

				row->override_style().container_padding.x = 12.f;
				row->override_style().item_width -= 28.f;

				row->add(label_item);

				const auto low_height_item = label_item->is_low_height();

				auto btn = gui::controls::button(
						"",
						[column]() {
							//nested_item_impl(column);
						},
						vec2d{ 16.f, low_height_item ? 20.f : 32.f } * gui::dpi::_get_actual_scale(),
						gui::e_items_align::center,
						gui::resources::get_texture(HASH("settings_icon")));

				btn->override_style().transparent_clickable = true;
				btn->override_style().invisible_clickable = true;
				btn->override_style().text_color = { 60, 60, 60 };
				btn->override_style().text_hovered = { 100, 100, 100 };

				row->add(btn);
				return row;
			}

			gui::i_renderable* item_from_arg(state_t& s, int index) {
				if (!s.is_table(index))
					return nullptr;

				const auto type = s.get_field<int>(_UNIQUE_NAME("@@item_type"), index);

				switch (type) {
					case item_container: return (gui::containers::column_t*)s.get_field<int>(_UNIQUE_NAME("@@container_pointer"), index);
					case item_slider:
					case item_dropbox:
					case item_colorpicker:
					case item_text:
					case item_checkbox: return (gui::controls::i_control*)s.get_field<int>(_UNIQUE_NAME("@@node_pointer"), index); break;
				}

				return nullptr;
			}

			LUA_FN(node_add) {
				LUA_STATE(l);

				auto item = item_from_arg(s, 1);
				if (item == nullptr) {
					perform_error(STRS("node_add(): invalid menu item"));
					return 0;
				}

				auto item_to_add = item_from_arg(s, 2);
				if (item_to_add == nullptr) {
					perform_error(STRS("node_add(): invalid menu item to add"));
					return 0;
				}

				const auto item_type = item->get_item_type();

				// override our control to row which contains the button which opens popup
				if (item_type == gui::e_item_type::control) {
					gui::containers::column_t* column = new gui::containers::column_t{};
					//scripts_storage[s.get_id()].m_menu_items.emplace_back(column);

					column->add(item_to_add);
					s.push((int)nested_item(s, item, column));
					s.set_field(_UNIQUE_NAME("@@node_pointer"), 1);
					s.push((int)column);
					s.set_field(_UNIQUE_NAME("@@container_pointer"), 1);
				} else if (item_type == gui::e_item_type::container) {
					auto column = (gui::containers::column_t*)s.get_field<int>(_UNIQUE_NAME("@@container_pointer"), 1);
					if (column == nullptr) {
						perform_error(STRS("node_add(): invalid child"));
						return 0;
					}

					column->add(item_to_add);
				} else
					perform_error(STRS("node_add(): invalid menu item type"));

				return 0;
			}

			LUA_FN(node_set_key) {
				LUA_STATE(l);

				if (!s.is_table(1))
					return 0;

				const auto type = s.get_field<int>(_UNIQUE_NAME("@@item_type"), 1);
				const auto key = s.get_arg_impl<std::string>(2);
				if (!key.has_value())
					return 0;

				switch (type) {
					case item_checkbox: {
						const auto var_pointer = (script_storage_impl_t<incheat_vars::types::bool_t>*)s.get_field<int>(_UNIQUE_NAME("@@var_pointer"), 1);
						if (var_pointer == nullptr)
							return 0;

						var_pointer->first = *key;
					} break;

					case item_slider:
					case item_dropbox: {
						const auto var_pointer = (script_storage_impl_t<incheat_vars::types::int_t>*)s.get_field<int>(_UNIQUE_NAME("@@var_pointer"), 1);
						if (var_pointer == nullptr)
							return 0;

						var_pointer->first = *key;
					} break;

					case item_colorpicker: {
						const auto var_pointer = (script_storage_impl_t<incheat_vars::types::color_t>*)s.get_field<int>(_UNIQUE_NAME("@@var_pointer"), 1);
						if (var_pointer == nullptr)
							return 0;

						var_pointer->first = *key;
					} break;
				}

				return 0;
			}

			LUA_FN(node_get_key) {
				LUA_STATE(l);

				if (!s.is_table(1)) {
					s.push(nullptr);
					return 1;
				}

				const auto type = s.get_field<int>(_UNIQUE_NAME("@@item_type"), 1);
				switch (type) {
					case item_checkbox: {
						const auto var_pointer = (script_storage_impl_t<incheat_vars::types::bool_t>*)s.get_field<int>(_UNIQUE_NAME("@@var_pointer"), 1);
						if (var_pointer == nullptr) {
							s.push(nullptr);
							return 1;
						}

						s.push(var_pointer->first);
						return 1;
					} break;

					case item_slider:
					case item_dropbox: {
						const auto var_pointer = (script_storage_impl_t<incheat_vars::types::int_t>*)s.get_field<int>(_UNIQUE_NAME("@@var_pointer"), 1);
						if (var_pointer == nullptr) {
							s.push(nullptr);
							return 1;
						}

						s.push(var_pointer->first);
						return 1;
					} break;

					case item_colorpicker: {
						const auto var_pointer = (script_storage_impl_t<incheat_vars::types::color_t>*)s.get_field<int>(_UNIQUE_NAME("@@var_pointer"), 1);
						if (var_pointer == nullptr) {
							s.push(nullptr);
							return 1;
						}

						s.push(var_pointer->first);
						return 1;
					} break;
				}

				s.push(nullptr);
				return 1;
			}

			namespace ns_tab {
				LUA_FN(set_icon) {
					LUA_STATE(l);
					const auto tab = (tab_t*)s.get_field<int>(_UNIQUE_NAME("@@tab_pointer"), 1);
					if (tab == nullptr) {
						perform_error(STRS("set_icon(): invalid argument\nAre you using it with ':'?"));
						return 0;
					}

					tab->m_icon = s.arg<std::string>(2);
					return 0;
				}

				LUA_FN(set_color) {
					LUA_STATE(l);
					const auto tab = (tab_t*)s.get_field<int>(_UNIQUE_NAME("@@tab_pointer"), 1);
					if (tab == nullptr) {
						perform_error(STRS("set_color(): invalid argument\nAre you using it with ':'?"));
						return 0;
					}

					const auto color1 = s.get_arg_impl<color_t>(2);
					if (!color1.has_value()) {
						perform_error(STRS("set_color(): invalid argument"));
						return 0;
					}

					tab->m_colors = { *color1, s.arg_or(*color1, 3) };
					return 0;
				}
			} // namespace ns_tab

			namespace checkbox {
				LUA_FN(get_value) {
					LUA_STATE(l);

					if (!s.is_table(1)) {
						s.push(nullptr);
						return 1;
					}

					const auto type = s.get_field<int>(_UNIQUE_NAME("@@item_type"), 1);
					if (type != item_checkbox) {
						s.push(nullptr);
						return 1;
					}

					const auto var_pointer = (script_storage_impl_t<incheat_vars::types::bool_t>*)s.get_field<int>(_UNIQUE_NAME("@@var_pointer"), 1);
					if (var_pointer == nullptr) {
						s.push(nullptr);
						return 1;
					}

					s.push((bool)var_pointer->second);
					return 1;
				}

				LUA_FN(set_value) {
					LUA_STATE(l);

					if (!s.is_table(1)) {
						s.push(nullptr);
						return 1;
					}

					const auto type = s.get_field<int>(_UNIQUE_NAME("@@item_type"), 1);
					if (type != item_checkbox) {
						s.push(nullptr);
						return 1;
					}

					const auto var_pointer = (script_storage_impl_t<incheat_vars::types::bool_t>*)s.get_field<int>(_UNIQUE_NAME("@@var_pointer"), 1);
					if (var_pointer == nullptr) {
						s.push(nullptr);
						return 1;
					}

					const auto value = s.get_arg_impl<bool>(2);
					if (!value.has_value()) {
						lua::perform_error(STRS("checkbox.set_value(): invalid argument"));
						return 0;
					}

					var_pointer->second = *value;
					return 0;
				}

				LUA_FN(set_callback) {
					LUA_STATE(l);
					const auto checkbox = (gui::controls::checkbox_t*)s.get_field<int>(_UNIQUE_NAME("@@node_pointer"), 1);
					if (checkbox == nullptr)
						return 0;

					if (!s.is_function(2))
						return 0;

					const auto callback_name = std::format("{:X}", (uintptr_t)checkbox);

					// push button callback
					s.push_value(2);
					s.set_global(callback_name);

					// make c++ callback which will call lua global function
					checkbox->set_callback([callback_name, l]() {
						LUA_STATE(l);
						s.get_global(callback_name);
						if (!s.is_function(-1)) {
							perform_error(STRS("internal error: 4"), true);
							s.pop();
							return;
						}

						if (lua_pcall(l, 0, 0, 0) != 0) {
							perform_error(dformat(STRS("button_callback(): callback error: {}"), lua_tostring(l, -1)));
							s.pop();
						}
					});

					return 0;
				}

				LUA_FN(create) {
					LUA_STATE(l);

					const auto name = s.get_arg_impl<std::string>(1);
					if (!name.has_value()) {
						s.push(nullptr);
						return 1;
					}

					auto& vars = scripts_storage[s.get_id()].m_bools;
					auto& kv = vars.emplace_back();
					kv.first = *name;

					s.push(table_t{
							{
									{ _UNIQUE_NAME("@@node_pointer"), (int)gui::controls::checkbox(*name, &kv.second) },
									{ _UNIQUE_NAME("@@var_pointer"), (int)(&kv) },
									{ _UNIQUE_NAME("@@item_type"), item_checkbox },
									{ STR("get"), get_value },
									{ STR("set"), set_value },
									{ STR("add"), node_add },
									{ STR("set_key"), node_set_key },
									{ STR("get_key"), node_get_key },
									{ STR("set_callback"), set_callback },
							},
					});

					return 1;
				}
			} // namespace checkbox

			namespace slider {
				LUA_FN(get_value) {
					LUA_STATE(l);

					if (!s.is_table(1)) {
						s.push(nullptr);
						return 1;
					}

					const auto type = s.get_field<int>(_UNIQUE_NAME("@@item_type"), 1);
					if (type != item_slider) {
						s.push(nullptr);
						return 1;
					}

					const auto var_pointer = (script_storage_impl_t<int>*)s.get_field<int>(_UNIQUE_NAME("@@var_pointer"), 1);
					if (var_pointer == nullptr) {
						s.push(nullptr);
						return 1;
					}

					s.push(var_pointer->second);
					return 1;
				}

				LUA_FN(set_value) {
					LUA_STATE(l);

					if (!s.is_table(1)) {
						s.push(nullptr);
						return 1;
					}

					const auto type = s.get_field<int>(_UNIQUE_NAME("@@item_type"), 1);
					if (type != item_slider) {
						s.push(nullptr);
						return 1;
					}

					const auto var_pointer = (script_storage_impl_t<int>*)s.get_field<int>(_UNIQUE_NAME("@@var_pointer"), 1);
					if (var_pointer == nullptr) {
						s.push(nullptr);
						return 1;
					}

					const auto value = s.get_arg_impl<int>(2);
					if (!value.has_value()) {
						lua::perform_error(STRS("slider.set_value(): invalid argument"));
						return 0;
					};

					var_pointer->second = *value;
					return 0;
				}

				LUA_FN(set_callback) {
					LUA_STATE(l);
					const auto slider = (gui::controls::slider_t*)s.get_field<int>(_UNIQUE_NAME("@@node_pointer"), 1);
					if (slider == nullptr)
						return 0;

					if (!s.is_function(2))
						return 0;

					const auto callback_name = std::format("{:X}", (uintptr_t)slider);

					// push button callback
					s.push_value(2);
					s.set_global(callback_name);

					// make c++ callback which will call lua global function
					slider->set_callback([callback_name, l]() {
						LUA_STATE(l);
						s.get_global(callback_name);
						if (!s.is_function(-1)) {
							perform_error(STRS("internal error: ZV"), true);
							s.pop();
							return;
						}

						if (lua_pcall(l, 0, 0, 0) != 0) {
							perform_error(dformat(STRS("slider_callback(): callback error: {}"), lua_tostring(l, -1)));
							s.pop();
						}
					});

					return 0;
				}

				LUA_FN(create) {
					LUA_STATE(l);

					const auto name = s.get_arg_impl<std::string>(1);
					if (!name.has_value()) {
						s.push(nullptr);
						return 1;
					}

					const auto min = s.get_arg_impl<int>(2);
					if (!min.has_value()) {
						s.push(nullptr);
						return 1;
					}

					const auto max = s.get_arg_impl<int>(3);
					if (!max.has_value()) {
						s.push(nullptr);
						return 1;
					}

					const auto has_callback = s.is_function(4);
					auto& vars = scripts_storage[s.get_id()].m_ints;
					auto& kv = vars.emplace_back();
					kv.second = std::clamp(kv.second, *min, *max);
					kv.first = *name;

					auto slider = gui::controls::slider(*name, &kv.second, *min, *max);

					if (has_callback) {
						const auto callback_name = std::format("{:X}_fmt", (uintptr_t)slider);

						// push button callback
						s.push_value(4);
						s.set_global(callback_name);

						slider->set_format([callback_name, l](int value) -> std::string {
							LUA_STATE(l);
							s.get_global(callback_name);
							if (!s.is_function(-1)) {
								perform_error(STRS("internal error: 2"), true);
								s.pop();
								return gui::controls::slider_t::default_format(value);
							}

							s.push(value);
							if (lua_pcall(l, 1, 1, 0) != 0) {
								perform_error(dformat(STRS("slider_format(): callback error: {}"), lua_tostring(l, -1)));
								s.pop();
							}

							if (!s.is_string(-1))
								return gui::controls::slider_t::default_format(value);

							const auto result = s.arg<std::string>(-1);
							s.pop();
							return result;
						});
					}

					s.push(table_t{
							{
									{ _UNIQUE_NAME("@@node_pointer"), (int)slider },
									{ _UNIQUE_NAME("@@var_pointer"), (int)(&kv) },
									{ _UNIQUE_NAME("@@item_type"), item_slider },
									{ STR("get"), get_value },
									{ STR("set"), set_value },
									{ STR("add"), node_add },
									{ STR("set_key"), node_set_key },
									{ STR("get_key"), node_get_key },
									{ STR("set_callback"), set_callback },
							},
					});

					return 1;
				}
			} // namespace slider

			namespace color_picker {
				LUA_FN(get_value) {
					LUA_STATE(l);

					if (!s.is_table(1)) {
						s.push(nullptr);
						return 1;
					}

					const auto type = s.get_field<int>(_UNIQUE_NAME("@@item_type"), 1);
					if (type != item_colorpicker) {
						s.push(nullptr);
						return 1;
					}

					const auto var_pointer = (script_storage_impl_t<color_t>*)s.get_field<int>(_UNIQUE_NAME("@@var_pointer"), 1);
					if (var_pointer == nullptr) {
						s.push(nullptr);
						return 1;
					}

					s.push(var_pointer->second);
					return 1;
				}

				LUA_FN(set_value) {
					LUA_STATE(l);

					if (!s.is_table(1)) {
						s.push(nullptr);
						return 1;
					}

					const auto type = s.get_field<int>(_UNIQUE_NAME("@@item_type"), 1);
					if (type != item_colorpicker) {
						s.push(nullptr);
						return 1;
					}

					const auto var_pointer = (script_storage_impl_t<color_t>*)s.get_field<int>(_UNIQUE_NAME("@@var_pointer"), 1);
					if (var_pointer == nullptr) {
						s.push(nullptr);
						return 1;
					}

					const auto value = s.get_arg_impl<color_t>(2);
					if (!value.has_value()) {
						lua::perform_error(STRS("slider.set_value(): invalid argument"));
						return 0;
					};

					var_pointer->second = *value;
					return 0;
				}

				LUA_FN(create) {
					LUA_STATE(l);

					const auto name = s.get_arg_impl<std::string>(1);
					if (!name.has_value()) {
						s.push(nullptr);
						return 1;
					}

					auto& vars = scripts_storage[s.get_id()].m_colors;
					auto& kv = vars.emplace_back();
					kv.first = *name;

					/*s.push(table_t{
							{
									{ _UNIQUE_NAME("@@node_pointer"), (int)gui::controls::colorpicker(*name, &kv.second, s.arg_or(true, 2)) },
									{ _UNIQUE_NAME("@@var_pointer"), (int)(&kv) },
									{ _UNIQUE_NAME("@@item_type"), item_colorpicker },
									{ STR("get"), get_value },
									{ STR("set"), set_value },
									{ STR("add"), node_add },
									{ STR("set_key"), node_set_key },
									{ STR("get_key"), node_get_key },
							},
					});*/

					return 1;
				}
			} // namespace color_picker

			namespace button {
				LUA_FN(add) {
					perform_error(STRS("button_add(): this function is removed"));
					return 0;
				}

				LUA_FN(set_callback) {
					LUA_STATE(l);
					gui::controls::button_t* button = (gui::controls::button_t*)s.get_field<int>(_UNIQUE_NAME("@@node_pointer"), 1);
					if (button == nullptr)
						return 0;

					if (!s.is_function(2))
						return 0;

					const auto callback_name = std::format("{:X}", (uintptr_t)button);

					// push button callback
					s.push_value(2);
					s.set_global(callback_name);

					// make c++ callback which will call lua global function
					button->set_callback([callback_name, l]() {
						LUA_STATE(l);
						s.get_global(callback_name);
						if (!s.is_function(-1)) {
							perform_error(STRS("internal error: 3"), true);
							s.pop();
							return;
						}

						if (lua_pcall(l, 0, 0, 0) != 0) {
							perform_error(dformat(STRS("button_callback(): callback error: {}"), lua_tostring(l, -1)));
							s.pop();
						}
					});

					return 0;
				}

				LUA_FN(create) {
					LUA_STATE(l);

					const auto name = s.get_arg_impl<std::string>(1);
					if (!name.has_value()) {
						s.push(nullptr);
						return 1;
					}

					if (!s.is_function(2)) {
						s.push(nullptr);
						return 1;
					}

					auto button_ref = gui::controls::button(*name, nullptr);
					const auto callback_name = std::format("{:X}", (uintptr_t)button_ref);

					// push button callback
					s.push_value(2);
					s.set_global(callback_name);

					// make c++ callback which will call lua global function
					button_ref->set_callback([callback_name, l]() {
						LUA_STATE(l);
						s.get_global(callback_name);
						if (!s.is_function(-1)) {
							perform_error(STRS("internal error: 1"), true);
							s.pop();
							return;
						}

						if (lua_pcall(l, 0, 0, 0) != 0) {
							perform_error(dformat(STRS("button_callback(): callback error: {}"), lua_tostring(l, -1)));
							s.pop();
						}
					});

					s.push(table_t{
							{
									{ _UNIQUE_NAME("@@node_pointer"), (int)button_ref },
									{ _UNIQUE_NAME("@@item_type"), item_colorpicker },
									{ STRS("add"), button::add },
									{ STRS("set_callback"), button::set_callback },
							},
					});

					return 1;
				}
			} // namespace button

			namespace dropbox {
				LUA_FN(get_value) {
					LUA_STATE(l);

					if (!s.is_table(1)) {
						s.push(nullptr);
						return 1;
					}

					const auto type = s.get_field<int>(_UNIQUE_NAME("@@item_type"), 1);
					if (type != item_dropbox) {
						s.push(nullptr);
						return 1;
					}

					const auto var_pointer = (script_storage_impl_t<int>*)s.get_field<int>(_UNIQUE_NAME("@@var_pointer"), 1);
					if (var_pointer == nullptr) {
						s.push(nullptr);
						return 1;
					}

					s.push(var_pointer->second);
					return 1;
				}

				LUA_FN(set_value) {
					LUA_STATE(l);

					if (!s.is_table(1)) {
						s.push(nullptr);
						return 1;
					}

					const auto type = s.get_field<int>(_UNIQUE_NAME("@@item_type"), 1);
					if (type != item_dropbox) {
						s.push(nullptr);
						return 1;
					}

					const auto var_pointer = (script_storage_impl_t<int>*)s.get_field<int>(_UNIQUE_NAME("@@var_pointer"), 1);
					if (var_pointer == nullptr) {
						s.push(nullptr);
						return 1;
					}

					const auto value = s.get_arg_impl<int>(2);
					if (!value.has_value()) {
						lua::perform_error(STRS("slider.set_value(): invalid argument"));
						return 0;
					};

					var_pointer->second = *value;
					return 0;
				}

				LUA_FN(create) {
					// label: text, multiselect: boolean, items: any[, ...]
					LUA_STATE(l);

					const auto name = s.get_arg_impl<std::string>(1);
					if (!name.has_value()) {
						s.push(nullptr);
						return 1;
					}

					const auto multiselect = s.get_arg_impl<bool>(2);
					if (!multiselect.has_value()) {
						s.push(nullptr);
						return 1;
					}

					auto items = s.get_arg_impl<std::vector<std::string>>(3);
					if (!items.has_value()) {
						items = std::vector<std::string>{};
						for (int i = 3, top = lua_gettop(l); i <= top; ++i) {
							auto item = s.get_arg_impl<std::string>(i);
							if (item.has_value())
								items->emplace_back(std::move(*item));
						}
					}

					if (items->size() < 1) {
						perform_error(STRS("ui.dropbox(): dropbox must have at least 1 item"));
						s.push(nullptr);
						return 1;
					}

					if (items->size() > 32) {
						perform_error(STRS("ui.dropbox(): dropbox must have less than 32 items"));
						s.push(nullptr);
						return 1;
					}

					auto& vars = scripts_storage[s.get_id()].m_ints;
					auto& kv = vars.emplace_back();
					kv.first = *name;

					s.push(table_t{
							{
									{ _UNIQUE_NAME("@@node_pointer"), (int)gui::controls::dropbox(*name, &kv.second, *items, *multiselect) },
									{ _UNIQUE_NAME("@@var_pointer"), (int)(&kv) },
									{ _UNIQUE_NAME("@@item_type"), item_dropbox },
									{ STR("get"), get_value },
									{ STR("set"), set_value },
									{ STR("add"), node_add },
									{ STR("set_key"), node_set_key },
									{ STR("get_key"), node_get_key },
							},
					});

					return 1;
				}
			} // namespace dropbox

			LUA_FN(create_text) {
				// value: text[, color: Color, bold: boolean]
				LUA_STATE(l);

				const auto label = s.get_arg_impl<std::string>(1);
				if (!label.has_value()) {
					s.push(nullptr);
					return 1;
				}

				const auto color = s.arg_or(gui::styles::get().text_color, 2);
				const auto bold = s.arg_or(false, 3);

				auto item = gui::controls::text(*label, bold);
				item->override_style().text_color = color;

				s.push(table_t{
						{
								{ _UNIQUE_NAME("@@node_pointer"), (int)item },
								{ _UNIQUE_NAME("@@item_type"), item_text },
								{ STR("add"), node_add },
						},
				});

				return 1;
			}

			LUA_FN(container_add) {
				LUA_STATE(l);

				auto container = (gui::containers::column_t*)s.get_field<int>(_UNIQUE_NAME("@@container_pointer"), 1);
				if (container == nullptr) {
					perform_error(STRS("container_add(): invalid argument"));
					return 0;
				}

				auto item = item_from_arg(s, 2);
				if (item != nullptr) {
					container->add(item);
				} else
					perform_error(STRS("container_add(): unknown menu item"));

				return 0;
			}

			static table_t create_container(state_t& s, gui::containers::column_t* ptr) {
				return table_t{
					{
							{ _UNIQUE_NAME("@@container_pointer"), (int)ptr },
							{ _UNIQUE_NAME("@@item_type"), item_container },
							{ STR("add"), container_add },
					}
				};
			}

			LUA_FN(register_tab) {
				LUA_STATE(l);

				if (!s.is_table(1)) {
					perform_error(STRS("register_tab(): invalid argument"));
					return 0;
				}

				const auto tab = (tab_t*)s.get_field<int>(_UNIQUE_NAME("@@tab_pointer"), 1);
				if (tab == nullptr) {
					perform_error(STRS("register_tab(): invalid argument"));
					return 0;
				}

				/*auto& t = menu->m_lua_tabs.emplace_back();
				t.first = s.get_id();
				t.second.m_name = tab->m_name;
				t.second.m_icon = tab->m_icon.empty() ? gui::resources::get_texture(HASH("scripts_icon")) : gui::create_texture_from_buffer(utils::read_file_bin(tab->m_icon)).get();
				t.second.m_colors = tab->m_colors;

				auto left = (gui::containers::column_t*)tab->m_items.first[0];
				for (auto item: left->get_items())
					t.second.m_items.first.emplace_back(item);

				auto right = (gui::containers::column_t*)tab->m_items.second[0];
				for (auto item: right->get_items())
					t.second.m_items.second.emplace_back(item);*/

				s.push(nullptr);
				s.set_field(_UNIQUE_NAME("@@tab_pointer"), 1);

				s.push(nullptr);
				s.set_field(STR("left"), 1);

				s.push(nullptr);
				s.set_field(STR("right"), 1);
				return 0;
			}

			LUA_FN(create_tab) {
				LUA_STATE(l);
				auto tab = std::make_shared<tab_t>();
				scripts_storage[s.get_id()].m_tabs.emplace_back(tab);

				const auto tab_name = s.get_arg_impl<std::string>(1);
				if (!tab_name.has_value()) {
					s.push(nullptr);
					return 1;
				}

				if (tab_name->empty()) {
					s.push(nullptr);
					return 1;
				}

				tab->m_name = *tab_name;
				tab->m_colors = { {}, {} };

				auto right = new gui::containers::column_t{};
				auto left = new gui::containers::column_t{};

				scripts_storage[s.get_id()].m_menu_items.emplace_back(right);
				scripts_storage[s.get_id()].m_menu_items.emplace_back(left);

				/*tab->m_items.first.emplace_back(left);
				tab->m_items.second.emplace_back(right);*/

				s.push(table_t{
						{
								{ _UNIQUE_NAME("@@tab_pointer"), (int)tab.get() },
								{ STR("set_icon"), ns_tab::set_icon },
								{ STR("set_color"), ns_tab::set_color },
								{ STR("left"), create_container(s, left) },
								{ STR("right"), create_container(s, right) },
								{ STR("register"), register_tab },
						},
				});
				return 1;
			}

			LUA_FN(create_group) {
				LUA_STATE(l);
				auto group = new gui::containers::group_t{ s.arg<std::string>(1) };

				s.push(table_t{
						{
								{ _UNIQUE_NAME("@@container_pointer"), (int)group },
								{ _UNIQUE_NAME("@@item_type"), item_container },
								{ STR("add"), container_add },
						} });
				return 1;
			}

			LUA_FN(update) {
				//menu->recreate_gui();
				return 0;
			}

			LUA_FN(get_icon) {
				LUA_STATE(l);

				const auto name = s.get_arg_impl<std::string>(1);
				if (!name.has_value()) {
					s.push(nullptr);
					return 1;
				}

				if (!fa::icons.contains(*name)) {
					s.push(nullptr);
					return 1;
				}

				s.push(fa::icons.at(*name));
				return 1;
			}

			LUA_FN(override_accent) {
				LUA_STATE(l);

				auto color1 = s.get_arg_impl<color_t>(1);
				auto color2 = s.get_arg_impl<color_t>(2);
				if (!color1.has_value() || !color2.has_value())
					return 0;

				gui::accent_color1 = *color1;
				gui::accent_color2 = *color2;
				return 0;
			}

			LUA_FN(get_accent) {
				LUA_STATE(l);
				s.push(gui::accent_color1);
				s.push(gui::accent_color2);
				return 2;
			}

			LUA_FN(get_menu_size) {
				LUA_STATE(l);
				//s.push(menu->window->get_size());
				return 1;
			}

			LUA_FN(get_menu_position) {
				LUA_STATE(l);
				//s.push(menu->window->get_position());
				return 1;
			}

			LUA_FN(set_menu_position) {
				LUA_STATE(l);
				auto pos = s.get_arg_impl<vec2d>(1);
				if (pos.has_value())
					return 0;

				//menu->window->set_position(*pos);
				return 0;
			}

			LUA_FN(get_dpi_scale) {
				LUA_STATE(l);
				s.push(gui::dpi::_get_actual_scale());
				return 1;
			}

			LUA_FN(get_mouse_position) {
				LUA_STATE(l);
				s.push(gui::globals().mouse_position);
				return 1;
			}

			LUA_FN(get_mouse_wheel) {
				LUA_STATE(l);
				s.push(gui::globals().mouse_wheel);
				return 1;
			}

			LUA_FN(is_mouse_pressed) {
				LUA_STATE(l);
				s.push(gui::globals().mouse_down[s.arg_or(false, 1) ? 1 : 0]);
				return 1;
			}

			LUA_FN(get_menu_alpha) {
				LUA_STATE(l);
				//s.push(menu->window->get_alpha());
				return 1;
			}
		} // namespace ui

		namespace render_ {
			// path: string, size: number[, flags: number]
			LUA_FN(create_font) {
				LUA_STATE(l);

				auto& font = scripts_storage[s.get_id()].m_font_queue.emplace_back();
				font.m_path = s.arg<std::string>(1);
				font.m_size = s.arg<float>(2);
				font.m_flags = s.arg_or(0, 3);
				font.m_pointer = (void**)(new uintptr_t{});

				s.push(table_t{
						{
								{ _UNIQUE_NAME("@@font_pointer"), (int)font.m_pointer },
						},
				});
				return 1;
			}

			// font: Font, text: string, position: Vector[, color: Color, flags: number, wrap_width: number]
			LUA_FN(text) {
				if (!(render::in_fsn)) {
					perform_error(STRS("render.text(): function is only allowed in 'render' callback"));
					return 0;
				}

				LUA_STATE(l);

				const auto font_ptr = (void**)s.get_field<int>(_UNIQUE_NAME("@@font_pointer"), 1);

				if (*font_ptr == nullptr)
					return 0;

				const auto message = s.arg<std::string>(2);
				const auto pos = s.arg<vec2d>(3);
				const auto color = s.arg_or(color_t{}, 4);
				const auto flags = s.arg_or(0, 5);
				const auto wrap_width = s.arg_or(0.f, 6);

				const auto coord = render::to_imvec2(pos);
				const auto size = render::calc_text_size(message, *font_ptr);
				const auto coord_out = ImVec2{ coord.x + 1.f, coord.y + 1.f };
				const auto outline_clr = color_t{ 0, 0, 0 }.modify_alpha((color.a() / 255.f) * (color.a() / 255.f));
				const auto pfont = (::font_t)*font_ptr;
				auto pos_ = ImVec2{ coord.x, coord.y };

				render::draw_list_fsn->PushTextureID(pfont->ContainerAtlas->TexID);

				if (flags & render::align_left || flags & render::align_bottom) {
					if (flags & render::align_left) {
						pos_.x -= size.x;

						if (flags & render::centered_y)
							pos_.y -= size.y * 0.5f;
					}

					if (flags & render::align_bottom) {
						pos_.y -= size.y;

						if (flags & render::centered_x)
							pos_.x -= size.x * 0.5f;
					}
				} else {
					if (flags & render::centered_x)
						pos_.x -= size.x * 0.5f;
					if (flags & render::centered_y)
						pos_.y -= size.y * 0.5f;
				}

				auto output = message.c_str();

				if (flags & render::outline) {
					pos_.y++;
					render::draw_list_fsn->AddText(pfont, pfont->FontSize, pos_, outline_clr.abgr(), output, nullptr, wrap_width);
					pos_.x++;
					render::draw_list_fsn->AddText(pfont, pfont->FontSize, pos_, outline_clr.abgr(), output, nullptr, wrap_width);
					pos_.y--;
					render::draw_list_fsn->AddText(pfont, pfont->FontSize, pos_, outline_clr.abgr(), output, nullptr, wrap_width);
					pos_.x--;
					render::draw_list_fsn->AddText(pfont, pfont->FontSize, pos_, outline_clr.abgr(), output, nullptr, wrap_width);
				}

				render::draw_list_fsn->AddText(pfont, pfont->FontSize, pos_, color.abgr(), output, nullptr, wrap_width);
				render::draw_list_fsn->PopTextureID();
				return 0;
			}

			// path: string
			LUA_FN(create_texture) {
				LUA_STATE(l);

				auto& texture = scripts_storage[s.get_id()].m_texture_queue.emplace_back();
				texture.m_path = s.arg<std::string>(1);
				texture.m_pointer = (void**)(new uintptr_t{});

				s.push(table_t{
						{
								{ _UNIQUE_NAME("@@texture_pointer"), (int)texture.m_pointer },
						},
				});
				return 1;
			}

			// texture: Texture, position: Vector[, size: Vector, color: Color, rounding: number]
			LUA_FN(image) {
				if (!(render::in_fsn)) {
					perform_error(STRS("render.image(): function is only allowed in 'render' callback"));
					return 0;
				}

				LUA_STATE(l);

				const auto texture_ptr = (void**)s.get_field<int>(_UNIQUE_NAME("@@texture_pointer"), 1);
				if (*texture_ptr == nullptr)
					return 0;

				const auto pos = s.get_arg_impl<vec2d>(2);
				const auto siZe = s.get_arg_impl<vec2d>(3);
				const auto color = s.arg_or(color_t{}, 4);
				const auto rounding = s.arg_or(0.f, 5);

				if (!pos.has_value() || !siZe.has_value())
					return 0;

				(*(render::gif_t**)texture_ptr)->render(render::draw_list_fsn, *pos, *siZe, color, rounding);
				return 0;
			}

			// mins: Vector, maxs: Vector, color: Color[, rounding: number, rounding_flags: number]
			LUA_FN(filled_rect) {
				if (!(render::in_fsn)) {
					perform_error(STRS("render.filled_rect(): function is only allowed in 'render' callback"));
					return 0;
				}

				LUA_STATE(l);

				const auto min = s.get_arg_impl<vec2d>(1);
				const auto max = s.get_arg_impl<vec2d>(2);
				const auto color = s.get_arg_impl<color_t>(3);

				if (!opt_has_value(min, max, color))
					return 0;

				const auto rounding = s.arg_or(0.f, 4);
				const auto flags = s.arg_or(0, 5);

				render::draw_list_fsn->AddRectFilled(render::to_imvec2(*min), render::to_imvec2(*max), color->u32(), rounding, flags);
				return 0;
			}

			// mins: Vector, maxs: Vector, color: Color[, thickness: number, rounding: number, rounding_flags: number]
			LUA_FN(rect) {
				if (!(render::in_fsn)) {
					perform_error(STRS("render.rect(): function is only allowed in 'render' callback"));
					return 0;
				}

				LUA_STATE(l);

				const auto min = s.get_arg_impl<vec2d>(1);
				const auto max = s.get_arg_impl<vec2d>(2);
				const auto color = s.get_arg_impl<color_t>(3);

				if (!opt_has_value(min, max, color))
					return 0;

				const auto thickness = s.arg_or(1.f, 4);
				const auto rounding = s.arg_or(0.f, 5);
				const auto flags = s.arg_or(0, 6);

				render::draw_list_fsn->AddRect(render::to_imvec2(*min), render::to_imvec2(*max), color->u32(), rounding, flags, thickness);
				return 0;
			}

			// pos1: Vector, pos2: Vector, color: Color[, thickness: number]
			LUA_FN(line) {
				if (!(render::in_fsn)) {
					perform_error(STRS("render.line(): function is only allowed in 'render' callback"));
					return 0;
				}

				LUA_STATE(l);

				const auto p1 = s.get_arg_impl<vec2d>(1);
				const auto p2 = s.get_arg_impl<vec2d>(2);
				const auto color = s.get_arg_impl<color_t>(3);

				if (!opt_has_value(p1, p2, color))
					return 0;

				const auto thickness = s.arg_or(1.f, 4);

				render::draw_list_fsn->AddLine(render::to_imvec2(*p1), render::to_imvec2(*p2), color->u32(), thickness);
				return 0;
			}

			// position: Vector, radius: number, color: Color[, segments: number]
			LUA_FN(filled_circle) {
				if (!(render::in_fsn)) {
					perform_error(STRS("render.filled_circle(): function is only allowed in 'render' callback"));
					return 0;
				}

				LUA_STATE(l);

				const auto pos = s.get_arg_impl<vec2d>(1);
				const auto radius = s.get_arg_impl<float>(2);
				const auto color = s.get_arg_impl<color_t>(3);

				if (!opt_has_value(pos, radius, color))
					return 0;

				const auto segments = s.arg_or(0, 4);

				render::draw_list_fsn->AddCircleFilled(render::to_imvec2(*pos), *radius, color->u32(), segments);
				return 0;
			}

			// position: Vector, radius: number, color: Color[thickness: number, segments: `number`]
			LUA_FN(circle) {
				if (!(render::in_fsn)) {
					perform_error(STRS("render.circle(): function is only allowed in 'render' callback"));
					return 0;
				}

				LUA_STATE(l);

				const auto pos = s.get_arg_impl<vec2d>(1);
				const auto radius = s.get_arg_impl<float>(2);
				const auto color = s.get_arg_impl<color_t>(3);

				if (!opt_has_value(pos, radius, color))
					return 0;

				const auto thickness = s.arg_or(1.f, 4);
				const auto segments = s.arg_or(0, 5);

				render::draw_list_fsn->AddCircle(render::to_imvec2(*pos), *radius, color->u32(), segments, thickness);
				return 0;
			}

			// position: `Vector`, radius: `number`, angle_min: `number`, angle_max: `number`, color: `Color`[, thickness: `number`, segments: `number`]
			LUA_FN(arc) {
				if (!(render::in_fsn)) {
					perform_error(STRS("render.arc(): function is only allowed in 'render' callback"));
					return 0;
				}

				LUA_STATE(l);

				const auto pos = s.get_arg_impl<vec2d>(1);
				const auto radius = s.get_arg_impl<float>(2);
				const auto angle_min = s.get_arg_impl<float>(3);
				const auto angle_max = s.get_arg_impl<float>(4);
				const auto color = s.get_arg_impl<color_t>(5);

				if (!opt_has_value(pos, radius, angle_min, angle_max, color))
					return 0;

				const auto thickness = s.arg_or(1.f, 6);
				const auto segments = s.arg_or(0, 7);

				render::draw_list_fsn->PathArcTo(render::to_imvec2(*pos), *radius, *angle_min, *angle_max, segments);
				render::draw_list_fsn->PathStroke(color->u32(), 0, thickness);
				return 0;
			}

			// position: `Vector`, radius: `number`, angle_min: `number`, angle_max: `number`, color: `Color`[, segments: `number`]
			LUA_FN(filled_arc) {
				if (!(render::in_fsn)) {
					perform_error(STRS("render.filled_arc(): function is only allowed in 'render' callback"));
					return 0;
				}

				LUA_STATE(l);

				const auto pos = s.get_arg_impl<vec2d>(1);
				const auto radius = s.get_arg_impl<float>(2);
				const auto angle_min = s.get_arg_impl<float>(3);
				const auto angle_max = s.get_arg_impl<float>(4);
				const auto color = s.get_arg_impl<color_t>(5);

				if (!opt_has_value(pos, radius, angle_min, angle_max, color))
					return 0;

				const auto segments = s.arg_or(0, 6);

				render::draw_list_fsn->PathArcTo(render::to_imvec2(*pos), *radius, *angle_min, *angle_max, segments);
				render::draw_list_fsn->PathFillConvex(color->u32());
				return 0;
			}

			// mins: Vector, maxs: Vector, color_top: Color, color_bottom: Color[, rounding: number, rounding_flags: number]
			LUA_FN(vertical_gradient) {
				if (!(render::in_fsn)) {
					perform_error(STRS("render.vertical_gradient(): function is only allowed in 'render' callback"));
					return 0;
				}

				LUA_STATE(l);
				const auto min = s.get_arg_impl<vec2d>(1);
				const auto max = s.get_arg_impl<vec2d>(2);
				const auto color_top = s.get_arg_impl<color_t>(3);
				const auto color_bottom = s.get_arg_impl<color_t>(4);

				if (!opt_has_value(min, max, color_top, color_bottom))
					return 0;

				const auto rounding = s.arg_or(0.f, 5);
				const auto flags = s.arg_or(0, 5);

				ImDrawGradient_Linear gradient{
					{ min->x + max->y * 0.5f, min->y },
					{ min->x + max->y * 0.5f, max->y },
					render::to_imvec4(*color_top),
					render::to_imvec4(*color_bottom)
				};

				render::draw_list_fsn->AddRectFilled(render::to_imvec2(*min), render::to_imvec2(*max), gradient, rounding, flags);
				return 0;
			}

			// mins: Vector, maxs: Vector, color_left: Color, color_right: Color[, rounding: number, rounding_flags: number]
			LUA_FN(horizontal_gradient) {
				if (!(render::in_fsn)) {
					perform_error(STRS("render.horizontal_gradient(): function is only allowed in 'render' callback"));
					return 0;
				}

				LUA_STATE(l);
				const auto min = s.get_arg_impl<vec2d>(1);
				const auto max = s.get_arg_impl<vec2d>(2);
				const auto color_left = s.get_arg_impl<color_t>(3);
				const auto color_right = s.get_arg_impl<color_t>(4);

				if (!opt_has_value(min, max, color_left, color_right))
					return 0;

				const auto rounding = s.arg_or(0.f, 5);
				const auto flags = s.arg_or(0, 5);

				ImDrawGradient_Linear gradient{
					{ min->x, min->y + max->y * 0.5f },
					{ max->x, min->y + max->y * 0.5f },
					render::to_imvec4(*color_left),
					render::to_imvec4(*color_right)
				};

				render::draw_list_fsn->AddRectFilled(render::to_imvec2(*min), render::to_imvec2(*max), gradient, rounding, flags);
				return 0;
			}

			LUA_FN(get_screen_size) {
				LUA_STATE(l);
				s.push(vec2d{ render::screen_width, render::screen_height });
				return 1;
			}

			LUA_FN(world_to_screen) {
				LUA_STATE(l);

				const auto position = s.get_arg_impl<vec3d>(1);
				if (position.has_value()) {
					vec2d result{};
					if (math::world_to_screen(*position, result))
						s.push(result);
					else
						s.push(nullptr);
				} else
					s.push(nullptr);

				return 1;
			}

			LUA_FN(text_size) {
				LUA_STATE(l);

				const auto font_ptr = (void**)s.get_field<int>(_UNIQUE_NAME("@@font_pointer"), 1);

				if (*font_ptr == nullptr)
					return 0;

				const auto message = s.get_arg_impl<std::string>(2);
				const auto wrap_width = s.arg_or(0.f, 3);

				if (!message.has_value()) {
					s.push(nullptr);
					return 1;
				}

				s.push(render::calc_text_size(*message, *font_ptr, wrap_width));
				return 1;
			}
		} // namespace render_

		namespace events {
#define EVENT_FROM_ARG(n) ((sdk::game_event_t*)s.get_field<int>(STR("@@event_pointer"), n))

			LUA_FN(get_name) {
				LUA_STATE(l);
				auto evt = EVENT_FROM_ARG(1);
				if (evt == nullptr) {
					s.push(nullptr);
					return 1;
				}

				s.push(std::string{ evt->get_name() });
				return 1;
			}

			LUA_FN(get_int) {
				LUA_STATE(l);

				auto evt = EVENT_FROM_ARG(1);
				if (evt == nullptr) {
					s.push(nullptr);
					return 1;
				}

				const auto key = s.get_arg_impl<std::string>(2);
				if (!key.has_value()) {
					s.push(nullptr);
					return 1;
				}

				s.push(evt->get_int(key->c_str()));
				return 1;
			}

			LUA_FN(get_float) {
				LUA_STATE(l);

				auto evt = EVENT_FROM_ARG(1);
				if (evt == nullptr) {
					s.push(nullptr);
					return 1;
				}

				const auto key = s.get_arg_impl<std::string>(2);
				if (!key.has_value()) {
					s.push(nullptr);
					return 1;
				}

				s.push(evt->get_float(key->c_str()));
				return 1;
			}

			LUA_FN(get_string) {
				LUA_STATE(l);

				auto evt = EVENT_FROM_ARG(1);
				if (evt == nullptr) {
					s.push(nullptr);
					return 1;
				}

				const auto key = s.get_arg_impl<std::string>(2);
				if (!key.has_value()) {
					s.push(nullptr);
					return 1;
				}

				s.push(std::string{ evt->get_string(key->c_str()) });
				return 1;
			}

			LUA_FN(set_int) {
				LUA_STATE(l);

				auto evt = EVENT_FROM_ARG(1);
				if (evt == nullptr)
					return 0;

				const auto key = s.get_arg_impl<std::string>(2);
				if (!key.has_value())
					return 0;

				const auto value = s.get_arg_impl<int>(3);
				if (!value.has_value())
					return 0;

				evt->set_int(key->c_str(), *value);
				return 0;
			}

			LUA_FN(set_float) {
				LUA_STATE(l);

				auto evt = EVENT_FROM_ARG(1);
				if (evt == nullptr)
					return 0;

				const auto key = s.get_arg_impl<std::string>(2);
				if (!key.has_value())
					return 0;

				const auto value = s.get_arg_impl<float>(3);
				if (!value.has_value())
					return 0;

				evt->set_float(key->c_str(), *value);
				return 0;
			}

			LUA_FN(set_string) {
				LUA_STATE(l);

				auto evt = EVENT_FROM_ARG(1);
				if (evt == nullptr)
					return 0;

				const auto key = s.get_arg_impl<std::string>(2);
				if (!key.has_value())
					return 0;

				const auto value = s.get_arg_impl<std::string>(3);
				if (!value.has_value())
					return 0;

				evt->set_string(key->c_str(), value->c_str());
				return 0;
			}

#undef EVENT_FROM_ARG

		} // namespace events

		namespace cvars {
#define CVAR_FROM_ARG(n) ((sdk::convar_t*)s.get_field<int>(_UNIQUE_NAME("@@cvar_pointer"), n))

			LUA_FN(get_int) {
				LUA_STATE(l);

				auto cvar = CVAR_FROM_ARG(1);
				if (cvar != nullptr)
					s.push(cvar->get_int());
				else
					s.push(nullptr);

				return 1;
			}

			LUA_FN(get_float) {
				LUA_STATE(l);

				auto cvar = CVAR_FROM_ARG(1);
				if (cvar != nullptr)
					s.push(cvar->get_float());
				else
					s.push(nullptr);

				return 1;
			}

			LUA_FN(get_string) {
				LUA_STATE(l);

				auto cvar = CVAR_FROM_ARG(1);
				if (cvar != nullptr)
					s.push(cvar->get_string());
				else
					s.push(nullptr);

				return 1;
			}

			LUA_FN(set_int) {
				LUA_STATE(l);

				auto cvar = CVAR_FROM_ARG(1);
				if (cvar != nullptr) {
					auto value = s.get_arg_impl<int>(2);

					if (!value.has_value()) {
						perform_error(dformat(STRS("cvar.set_int(): invalid argument at {} index\nAre you using it with ':'?"), 2));
						return 0;
					}

					cvar->set_value(*value);
				}

				return 0;
			}

			LUA_FN(set_float) {
				LUA_STATE(l);

				auto cvar = CVAR_FROM_ARG(1);
				if (cvar != nullptr) {
					auto value = s.get_arg_impl<float>(2);

					if (!value.has_value()) {
						perform_error(dformat(STRS("cvar.set_float(): invalid argument at {} index"), 2));
						return 0;
					}

					cvar->set_value(*value);
				}

				return 0;
			}

			LUA_FN(set_string) {
				LUA_STATE(l);

				auto cvar = CVAR_FROM_ARG(1);
				if (cvar != nullptr) {
					auto value = s.get_arg_impl<std::string>(2);

					if (!value.has_value()) {
						perform_error(dformat(STRS("cvar.set_string(): invalid argument at {} index"), 2));
						return 0;
					}

					cvar->set_value(value->c_str());
				}

				return 0;
			}

			LUA_FN(find) {
				LUA_STATE(l);
				const auto cvar = interfaces::convar->find_var(fnva1(s.arg<std::string>(1).c_str()));

				if (cvar == nullptr) {
					s.push(nullptr);
					return 1;
				}

				s.push(table_t{
						{
								{ _UNIQUE_NAME("@@cvar_pointer"), (int)cvar },
								{ STR("get_int"), get_int },
								{ STR("get_float"), get_float },
								{ STR("get_string"), get_string },
								{ STR("set_int"), set_int },
								{ STR("set_float"), set_float },
								{ STR("set_string"), set_string },
						},
				});
				return 1;
			}
#undef CVAR_FROM_ARG
		} // namespace cvars

		namespace vars {
#define CHECK_PTR(ptr) ((uintptr_t)(ptr) < ((uintptr_t)&settings->end))
			LUA_FN(get) {
				LUA_STATE(l);
				if (!s.is_table(1)) {
					perform_error(STRS("vars.get(): invalid argument"));
					s.push(nullptr);
					return 1;
				}

				auto var_type = s.get_field<int>(_UNIQUE_NAME("@@var_type"), 1);
				auto var = s.get_field<int>(_UNIQUE_NAME("@@var_pointer"), 1);

				auto var_pointer = (int)var ^ BASE_ADDRESS;
				if (CHECK_PTR(var_pointer)) {
					switch (var_type) {
						case -1: perform_error(STRS("vars.set(): invalid var type")); break;
						case 0: s.push((bool)*(incheat_vars::types::bool_t*)var_pointer); break;
						case 1: s.push(*(incheat_vars::types::int_t*)var_pointer); break;
						case 2: s.push((int)*(incheat_vars::types::flags_t*)var_pointer); break;
						case 3: s.push((*(incheat_vars::types::color_t*)var_pointer).get()); break;
					}
				} else
					s.push(nullptr);

				return 1;
			}

			LUA_FN(set) {
				LUA_STATE(l);
				if (!s.is_table(1)) {
					perform_error(STRS("vars.set(): invalid argument"));
					s.push(nullptr);
					return 1;
				}

				auto var_type = s.get_field<int>(_UNIQUE_NAME("@@var_type"), 1);
				auto var = s.get_field<int>(_UNIQUE_NAME("@@var_pointer"), 1);

				auto var_pointer = (int)var ^ BASE_ADDRESS;
				if (CHECK_PTR(var_pointer)) {
					switch (var_type) {
						case -1: perform_error(STRS("vars.set(): invalid var type")); break;
						case 0: *(incheat_vars::types::bool_t*)var_pointer = s.arg<bool>(2); break;
						case 1: *(incheat_vars::types::int_t*)var_pointer = s.arg<int>(2); break;
						case 2: *(incheat_vars::types::flags_t*)var_pointer = s.arg<int>(2); break;
						case 3: *(incheat_vars::types::color_t*)var_pointer = s.arg<color_t>(2); break;
					}
				}

				return 0;
			}

			LUA_FN(find) {
				LUA_STATE(l);

				const auto var_name = _VAR_GET_NAME_RT(s.arg<std::string>(1).c_str());

				int var_type = -1;
				void* var = nullptr;

				// pizdec
				{
					if (var = settings->find_bool(var_name)) {
						var_type = 0;
					} else if (var = settings->find_int(var_name)) {
						var_type = 1;
					} else if (var = settings->find_flag(var_name)) {
						var_type = 2;
					} else if (var = settings->find_color(var_name)) {
						var_type = 3;
					}
				}

				if (var != nullptr) {
					s.push(table_t{
							{
									{ _UNIQUE_NAME("@@var_pointer"), (int)((int)var ^ BASE_ADDRESS) },
									{ _UNIQUE_NAME("@@var_type"), var_type },
									{ STR("get"), get },
									{ STR("set"), set },
							},
					});

				} else
					s.push(nullptr);

				return 1;
			}
#undef CHECK_PTR
		} // namespace vars

		namespace entity {
			LUA_FN(get_prop) {
				LUA_STATE(l);

				static std::unordered_map<std::string, std::pair<sdk::recv_prop_t*, uintptr_t>> cached_offsets{};
				const auto entity = (sdk::base_entity_t*)s.get_field<int>(_UNIQUE_NAME("@@entity_pointer"), 1);
				if (entity == nullptr) {
					s.push(nullptr);
					return 1;
				}

				const auto table_name = s.arg<std::string>(2);
				if (table_name.empty()) {
					s.push(nullptr);
					return 1;
				}

				const auto prop_name = s.arg<std::string>(3);
				if (prop_name.empty()) {
					s.push(nullptr);
					return 1;
				}

				const auto key = dformat(STR("{}->{}"), table_name, prop_name);

				if (!cached_offsets.contains(key)) {
					sdk::recv_prop_t* prop = nullptr;
					const auto offset = netvars->get_offset(fnva1(table_name.c_str()), fnva1(prop_name.c_str()), &prop);
					if (prop == nullptr) {
						s.push(nullptr);
						return 1;
					}

					cached_offsets[key] = { prop, offset };
				}

				const auto& [prop, offset] = cached_offsets[key];
				auto prop_ptr = (void*)((uintptr_t)entity + offset);

				switch (cached_offsets[key].first->m_prop_type) {
					case sdk::e_send_prop_type::_int: s.push(*(int*)prop_ptr); break;
					case sdk::e_send_prop_type::_float: s.push(*(float*)prop_ptr); break;
					case sdk::e_send_prop_type::_vec_xy:
					case sdk::e_send_prop_type::_vec: {
						const auto v = *(vec3d*)prop_ptr;
						// printf("%f %f %f\n", v.x, v.y, v.z);
						s.push(v);
					} break;
					case sdk::e_send_prop_type::_string: s.push(std::string{ (char*)prop_ptr }); break;
					default: s.push(nullptr); break;
				}

				return 1;
			}

			LUA_FN(get) {
				LUA_STATE(l);

				auto index = s.get_arg_impl<int>(1);
				if (!index.has_value()) {
					s.push(nullptr);
					return 1;
				}

				auto entity = interfaces::entity_list->get_client_entity(*index);
				if (entity == nullptr) {
					s.push(nullptr);
					return 1;
				}

				s.push(table_t{
						{
								{ _UNIQUE_NAME("@@entity_pointer"), (int)entity },
								{ STR("get_prop"), get_prop },
						},
				});

				return 1;
			}

#define PLAYER_GETTER(fn_name)                                                                        \
	LUA_FN(fn_name) {                                                                                 \
		LUA_STATE(l);                                                                                 \
                                                                                                      \
		const auto entity = (sdk::cs_player_t*)s.get_field<int>(_UNIQUE_NAME("@@entity_pointer"), 1); \
		if (entity == nullptr) {                                                                      \
			s.push(nullptr);                                                                          \
			return 1;                                                                                 \
		}                                                                                             \
                                                                                                      \
		s.push(entity->fn_name());                                                                    \
		return 1;                                                                                     \
	}

			PLAYER_GETTER(origin);
			PLAYER_GETTER(velocity);
			PLAYER_GETTER(name);

			static void push_player(state_t& s, sdk::cs_player_t* player) {
				if (player == nullptr) {
					s.push(nullptr);
					return;
				}

				s.push(table_t{
						{
								{ _UNIQUE_NAME("@@entity_pointer"), (int)player },
								{ STR("get_prop"), get_prop },
								{ STR("get_origin"), origin },
								{ STR("get_velocity"), velocity },
								{ STR("get_name"), name },
								{ 0, (int)player },
						},
				});
			}

			LUA_FN(get_players) {
				LUA_STATE(l);

				s.new_table();

				const auto enemies_only = s.arg_or(false, 1);
				const auto include_dormant = s.arg_or(false, 2);

				const auto has_callback = s.is_function(3);

				int i = 0;
				for (auto& [player, entry]: entities->m_players) {
					if (enemies_only) {
						if (player->is_teammate() || player == globals->m_local)
							continue;
					}

					if (!include_dormant) {
						if (player->dormant())
							continue;
					}

					if (has_callback) {
						lua_pushvalue(l, 3);	// copy function to stack
						push_player(s, player); // push player as argument
						if (lua_pcall(l, 1, 1, 0) != 0) {
							perform_error(dformat(STRS("get_players(): callback error: {}"), lua_tostring(l, -1)));
							lua_pop(l, 1);
							return 0;
						}

						const auto result = s.get_arg_impl<bool>(-1);
						lua_pop(l, 1);

						if (result.has_value()) {
							if (!result.value())
								continue;
						}
					}

					s.push(++i);			// push the index
					push_player(s, player); // push the value

					lua_settable(l, -3); // set the key-value pair in the table (pops both the key and the value)
				}

				return 1;
			}

			LUA_FN(me) {
				LUA_STATE(l);
				push_player(s, globals->m_local);
				return 1;
			}

			LUA_FN(get_by_user_id) {
				LUA_STATE(l);
				const auto index = s.get_arg_impl<int>(1);
				if (!index.has_value()) {
					s.push(nullptr);
					return 1;
				}

				push_player(s, (sdk::cs_player_t*)interfaces::entity_list->get_client_entity(interfaces::engine->get_player_for_user_id(*index)));
				return 1;
			}

			LUA_FN(get_by_handle) {
				LUA_STATE(l);
				const auto handle = s.get_arg_impl<int>(1);
				if (!handle.has_value()) {
					s.push(nullptr);
					return 1;
				}

				push_player(s, (sdk::cs_player_t*)interfaces::entity_list->get_client_entity_handle(*handle));
				return 1;
			}
		} // namespace entity

		namespace logs {
			LUA_FN(info) {
				LUA_STATE(l);
				cheat_logs->add_info(s.arg<std::string>(1));
				return 0;
			}

			LUA_FN(error) {
				LUA_STATE(l);
				cheat_logs->add_error(s.arg<std::string>(1));
				return 0;
			}

			LUA_FN(buy) {
				LUA_STATE(l);
				cheat_logs->add_buy(s.arg<std::string>(1));
				return 0;
			}

			LUA_FN(hit) {
				LUA_STATE(l);
				cheat_logs->add_hit(s.arg<std::string>(1));
				return 0;
			}

			LUA_FN(miss) {
				LUA_STATE(l);
				cheat_logs->add_miss(s.arg<std::string>(1));
				return 0;
			}
		} // namespace logs

		namespace global_vars {
			LUA_GETTER(realtime, interfaces::global_vars->m_realtime);
			LUA_GETTER(framecount, interfaces::global_vars->m_framecount);
			LUA_GETTER(curtime, interfaces::global_vars->m_curtime);
			LUA_GETTER(frametime, interfaces::global_vars->m_frametime);
			LUA_GETTER(max_clients, interfaces::global_vars->m_max_clients);
			LUA_GETTER(tickcount, interfaces::global_vars->m_tickcount);
			LUA_GETTER(interval_per_tick, interfaces::global_vars->m_interval_per_tick);
			LUA_GETTER(interpolation_amount, interfaces::global_vars->m_interpolation_amount);
			LUA_GETTER(simticks_this_frame, interfaces::global_vars->m_simticks_this_frame);
		} // namespace global_vars

		namespace client {
			LUA_FN(choked_commands) {
				LUA_STATE(l);
				if (interfaces::client_state == nullptr) {
					s.push(nullptr);
					return 1;
				}

				s.push(interfaces::client_state->m_choked_commands);
				return 1;
			}
		} // namespace client

		namespace anti_aim {
			LUA_FN(can_work) {
				LUA_STATE(l);

				if (globals->m_cmd != nullptr && globals->m_cmd->m_command_number == interfaces::client_state->get_current_tick())
					s.push(hvh::antiaim->can_work(globals->m_cmd));
				else
					s.push(false);

				return 1;
			}

			LUA_FN(get_at_target_yaw) {
				LUA_STATE(l);

				auto yaw = hvh::antiaim->get_at_target_yaw(s.arg_or(false, 1));
				if (yaw.has_value())
					s.push(*yaw);
				else
					s.push(nullptr);

				return 1;
			}
		} // namespace anti_aim

		namespace math_ {
			LUA_FN(random_int) {
				LUA_STATE(l);
				s.push(math::random_int(s.arg<int>(1), s.arg<int>(2)));
				return 1;
			}

			LUA_FN(random_float) {
				LUA_STATE(l);
				s.push(math::random_float(s.arg<float>(1), s.arg<float>(2)));
				return 1;
			}
		} // namespace math_

		namespace cheat {
			LUA_FN(username) {
				LUA_STATE(l);
				s.push(network::username);
				return 1;
			}

			LUA_FN(build_date) {
				LUA_STATE(l);
				s.push(dformat(STRS("{} {}"), STRC(__DATE__), STRC(__TIME__)));
				return 1;
			}

			LUA_FN(print) {
				LUA_STATE(l);
				auto text = s.get_arg_impl<std::string>(1);
				if (text.has_value())
					game_console->print(*text);

				return 0;
			}
		} // namespace cheat

		namespace hotkeys_ {
			LUA_FN(hotkey_state) {
				LUA_STATE(l);
				if (!s.is_table(1)) {
					s.push(nullptr);
					return 1;
				}

				const auto hotkey = (hotkey_t*)s.get_field<int>(_UNIQUE_NAME("@@hotkey_pointer"), 1);
				if (hotkey != nullptr) {
					s.push(hotkey->m_active);
				} else
					s.push(nullptr);

				return 1;
			}

			LUA_FN(find) {
				LUA_STATE(l);
				const auto name = s.get_arg_impl<std::string>(1);
				if (!name.has_value()) {
					perform_error(STRS("hotkeys.find(): invalid argument"));
					return 1;
				}

				auto it = std::find_if(hotkeys->m_hotkeys.begin(), hotkeys->m_hotkeys.end(), [&name](hotkey_t* hotkey) {
					return hotkey->m_localize_name == *name;
				});

				if (it != hotkeys->m_hotkeys.end()) {
					s.push(table_t{
							{
									{ _UNIQUE_NAME("@@hotkey_pointer"), (int)*it },
									{ STR("state"), hotkey_state },
							},
					});
				} else
					s.push(nullptr);

				return 1;
			}
		} // namespace hotkeys_

		namespace script {
			LUA_FN(set_name) {
				LUA_STATE(l);
				auto name = s.get_arg_impl<std::string>(1);
				if (!name.has_value())
					return 0;

				scripts_storage[s.get_id()].m_name = *name;
				return 0;
			}

			LUA_FN(set_version) {
				LUA_STATE(l);
				auto version = s.get_arg_impl<std::string>(1);
				if (!version.has_value())
					return 0;

				auto parsed = version_t::parse(*version);
				if (!parsed.empty())
					scripts_storage[s.get_id()].m_version = *version;

				return 0;
			}

			static void for_each_entry(script_storage_t& storage, std::function<void(const std::string&, std::variant<bool, int>)> callback) {
				for (auto& [key, value]: storage.m_bools)
					callback(key, value);

				for (auto& [key, value]: storage.m_ints)
					callback(key, value);

				for (auto& [key, value]: storage.m_colors)
					callback(key, (int)value.get().u32());
			}

			static void crypt_xor(uint64_t key, utils::bytes_t& bytes) {
				for (size_t i = 0; i < bytes.size(); ++i) {
					const auto current_shift = (i % sizeof(key)) << 3;
					bytes[i] ^= (key & ((uint64_t)0xFF << current_shift)) >> current_shift;
				}
			}

			static void crypt_xor(const std::string& config_name, utils::bytes_t& bytes) {
				return crypt_xor((uint64_t)fnva1(config_name.c_str()) | 0xDEAD133700000000, bytes);
			}

			static void save_impl(const std::string& script_name, const std::string& config_name, const json_t& j) {
				const auto folder = dformat(STRS("weave/settings/{}"), script_name);
				const auto path = folder + STRS("/") + config_name;

				create_dir_if_not_exists(folder);

				const auto result = j.dump(4);
				utils::bytes_t buf = { result.begin(), result.end() };
				crypt_xor(script_name, buf);
				utils::write_file_bin(path, buf);
			}

			template<typename T, typename U>
			static bool load_var(const std::string& key, std::list<script_storage_impl_t<U>>& vars, const T& value) {
				auto var = std::find_if(vars.begin(), vars.end(), [&key](const auto& kv) { return kv.first == key; });

				if (var == vars.end())
					return false;

				var->second = U(value);
				return true;
			}

			static void load_impl(script_storage_t& storage, const json_t& j) {
				const auto& config = j[STRS("config")];
				for (auto it = config.begin(); it != config.end(); ++it) {
					const auto& key = it.key();

					switch (it->type()) {
						case json_t::value_t::boolean:
							if (!load_var<bool>(key, storage.m_bools, it->get<bool>()))
								perform_error(dformat(STRS("Warning: unknown var '{}'"), key));
							break;
						case json_t::value_t::number_unsigned:
						case json_t::value_t::number_integer: {
							if (!load_var<int>(key, storage.m_ints, it->get<int>())) {
								if (!load_var<int>(key, storage.m_colors, it->get<int>()))
									perform_error(dformat(STRS("Warning: unknown var '{}'"), key));
							}
						} break;
					}
				}
			}

			LUA_FN(save_all) {
				LUA_STATE(l);

				if (!scripts_storage.contains(s.get_id()))
					return s.return_values(STRS("unknown error"));

				const auto config_name = s.get_arg_impl<std::string>(1);
				if (!config_name.has_value() || config_name->empty())
					return s.return_values(STRS("invalid arguments"));

				auto& storage = scripts_storage[s.get_id()];

				json_t j{};
				j[STRS("version")] = storage.m_version.get();

				for_each_entry(storage, [&j](const std::string& key, const auto& value) {
					std::visit([&j, &key](const auto& v) { j[STRS("config")][key] = v; }, value);
				});

				save_impl(storage.m_name, *config_name, j);
				return s.return_values(nullptr);
			}

			LUA_FN(save) {
				LUA_STATE(l);

				const auto config_name = s.get_arg_impl<std::string>(1);
				if (!config_name.has_value() || config_name->empty())
					return s.return_values(STRS("invalid arguments"));

				std::vector<std::pair<int, int>> items{};
				for (int i = 2, top = lua_gettop(l); i <= top; ++i) {
					auto var_pointer = s.get_field<int>(_UNIQUE_NAME("@@var_pointer"), i);
					auto var_type = s.get_field<int>(_UNIQUE_NAME("@@item_type"), i);
					if (var_pointer != 0)
						items.emplace_back(var_pointer, var_type);
				}

				if (items.empty())
					return s.return_values(STRS("invalid items"));

				auto& storage = scripts_storage[s.get_id()];

				json_t j{};
				j[STRS("version")] = storage.m_version.get();

				for (auto& [var, type]: items) {
					switch (type) {
						case ui::item_checkbox: {
							const auto var_pointer = (script_storage_impl_t<incheat_vars::types::bool_t>*)var;
							j[STRS("config")][var_pointer->first] = (bool)var_pointer->second;
						} break;

						case ui::item_slider:
						case ui::item_dropbox: {
							const auto var_pointer = (script_storage_impl_t<incheat_vars::types::int_t>*)var;
							j[STRS("config")][var_pointer->first] = var_pointer->second;
						} break;

						case ui::item_colorpicker: {
							const auto var_pointer = (script_storage_impl_t<incheat_vars::types::color_t>*)var;
							j[STRS("config")][var_pointer->first] = (int)var_pointer->second.get().u32();
						} break;
					}
				}

				save_impl(storage.m_name, *config_name, j);
				return s.return_values(nullptr);
			}

			LUA_FN(dump_base64) {
				LUA_STATE(l);

				std::vector<std::pair<int, int>> items{};
				for (int i = 1, top = lua_gettop(l); i <= top; ++i) {
					auto var_pointer = s.get_field<int>(_UNIQUE_NAME("@@var_pointer"), i);
					auto var_type = s.get_field<int>(_UNIQUE_NAME("@@item_type"), i);
					if (var_pointer != 0)
						items.emplace_back(var_pointer, var_type);
				}

				if (items.empty())
					return s.return_values(STRS("invalid items"));

				auto& storage = scripts_storage[s.get_id()];

				json_t j{};
				j[STRS("version")] = storage.m_version.get();

				for (auto& [var, type]: items) {
					switch (type) {
						case ui::item_checkbox: {
							const auto var_pointer = (script_storage_impl_t<incheat_vars::types::bool_t>*)var;
							j[STRS("config")][var_pointer->first] = (bool)var_pointer->second;
						} break;

						case ui::item_slider:
						case ui::item_dropbox: {
							const auto var_pointer = (script_storage_impl_t<incheat_vars::types::int_t>*)var;
							j[STRS("config")][var_pointer->first] = var_pointer->second;
						} break;

						case ui::item_colorpicker: {
							const auto var_pointer = (script_storage_impl_t<incheat_vars::types::color_t>*)var;
							j[STRS("config")][var_pointer->first] = (int)var_pointer->second.get().u32();
						} break;
					}
				}

				const auto result = j.dump(4);
				utils::bytes_t buf = { result.begin(), result.end() };
				crypt_xor(storage.m_name, buf);
				return /*s.return_values(nullptr, base64::encode(buf))*/0;
			}

			LUA_FN(dump_all_base64) {
				LUA_STATE(l);

				auto& storage = scripts_storage[s.get_id()];

				json_t j{};
				j[STRS("version")] = storage.m_version.get();

				for_each_entry(storage, [&j](const std::string& key, const auto& value) {
					std::visit([&j, &key](const auto& v) { j[STRS("config")][key] = v; }, value);
				});

				const auto result = j.dump(4);
				utils::bytes_t buf = { result.begin(), result.end() };
				crypt_xor(storage.m_name, buf);
				return /*s.return_values(nullptr, base64::encode(buf))*/0;
			}

			LUA_FN(load_all) {
				LUA_STATE(l);

				const auto config_name = s.get_arg_impl<std::string>(1);
				if (!config_name.has_value() || config_name->empty())
					return s.return_values(STRS("invalid arguments"));

				auto& storage = scripts_storage[s.get_id()];

				const auto folder = dformat(STRS("weave/settings/{}"), storage.m_name);
				const auto path = folder + STRS("/") + *config_name;

				if (!fs::exists(path))
					return s.return_values(STRS("config does not exists"));

				auto buf = utils::read_file_bin(path);
				crypt_xor(storage.m_name, buf);

				try {
					const auto j = json_t::parse(buf);
					if (!j.contains(STRS("version")))
						return s.return_values(STRS("invalid config"));

					std::string version = j[STRS("version")].get<std::string>();
					version.erase(std::remove_if(version.begin(), version.end(), [](char c) { return !(std::isdigit(c) || c == '.'); }), version.end());
					if (!storage.m_version.is_backward_compatibility({ version }))
						return s.return_values(STRS("incompatible version"));

					load_impl(storage, j);
				} catch (std::exception& e) {
					perform_error(dformat(STRS("script.load_all(): error parsing json: {}"), e.what()));
				}

				return s.return_values(nullptr);
			}

			LUA_FN(load) {
				LUA_STATE(l);

				const auto config_name = s.get_arg_impl<std::string>(1);
				if (!config_name.has_value() || config_name->empty())
					return s.return_values(STRS("invalid version"));

				std::vector<std::pair<int, int>> items{};
				for (int i = 2, top = lua_gettop(l); i <= top; ++i) {
					auto var_pointer = s.get_field<int>(_UNIQUE_NAME("@@var_pointer"), i);
					auto var_type = s.get_field<int>(_UNIQUE_NAME("@@item_type"), i);
					if (var_pointer != 0)
						items.emplace_back(var_pointer, var_type);
				}

				if (items.empty())
					return s.return_values(STRS("invalid items"));

				auto& storage = scripts_storage[s.get_id()];

				const auto folder = dformat(STRS("weave/settings/{}"), storage.m_name);
				const auto path = folder + STRS("/") + *config_name;

				if (!fs::exists(path))
					return s.return_values(STRS("config does not exists"));

				auto buf = utils::read_file_bin(path);
				crypt_xor(storage.m_name, buf);

				try {
					const auto j = json_t::parse(buf);
					if (!j.contains(STRS("version")))
						return s.return_values(STRS("invalid config"));

					std::string version = j[STRS("version")].get<std::string>();
					version.erase(std::remove_if(version.begin(), version.end(), [](char c) { return !(std::isdigit(c) || c == '.'); }), version.end());
					if (!storage.m_version.is_backward_compatibility({ version }))
						return s.return_values(STRS("incompatible version"));

					const auto& config = j[STRS("config")];
					for (auto& [var, type]: items) {
						switch (type) {
							case ui::item_checkbox: {
								const auto var_pointer = (script_storage_impl_t<incheat_vars::types::bool_t>*)var;
								if (config.contains(var_pointer->first))
									var_pointer->second = config[var_pointer->first].get<bool>();
								else
									perform_error(dformat(STRS("Warning: '{}' is not exist in config"), var_pointer->first));
							} break;

							case ui::item_slider:
							case ui::item_dropbox: {
								const auto var_pointer = (script_storage_impl_t<incheat_vars::types::int_t>*)var;
								if (config.contains(var_pointer->first))
									var_pointer->second = config[var_pointer->first].get<int>();
								else
									perform_error(dformat(STRS("Warning: '{}' is not exist in config"), var_pointer->first));
							} break;

							case ui::item_colorpicker: {
								const auto var_pointer = (script_storage_impl_t<incheat_vars::types::color_t>*)var;
								if (config.contains(var_pointer->first))
									var_pointer->second.get().u32() = config[var_pointer->first].get<int>();
								else
									perform_error(dformat(STRS("Warning: '{}' is not exist in config"), var_pointer->first));
							} break;
						}
					}

				} catch (std::exception& e) {
					perform_error(dformat(STRS("script.load(): error parsing json: {}"), e.what()));
				}

				return s.return_values(nullptr);
			}

			LUA_FN(load_base64) {
				LUA_STATE(l);

				const auto text = s.get_arg_impl<std::string>(1);
				if (!text.has_value() || text->empty())
					return s.return_values(STRS("invalid arguments"));

				std::vector<std::pair<int, int>> items{};
				for (int i = 1, top = lua_gettop(l); i <= top; ++i) {
					auto var_pointer = s.get_field<int>(_UNIQUE_NAME("@@var_pointer"), i);
					auto var_type = s.get_field<int>(_UNIQUE_NAME("@@item_type"), i);
					if (var_pointer != 0)
						items.emplace_back(var_pointer, var_type);
				}

				if (items.empty())
					return s.return_values(STRS("invalid items"));

				auto& storage = scripts_storage[s.get_id()];

				auto buf = std::vector<uint8_t>{} /*base64::decode(*text)*/;
				crypt_xor(storage.m_name, buf);

				try {
					const auto j = json_t::parse(buf);
					if (!j.contains(STRS("version")))
						return s.return_values(STRS("invalid config"));

					std::string version = j[STRS("version")].get<std::string>();
					version.erase(std::remove_if(version.begin(), version.end(), [](char c) { return !(std::isdigit(c) || c == '.'); }), version.end());
					if (!storage.m_version.is_backward_compatibility({ version }))
						return s.return_values(STRS("incompatible version"));

					const auto& config = j[STRS("config")];
					for (auto& [var, type]: items) {
						switch (type) {
							case ui::item_checkbox: {
								const auto var_pointer = (script_storage_impl_t<incheat_vars::types::bool_t>*)var;
								if (config.contains(var_pointer->first))
									var_pointer->second = config[var_pointer->first].get<bool>();
								else
									perform_error(dformat(STRS("Warning: '{}' is not exist in config"), var_pointer->first));
							} break;

							case ui::item_slider:
							case ui::item_dropbox: {
								const auto var_pointer = (script_storage_impl_t<incheat_vars::types::int_t>*)var;
								if (config.contains(var_pointer->first))
									var_pointer->second = config[var_pointer->first].get<int>();
								else
									perform_error(dformat(STRS("Warning: '{}' is not exist in config"), var_pointer->first));
							} break;

							case ui::item_colorpicker: {
								const auto var_pointer = (script_storage_impl_t<incheat_vars::types::color_t>*)var;
								if (config.contains(var_pointer->first))
									var_pointer->second.get().u32() = config[var_pointer->first].get<int>();
								else
									perform_error(dformat(STRS("Warning: '{}' is not exist in config"), var_pointer->first));
							} break;
						}
					}

				} catch (std::exception& e) {
					perform_error(dformat(STRS("script.load(): error parsing json: {}"), e.what()));
				}

				return s.return_values(nullptr);
			}

			LUA_FN(load_all_base64) {
				LUA_STATE(l);

				const auto text = s.get_arg_impl<std::string>(1);
				if (!text.has_value() || text->empty())
					return s.return_values(STRS("invalid arguments"));

				auto& storage = scripts_storage[s.get_id()];

				auto buf = std::vector<uint8_t>{} /*base64::decode(*text)*/;
				crypt_xor(storage.m_name, buf);

				try {
					const auto j = json_t::parse(buf);
					if (!j.contains(STRS("version")))
						return s.return_values(STRS("invalid config"));

					std::string version = j[STRS("version")].get<std::string>();
					version.erase(std::remove_if(version.begin(), version.end(), [](char c) { return !(std::isdigit(c) || c == '.'); }), version.end());
					if (!storage.m_version.is_backward_compatibility({ version }))
						return s.return_values(STRS("incompatible version"));

					load_impl(storage, j);
				} catch (std::exception& e) {
					return s.return_values(dformat(STRS("error parsing json: {}"), e.what()));
				}

				return s.return_values(nullptr);
			}
		} // namespace script

		LUA_FN(pattern_scan) {
			LUA_STATE(l);

			const auto modname = s.get_arg_impl<std::string>(1);
			const auto signature = s.get_arg_impl<std::string>(2);

			if (!modname.has_value() || !signature.has_value())
				return s.return_values(nullptr);

			return s.return_values(utils::find_pattern(*modname, *signature));
		}

		LUA_FN(find_interface) {
			LUA_STATE(l);

			const auto modname = s.get_arg_impl<std::string>(1);
			const auto interface_name = s.get_arg_impl<std::string>(2);

			if (!opt_has_value(modname, interface_name))
				return s.return_values(nullptr);

			return s.return_values(utils::find_interface(modname->c_str(), interface_name->c_str()));
		}

		static void init(state_t& s) {
			using namespace api;

			s.bind_global_table(
					STRS("ui"),
					{
							{ STRS("tab"), ui::create_tab },
							{ STRS("group"), ui::create_group },
							{ STRS("register_tab"), ui::register_tab },
							{ STRS("update"), ui::update },
							{ STRS("checkbox"), ui::checkbox::create },
							{ STRS("slider"), ui::slider::create },
							{ STRS("dropbox"), ui::dropbox::create },
							{ STRS("color_picker"), ui::color_picker::create },
							{ STRS("text"), ui::create_text },
							{ STRS("button"), ui::button::create },
							{ STRS("get_icon"), ui::get_icon },
							{ STRS("get_accent"), ui::get_accent },
							{ STRS("override_accent"), ui::override_accent },
							{ STRS("get_menu_position"), ui::get_menu_position },
							{ STRS("get_menu_size"), ui::get_menu_size },
							{ STRS("set_menu_position"), ui::set_menu_position },
							{ STRS("get_dpi_scale"), ui::get_dpi_scale },
							{ STRS("get_mouse_position"), ui::get_mouse_position },
							{ STRS("get_mouse_wheel"), ui::get_mouse_wheel },
							{ STRS("is_mouse_pressed"), ui::is_mouse_pressed },
							{ STRS("get_menu_alpha"), ui::get_menu_alpha },
					});

			s.bind_global_table(
					STRS("render"),
					{
							{ STRS("filled_rect"), render_::filled_rect },
							{ STRS("rect"), render_::rect },
							{ STRS("filled_circle"), render_::filled_circle },
							{ STRS("circle"), render_::circle },
							{ STRS("line"), render_::line },
							{ STRS("vertical_gradient"), render_::vertical_gradient },
							{ STRS("horizontal_gradient"), render_::horizontal_gradient },
							{ STRS("arc"), render_::arc },
							{ STRS("filled_arc"), render_::filled_arc },

							{ STRS("create_font"), render_::create_font },
							{ STRS("create_texture"), render_::create_texture },
							{ STRS("text"), render_::text },
							{ STRS("image"), render_::image },
							{ STRS("get_screen_size"), render_::get_screen_size },
							{ STRS("world_to_screen"), render_::world_to_screen },
							{ STRS("text_size"), render_::text_size },
					});

			s.bind_global_table(
					STRS("vars"),
					{
							{ STRS("find"), vars::find },
					});

			s.bind_global_table(
					STRS("convar"),
					{
							{ STRS("find"), cvars::find },
					});

			s.bind_global_table(
					STRS("entity"),
					{
							{ STRS("get"), entity::get },
							{ STRS("me"), entity::me },
							{ STRS("get_players"), entity::get_players },
							{ STRS("get_by_user_id"), entity::get_by_user_id },
							{ STRS("get_by_handle"), entity::get_by_handle },
					});

			s.bind_global_table(
					STRS("logs"),
					{
							{ STRS("@@buy"), logs::buy },
							{ STRS("@@error"), logs::error },
							{ STRS("@@hit"), logs::hit },
							{ STRS("@@info"), logs::info },
							{ STRS("@@miss"), logs::miss },
					});

			s.bind_global_table(
					STRS("global_vars"),
					{
							{ STRS("realtime"), global_vars::realtime },
							{ STRS("framecount"), global_vars::framecount },
							{ STRS("curtime"), global_vars::curtime },
							{ STRS("frametime"), global_vars::frametime },
							{ STRS("max_clients"), global_vars::max_clients },
							{ STRS("tickcount"), global_vars::tickcount },
							{ STRS("interval_per_tick"), global_vars::interval_per_tick },
							{ STRS("interpolation_amount"), global_vars::interpolation_amount },
							{ STRS("simticks_this_frame"), global_vars::simticks_this_frame },
					});

			s.bind_global_table(
					STRS("client"),
					{
							{ STRS("choked_commands"), client::choked_commands },
					});

			s.bind_global_table(
					STRS("anti_aim"),
					{
							{ STRS("can_work"), anti_aim::can_work },
							{ STRS("get_at_target_yaw"), anti_aim::get_at_target_yaw },
					});

			s.bind_global_table(
					STRS("cheat"),
					{
							{ STRS("username"), cheat::username },
							{ STRS("build_date"), cheat::build_date },
							{ STRS("print"), cheat::print },
					});

			s.bind_global_table(
					STRS("script"),
					{
							{ STRS("save_all"), script::save_all },
							{ STRS("save"), script::save },
							{ STRS("dump_base64"), script::dump_base64 },
							{ STRS("dump_all_base64"), script::dump_all_base64 },
							{ STRS("load_all"), script::load_all },
							{ STRS("load"), script::load },
							{ STRS("load_base64"), script::load_base64 },
							{ STRS("load_all_base64"), script::load_all_base64 },
							{ STRS("set_name"), script::set_name },
							{ STRS("set_version"), script::set_version },
					});

			s.bind_global_table(
					STRS("hotkeys"),
					{
							{ STRS("find"), hotkeys_::find },
					});

			s.bind_global_function(STRS("pattern_scan"), pattern_scan);
			s.bind_global_function(STRS("find_interface"), find_interface);

			{
				s.bind_global_function(STRS("_random_float"), math_::random_float);
				s.bind_global_function(STRS("_random_int"), math_::random_int);

				s.eval(STRS("math.random_float = _random_float"));
				s.eval(STRS("math.random_int = _random_int"));
			}
		}
	} // namespace api

	void config_callback(const std::string& event_name, const network::config_t& config) {
		callback(event_name, table_t{
									 {
											 { STR("id"), config.m_id },
											 { STR("name"), config.m_name },
											 { STR("author"), config.m_author },
											 { STR("last_update_by"), config.m_last_update_by },
											 { STR("private"), (bool)config.m_private },
									 } });
	}

	void game_event(const std::string& name, sdk::game_event_t* e) {
		using namespace api;

		callback(name, table_t{
							   {
									   { STR("@@event_pointer"), (int)e },
									   { STR("get_name"), events::get_name },
									   { STR("get_int"), events::get_int },
									   { STR("get_float"), events::get_float },
									   { STR("get_string"), events::get_string },
									   { STR("set_int"), events::set_int },
									   { STR("set_float"), events::set_float },
									   { STR("set_string"), events::set_string },
							   } });
	}

	void user_cmd_callback(const std::string& name, sdk::user_cmd_t* cmd) {
		using namespace api;

		auto read_cmd_int = [](state_t& s, const std::string& name, int index) -> int {
			lua_pushstring(*s, name.c_str());
			lua_gettable(*s, index);
			const auto value = lua_tointeger(*s, -1);
			lua_pop(*s, 1);
			return value;
		};

		auto read_cmd_bool = [](state_t& s, const std::string& name, int index) -> bool {
			lua_pushstring(*s, name.c_str());
			lua_gettable(*s, index);
			const auto value = lua_toboolean(*s, -1);
			lua_pop(*s, 1);
			return value;
		};

		auto read_cmd_float = [](state_t& s, const std::string& name, int index) -> float {
			lua_pushstring(*s, name.c_str());
			lua_gettable(*s, index);
			const auto value = lua_tonumber(*s, -1);
			lua_pop(*s, 1);
			return (float)value;
		};

		auto read_cmd_vec3d = [](state_t& s, const std::string& name, int index) -> vec3d {
			lua_pushstring(*s, name.c_str());
			lua_gettable(*s, index);
			const auto value = s.arg<vec3d>(-1);
			lua_pop(*s, 1);
			return value;
		};

		auto restore_cmd = [&](state_t& s) {
			cmd->m_viewangles = read_cmd_vec3d(s, STR("view_angles"), 1);
			cmd->m_buttons = read_cmd_int(s, STRC("buttons"), 1);

			const auto move = read_cmd_vec3d(s, STR("move"), 1);
			cmd->m_forwardmove = move.x;
			cmd->m_sidemove = move.y;
			cmd->m_upmove = move.z;

			*globals->m_send_packet = read_cmd_bool(s, STR("send_packet"), 1);
		};

		callback(name, table_t{ {
							   { STR("command_number"), cmd->m_command_number },
							   { STR("tick_count"), cmd->m_tickcount },
							   { STR("view_angles"), cmd->m_viewangles },
							   { STR("aim_direction"), cmd->m_aim_direction },
							   { STR("move"), vec3d{ cmd->m_forwardmove, cmd->m_sidemove, cmd->m_upmove } },
							   { STR("buttons"), (int)cmd->m_buttons },
							   { STR("send_packet"), *globals->m_send_packet },
					   } },
				 restore_cmd);
	}

	LUA_FN(vector_to_string) {
		LUA_STATE(l);
		const auto v = s.get_arg_impl<vec3d>(1);
		if (!v.has_value()) {
			auto v2 = s.get_arg_impl<vec2d>(1);
			if (v2.has_value()) {
				s.push(std::format("vector({}, {})", v2->x, v2->y));
				return 1;
			}

			s.push(nullptr);
			return 1;
		}

		s.push(std::format("vector({}, {}, {})", v->x, v->y, v->z));
		return 1;
	}

	LUA_FN(color_to_string) {
		LUA_STATE(l);
		const auto color = s.arg<color_t>(1);
		s.push(std::format("color({}, {}, {}, {})", color.r(), color.g(), color.b(), color.a()));
		return 1;
	}

	static void pre_init(state_t& s) {
		s.bind_global_function(STRS("@@vector_to_string"), vector_to_string);
		s.bind_global_function(STRS("@@color_to_string"), color_to_string);

		// vector
		s.eval(STRS(R"(
Vector = {}
Vector.__add = function(a, b) return Vector:new{x = a.x + b.x, y = a.y + b.y, z = a.z + b.z, w = a.w + b.w} end
Vector.__sub = function(a, b) return Vector:new{x = a.x - b.x, y = a.y - b.y, z = a.z - b.z, w = a.w - b.w} end
Vector.__mul = function(a, b) return Vector:new{x = a.x * b.x, y = a.y * b.y, z = a.z * b.z, w = a.w * b.w} end
Vector.__div = function(a, b) return Vector:new{x = a.x / b.x, y = a.y / b.y, z = a.z / b.z, w = a.w / b.w} end
Vector.__tostring = _G['@@vector_to_string']
function Vector:new(obj) self.__index = self return setmetatable(obj or {}, self) end
function vector(x, y, z, w)
	if x == nil then x = 0 end
	if y == nil then y = x end
	if z == nil then z = 0 end
	return Vector:new{x = x, y = y, z = z, w = w}
end
)"));

		// color
		s.eval(STRS(R"(
Color = {}
Color.__tostring = _G['@@color_to_string']
function Color:new(obj) self.__index = self return setmetatable(obj or {}, self) end
function color(r, g, b, a) 
    if r == nil then r = 255 end
    if g == nil then g = r end
    if b == nil then b = r end
    if a == nil then a = 255 end
    return Color:new{r = r, g = g, b = b, a = a}
end
)"));

		// fmt
		s.eval(STRS(R"(
_G['@@fmt'] = function(...)
    local args = {...}
    local str = ''
    for i, v in ipairs(args) do
        if v == nil then
            str = str .. 'nil'
		else
            str = str .. tostring(v)
        end
        if i ~= #args then
            str = str .. ' '
        end
    end

	return str
end
)"));

		// print
		s.eval(STRS("print = function(...) cheat.print(_G['@@fmt'](...) .. '\\n') end"));
	}

	static void post_init(state_t& s) {
		s.eval(STRS("logs.info = function(...) logs['@@info'](_G['@@fmt'](...)) end"));
		s.eval(STRS("logs.buy = function(...) logs['@@buy'](_G['@@fmt'](...)) end"));
		s.eval(STRS("logs.hit = function(...) logs['@@hit'](_G['@@fmt'](...)) end"));
		s.eval(STRS("logs.miss = function(...) logs['@@miss'](_G['@@fmt'](...)) end"));
		s.eval(STRS("logs.error = function(...) logs['@@error'](_G['@@fmt'](...)) end"));
	}

	void init_state(state_t& s) {
		pre_init(s);
		api::init(s);
		post_init(s);
	}
} // namespace lua