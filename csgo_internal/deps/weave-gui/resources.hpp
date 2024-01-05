#pragma once
#include "utils/macros.hpp"
#include <atomic>
#include <d3dx9.h>
#include <string>

namespace NS_GUI {
	class resources {
	private:
		static inline std::unordered_map<uint32_t, void*> textures;
		static inline std::vector<std::string> files_to_create;

		static inline std::vector<std::string> fonts_to_download;

	public:
		static inline std::atomic<bool> downloaded = false;

		static __forceinline void* get_texture(uint32_t hash) {
			if (!textures.contains(hash))
				return nullptr;

			return textures.at(hash);
		}

		static void add_pngs(const std::vector<std::string>& items) { files_to_create.insert(files_to_create.end(), items.begin(), items.end()); }
		static void add_fonts(const std::vector<std::string>& items) { fonts_to_download.insert(fonts_to_download.end(), items.begin(), items.end()); }

		static inline void add_font(const std::string& name) { fonts_to_download.emplace_back(name); }
		static inline void add_png(const std::string& name) { files_to_create.emplace_back(name); }
		static void download();
		static void create_textures(IDirect3DDevice9* device, std::function<void()> callback);
	};
}// namespace NS_GUI