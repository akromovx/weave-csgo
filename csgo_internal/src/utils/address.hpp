#pragma once
#include "../base_includes.hpp"

// credits to: supremacy
// useful for patterns and other things

struct address_t {
protected:
	uintptr_t m_addr;

public:
	__forceinline address_t() : m_addr{} {};
	__forceinline ~address_t(){};

	__forceinline address_t(uintptr_t a) : m_addr{ a } {}
	__forceinline address_t(const void* a) : m_addr{ (uintptr_t)a } {}

	__forceinline operator uintptr_t() { return m_addr; }
	__forceinline operator void*() { return (void*)m_addr; }
	__forceinline operator const void*() { return (const void*)m_addr; }

	__forceinline bool operator==(address_t a) const {
		return as<uintptr_t>() == a.as<uintptr_t>();
	}
	__forceinline bool operator!=(address_t a) const {
		return as<uintptr_t>() != a.as<uintptr_t>();
	}

	template<typename t = address_t>
	__forceinline t as() const {
		return (m_addr) ? (t)m_addr : t{};
	}

	template<typename t = address_t>
	__forceinline t as(size_t offset) const {
		return (m_addr) ? (t)(m_addr + offset) : t{};
	}

	template<typename t = address_t>
	__forceinline t as(ptrdiff_t offset) const {
		return (m_addr) ? (t)(m_addr + offset) : t{};
	}

	template<typename t = address_t>
	__forceinline t at(size_t offset) const {
		return (m_addr) ? *(t*)(m_addr + offset) : t{};
	}

	template<typename t = address_t>
	__forceinline t at(ptrdiff_t offset) const {
		return (m_addr) ? *(t*)(m_addr + offset) : t{};
	}

	template<typename t = address_t>
	__forceinline t add(size_t offset) const {
		return (m_addr) ? (t)(m_addr + offset) : t{};
	}

	template<typename t = address_t>
	__forceinline t add(ptrdiff_t offset) const {
		return (m_addr) ? (t)(m_addr + offset) : t{};
	}

	template<typename t = address_t>
	__forceinline t sub(size_t offset) const {
		return (m_addr) ? (t)(m_addr - offset) : t{};
	}

	template<typename t = address_t>
	__forceinline t sub(ptrdiff_t offset) const {
		return (m_addr) ? (t)(m_addr - offset) : t{};
	}

	template<typename t = address_t>
	__forceinline t to() const {
		return *(t*)m_addr;
	}

	template<typename t = address_t>
	__forceinline t get(size_t n = 1) {
		uintptr_t out;

		if (!m_addr)
			return t{};

		out = m_addr;

		for (size_t i{ n }; i > 0; --i) {
			if (!valid(out))
				return t{};

			out = *(uintptr_t*)out;
		}

		return (t)out;
	}

	template<typename t = address_t>
	__forceinline t rel32(size_t offset) {
		uintptr_t out;
		uint32_t r;

		if (!m_addr)
			return t{};

		out = m_addr + offset;

		r = *(uint32_t*)out;
		if (!r)
			return t{};

		out = (out + 4) + r;

		return (t)out;
	}

	template<typename t = uintptr_t>
	__forceinline void set(const t& value) {
		if (!m_addr)
			return;

		*(t*)m_addr = value;
	}

	static __forceinline bool valid(uintptr_t addr) {
		MEMORY_BASIC_INFORMATION mbi{};

		if (addr == 0)
			return false;

		if (!VirtualQuery((const void*)addr, &mbi, sizeof(mbi)))
			return false;

		if ((mbi.Protect & PAGE_NOACCESS) || (mbi.Protect & PAGE_GUARD))
			return false;

		return true;
	}
};