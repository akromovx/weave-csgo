#include "script.hpp"

#include "../features/visuals/logs.hpp"
#include "../features/menu.hpp"

#include "../game/events.hpp"

#include "../../deps/imgui/imgui_freetype.h"

#include "../render.hpp"

namespace lua {
	static int register_callback(lua_State* l) {
		// Ensure we have two arguments: a string and a function
		if (lua_gettop(l) != 2 || !lua_isstring(l, 1) || !lua_isfunction(l, 2)) {
			perform_error(STRS("register_callback(): expected a string and a function as arguments"));
			return 0;
		}

		const char* name = lua_tostring(l, 1);

		// Access the registry __callbacks table
		lua_getfield(l, LUA_REGISTRYINDEX, STRC("__callbacks"));

		// Check if the callback table for the given name exists
		lua_pushstring(l, name);
		lua_gettable(l, -2);

		if (lua_isnil(l, -1)) {
			// Create a new table if the callback table doesn't exist
			lua_pop(l, 1);

			lua_newtable(l);
			lua_pushstring(l, name);
			lua_pushvalue(l, -2);
			lua_settable(l, -4);
		}

		// Append the function to the callback table
		lua_pushvalue(l, 2); // Push the function again
		lua_rawseti(l, -2, lua_objlen(l, -2) + 1);

		// Cleanup the Lua stack
		lua_pop(l, 2);
		return 0;
	}

	// This function is similar to register_callback but is designed to work as a closure
	static int register_callback_closure(lua_State* l) {
		const char* name = lua_tostring(l, lua_upvalueindex(1)); // Retrieve 'callback_name' from upvalue
		if (lua_isfunction(l, 1)) {
			lua_getfield(l, LUA_REGISTRYINDEX, STRSC("register_callback"));
			lua_pushstring(l, name);
			lua_pushvalue(l, 1);
			if (lua_pcall(l, 2, 0, 0) != 0) {
				const char* error = lua_tostring(l, -1);
				perform_error(dformat(STRS("error setting callback: {}"), error), true);
				lua_pop(l, 1);
			}
		}
		return 0;
	}

	static int meta_index(lua_State* l) {
		const char* name = lua_tostring(l, 2); // This is the 'callback_name'

		// Return a function (closure) that captures the 'callback_name'
		// and takes a function as an argument to register it
		lua_pushstring(l, name);
		lua_pushcclosure(l, register_callback_closure, 1); // 1 upvalue: the 'callback_name'
		return 1;
	}

	static int call_callbacks(lua_State* l, const std::string& name,
							  const std::function<void(lua_State*)>& build_arg = nullptr,
							  const std::function<void(state_t&)>& after_call = nullptr) {
		// Build argument and retrieve it from the Lua stack
		if (build_arg != nullptr)
			build_arg(l);

		int arg = lua_gettop(l);

		// Access the registry __callbacks table
		lua_getfield(l, LUA_REGISTRYINDEX, STRC("__callbacks"));

		// Check if the callback table for the given name exists
		lua_pushstring(l, name.c_str());
		lua_gettable(l, -2);

		if (!lua_istable(l, -1)) {
			// The callback table doesn't exist, so return an error code
			lua_pop(l, arg != 0 ? 3 : 2);
			return -1;
		}

		// Get the length of the callback table
		int len = lua_objlen(l, -1);

		// Iterate over the callbacks and call each one
		for (int i = 1; i <= len; i++) {
			// Get the callback function
			lua_rawgeti(l, -1, i);

			if (arg != 0)
				lua_pushvalue(l, arg);

			try {
				// Call the callback function with no arguments
				if (lua_pcall(l, arg != 0 ? 1 : 0, 0, 0) != 0) {
					// An error occurred while calling the callback, handle it here
					const char* error = lua_tostring(l, -1);
					// Handle the error, print it, or log it, etc.
					perform_error(dformat(STRS("LuaJIT(1) error calling callback: {}"), error), true);
					lua_pop(l, 1); // Pop the error message from the stack
				}
			} catch (std::exception& e) {
				cheat_logs->add_error(dformat(STRS("LuaJIT(2) error calling callback: {}"), e.what()));
			} catch (...) {
				cheat_logs->add_error(dformat(STRS("LuaJIT(3) error calling callback")));
			}

			if (after_call != nullptr) {
				state_t s{ l };
				after_call(s);
			}
		}

		// Clean up the Lua stack
		lua_pop(l, arg != 0 ? 3 : 2);

		// Return 0 to indicate success
		return 0;
	}

	bool script_t::dx9_create_font_textures() {
		unsigned char* pixels;
		int width, height;
		m_atlas->GetTexDataAsRGBA32(&pixels, &width, &height);

		// Upload texture to graphics system
		m_font_texture = nullptr;
		if (render::device->CreateTexture(width, height, 1, D3DUSAGE_DYNAMIC, D3DFMT_A8R8G8B8, D3DPOOL_DEFAULT, &m_font_texture, nullptr) < 0)
			return false;

		D3DLOCKED_RECT tex_locked_rect{};
		if (m_font_texture->LockRect(0, &tex_locked_rect, nullptr, D3DLOCK_DISCARD) != D3D_OK)
			return false;

		for (int y = 0; y < height; y++)
			memcpy((unsigned char*)tex_locked_rect.pBits + tex_locked_rect.Pitch * y, pixels + (width * 4) * y, (width * 4)); // Adjusted the pixel size

		m_font_texture->UnlockRect(0);

		// Store our identifier
		m_atlas->TexID = (ImTextureID)m_font_texture;
		return true;
	}

	static std::mutex script_load_mutex{};

	bool lua::script_t::load(const std::string& id, const std::string& encoded) {
		m_id = id;
		THREAD_SAFE(script_load_mutex);

		std::filesystem::path cwd = std::filesystem::current_path();
		std::string final_path{};

		for (char c: cwd.string()) {
			final_path += (c == '\\') ? "\\\\" : std::string(1, c);
		}

		std::string new_package_path = final_path.append(STR("\\\\weave\\\\lua\\\\lib\\\\?.lua;"));
		m_state->eval(STR("package.path = \"") + new_package_path + "\";");

		lua_newtable(*m_state);
		lua_setfield(*m_state, LUA_REGISTRYINDEX, ("__callbacks"));

		lua_pushcfunction(*m_state, register_callback);
		lua_setfield(*m_state, LUA_REGISTRYINDEX, ("register_callback"));

		m_state->bind_global_function(STRS("register_callback"), register_callback);

		m_state->bind_global_table(STRS("callback"),
								   {
										   { STRS("new"), register_callback },
								   },
								   meta_index);

		init_state(*m_state);
		m_atlas = new ImFontAtlas{};
		if (!m_state->eval_base64(encoded)) {
#ifdef _DEBUG
			printf("failed to load bytecode: %s\n", encoded.c_str());
#endif
			perform_error(STRS("failed to load bytecode"), true);
			return false;
		}

		auto& storage = scripts_storage[m_state->get_id()];

		if (!storage.m_font_queue.empty()) {
			for (auto& font: storage.m_font_queue) {
				if (*font.m_pointer != nullptr)
					continue;

				ImFontConfig cfg;
				cfg.FontBuilderFlags = font.m_flags;
				if (!fs::exists(font.m_path)) {
					perform_error(STRS("font file doesn't exist"), true);
					return false;
				}

				*font.m_pointer = m_atlas->AddFontFromFileTTF(font.m_path.c_str(), font.m_size, &cfg, m_atlas->GetGlyphRangesCyrillic());
			}

			if (!m_atlas->Fonts.empty()) {
				ImGuiFreeType::BuildFontAtlas(m_atlas);

				if (!dx9_create_font_textures()) {
					perform_error(STRS("failed to create font textures"), true);
					return false;
				}
			}
		}

		if (!storage.m_texture_queue.empty()) {
			for (auto& texture: storage.m_texture_queue) {
				if (*texture.m_pointer != nullptr)
					continue;

				if (fs::exists(texture.m_path))
					*texture.m_pointer = gui::create_gif_from_buffer(utils::read_file_bin(texture.m_path)).get();
			}
		}

		return true;
	}

	void script_t::callback(const std::string& name, const table_t& args, std::function<void(state_t&)> callback) {
		THREAD_SAFE(m_mutex);

		call_callbacks(
				*m_state, name,
				[&](auto* l) {
					LUA_STATE(l);
					s.push(args);
				},
				callback);

		auto stack = lua_gettop(*m_state);
		if (stack != 0)
			MessageBoxA(0, "stack error", 0, 0);
	}
} // namespace lua