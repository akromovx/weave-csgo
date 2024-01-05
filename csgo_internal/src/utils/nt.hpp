#pragma once
#include <Windows.h>

#pragma comment(lib, "ntdll")

extern "C" {
NTSYSAPI NTSTATUS NTAPI NtProtectVirtualMemory(HANDLE process_handle, PVOID* base_address, PULONG number_of_bytes_to_protect,
											   ULONG new_access_protection, PULONG old_access_protection);
}

#define CURRENT_PROCESS HANDLE(-1)
constexpr auto page_size = 4096;
constexpr auto previous_state_mask = 0x40000000;

class memory_guard_t {
public:
	template<typename F>
	explicit memory_guard_t(const DWORD protection, const F address, const size_t wanted_size = page_size)
		: address(reinterpret_cast<PVOID>(address)), protection(PAGE_EXECUTE_READ), wanted_size(wanted_size), is_attached(true) {
		ULONG size = wanted_size;
		NtProtectVirtualMemory(CURRENT_PROCESS, &this->address, &size, protection, &this->protection);
	}

	PVOID detach() {
		is_attached = false;
		return address;
	}

	memory_guard_t(memory_guard_t const& other) = delete;
	memory_guard_t& operator=(memory_guard_t const& other) = delete;
	memory_guard_t(memory_guard_t&& other) = delete;
	memory_guard_t& operator=(memory_guard_t&& other) = delete;

	~memory_guard_t() {
		if (!is_attached)
			return;

		ULONG size = wanted_size;
		NtProtectVirtualMemory(CURRENT_PROCESS, &address, &size, protection, &protection);
	}

private:
	PVOID address;
	DWORD protection;
	size_t wanted_size;
	bool is_attached;
};