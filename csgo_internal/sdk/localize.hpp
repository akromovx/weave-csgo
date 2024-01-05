#pragma once

namespace sdk {
	struct localize_t {
		VFUNC(find(const char* token_name), const wchar_t*(__thiscall*)(decltype(this), const char*), 11, token_name);
		VFUNC(find_safe(const char* token_name), const wchar_t*(__thiscall*)(decltype(this), const char*), 12, token_name);

		VFUNC(convert_ansi_to_unicode(const char* ansi, wchar_t* unicode, int unicode_buffer_size),
			  const wchar_t*(__thiscall*)(decltype(this), const char*, wchar_t*, int), 15, ansi, unicode, unicode_buffer_size);

		VFUNC(convert_unicode_to_ansi(const wchar_t* unicode, char* ansi, int ansi_buffer_size),
			  const wchar_t*(__thiscall*)(decltype(this), const wchar_t*, char*, int), 16, unicode, ansi, ansi_buffer_size);

		VFUNC(find_utf8(const char* token_name), const char*(__thiscall*)(decltype(this), const char*), 47, token_name);

		__forceinline std::string convert_unicode_to_ansi(const wchar_t* unicode) noexcept {
			char buffer[4096]{};
			convert_unicode_to_ansi(unicode, buffer, sizeof(buffer));
			return buffer;
		}
	};
} // namespace sdk