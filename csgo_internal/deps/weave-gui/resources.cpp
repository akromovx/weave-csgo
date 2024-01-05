#include "resources.hpp"
#include "../../src/utils/fnva1.hpp"
#include "../../src/utils/obfuscation.hpp"
#include "render_wrapper.hpp"
#include <d3dx9.h>
#include <format>
#include "../../src/utils/utils.hpp"

#define CREATE_TEXTURE_MEM(tex_name, bytes, bytes_size) \
	static LPDIRECT3DTEXTURE9 tex_name = NULL;          \
	if (tex_name == NULL)                               \
	D3DXCreateTextureFromFileInMemory(render::device, bytes, bytes_size, &tex_name)

#define CREATE_TEXTURE_FILE(tex_name, path)    \
	static LPDIRECT3DTEXTURE9 tex_name = NULL; \
	if (tex_name == NULL)                      \
	D3DXCreateTextureFromFileA(render::device, path, &tex_name)

namespace NS_GUI {
	void resources::create_textures(IDirect3DDevice9* device, std::function<void()> callback) {
		if (!downloaded.load())
			return;

		for (auto& name: files_to_create) {
			auto& texture = textures[fnva1(name.c_str())];
			texture = new render::gif_t{ utils::read_file_bin(std::vformat(STR("C:\\Weave\\assets\\{}.png"), std::make_format_args(name))) };

			//D3DXCreateTextureFromFileA(device, std::vformat(STR("C:\\Weave\\assets\\{}.png"), std::make_format_args(name)).c_str(), (LPDIRECT3DTEXTURE9*)&texture);
		}

		if (!files_to_create.empty()) {
			files_to_create.clear();
			callback();
		}
	}
}// namespace NS_GUI