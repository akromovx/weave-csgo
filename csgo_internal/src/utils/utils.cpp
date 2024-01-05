#include "utils.hpp"
#include <fstream>
#include "sha2.hpp"

#define IS_IN_RANGE(value, max, min) (value >= max && value <= min)
#define GET_BITS(value) (IS_IN_RANGE(value, '0', '9') ? (value - '0') : ((value & (~0x20)) - 'A' + 0xA))
#define GET_BYTE(value) (GET_BITS(value[0]) << 4 | GET_BITS(value[1]))

using namespace std::chrono_literals;

namespace utils {
	__forceinline address_t find_pattern(const std::string& image_name, const std::string& signature) {
		auto image = GetModuleHandleA(image_name.c_str());
		if (!image) {
			return 0u;
		}

		auto image_base = (uintptr_t)(image);
		auto image_dos_hdr = (IMAGE_DOS_HEADER*)(image_base);

		if (image_dos_hdr->e_magic != XOR32S(IMAGE_DOS_SIGNATURE))
			return 0u;

		auto image_nt_hdrs = (IMAGE_NT_HEADERS*)(image_base + image_dos_hdr->e_lfanew);

		if (image_nt_hdrs->Signature != XOR32S(IMAGE_NT_SIGNATURE))
			return 0u;

		auto scan_begin = (uint8_t*)(image_base);
		auto scan_end = (uint8_t*)(image_base + image_nt_hdrs->OptionalHeader.SizeOfImage);

		uint8_t* scan_result = nullptr;
		uint8_t* scan_data = (uint8_t*)(signature.c_str());

		for (auto current = scan_begin; current < scan_end; current++) {
			if (*(uint8_t*)scan_data == '\?' || *current == GET_BYTE(scan_data)) {
				if (!scan_result)
					scan_result = current;

				if (!scan_data[2])
					return (uintptr_t)(scan_result);

				scan_data += (*(uint16_t*)scan_data == '\?\?' || *(uint8_t*)scan_data != '\?') ? 3 : 2;

				if (!*scan_data)
					return (uintptr_t)(scan_result);
			} else if (scan_result) {
				current = scan_result;
				scan_data = (uint8_t*)(signature.c_str());
				scan_result = nullptr;
			}
		}

		return 0u;
	}

	__forceinline address_t find_pattern(uintptr_t start, const std::string& signature, uintptr_t size) {
		auto scan_begin = (uint8_t*)(start);
		auto scan_end = (uint8_t*)(start + size);

		uint8_t* scan_result = nullptr;
		uint8_t* scan_data = (uint8_t*)(signature.c_str());

		for (auto current = scan_begin; current < scan_end; current++) {
			if (*(uint8_t*)scan_data == '\?' || *current == GET_BYTE(scan_data)) {
				if (!scan_result)
					scan_result = current;

				if (!scan_data[2])
					return (uintptr_t)(scan_result);

				scan_data += (*(uint16_t*)scan_data == '\?\?' || *(uint8_t*)scan_data != '\?') ? 3 : 2;

				if (!*scan_data)
					return (uintptr_t)(scan_result);
			} else if (scan_result) {
				current = scan_result;
				scan_data = (uint8_t*)(signature.c_str());
				scan_result = nullptr;
			}
		}

		return 0u;
	}

	__forceinline address_t find_interface(const char* library, const char* name) {
		typedef void* (*create_interface_fn)(const char* name, int ret);
		create_interface_fn create_interface = (create_interface_fn)GetProcAddress(GetModuleHandleA(library), STRC("CreateInterface"));

		if (!create_interface)
			return nullptr;

		return create_interface(name, 0);
	}

	__forceinline void wait_for_module(std::string name) {
		while (!GetModuleHandleA(name.c_str()))
			std::this_thread::sleep_for(250ms);
	}

	__forceinline bytes_t read_file_bin(const std::string& path) {
		bytes_t result;

		std::ifstream ifs(path, std::ios::binary | std::ios::ate);
		if (ifs) {
			std::ifstream::pos_type pos = ifs.tellg();
			result.resize((size_t)pos);

			ifs.seekg(0, std::ios::beg);
			ifs.read((char*)result.data(), pos);
		}

		return result;
	}

	__forceinline void write_file_bin(const std::string& path, const bytes_t& bytes) {
		std::ofstream file_out(path, std::ios::binary | std::ios::ate);
		if (file_out.good())
			file_out.write((char*)bytes.data(), bytes.size());

		file_out.close();
	}

	__forceinline std::string file_hash(const std::string& path) {
		auto file = read_file_bin(path);
		if (file.empty())
			return "";

		return sha512::get(file);
	}
} // namespace utils