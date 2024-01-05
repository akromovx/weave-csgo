#pragma once
#include "../base_includes.hpp"

namespace utils {
	class protect_guard_t {
	public:
		__forceinline protect_guard_t(void* base, uint32_t length, uint32_t protect) {
			m_base = base;
			m_length = length;

			VirtualProtect(base, length, protect, (PDWORD)(&m_old_protect));
		}

		__forceinline ~protect_guard_t() {
			VirtualProtect(m_base, m_old_protect, m_old_protect, (PDWORD)(&m_old_protect));
		}

	private:
		void* m_base{ nullptr };
		uint32_t m_length{};
		uint32_t m_old_protect{};
	};

	class vmt_t {
	public:
		__forceinline vmt_t() : m_class_base(nullptr), m_method_count(0),
								m_shadow_vtable(nullptr), m_original_vtable(nullptr),
								m_indices({}) {}

		__forceinline vmt_t(void* base) : m_class_base(base), m_method_count(0),
										  m_shadow_vtable(nullptr), m_original_vtable(nullptr),
										  m_indices({}) {}

		__forceinline ~vmt_t() {
			restore_vtable();

			m_indices.clear();
		}

		__forceinline void setup(void* base = nullptr) {
			if (base != nullptr)
				m_class_base = base;

			if (!m_class_base)
				return;

			m_original_vtable = *(uintptr_t**)(m_class_base);
			m_method_count = get_vtable_methods(m_original_vtable);

			if (m_method_count == 0)
				return;

			m_shadow_vtable = new uintptr_t[m_method_count + 1]();

			m_shadow_vtable[0] = m_original_vtable[-1];
			std::memcpy(&m_shadow_vtable[1], m_original_vtable, m_method_count * sizeof(uintptr_t));

			protect_guard_t guard = protect_guard_t{ m_class_base, sizeof(uintptr_t), PAGE_READWRITE };
			*(uintptr_t**)(m_class_base) = &m_shadow_vtable[1];
		}

		template<typename T>
		__forceinline void hook(uint32_t index, T method) {
			m_shadow_vtable[index + 1] = (uintptr_t)(method);
			m_indices.emplace_back(index);
		}

		__forceinline void unhook(uint32_t index) {
			m_shadow_vtable[index + 1] = m_original_vtable[index];
		}

		__forceinline void unhook_all() {
			if (m_indices.empty())
				return;

			for (const auto& i: m_indices)
				this->unhook(i);
		}

		template<typename T>
		__forceinline constexpr T original(uint32_t index) {
			return (T)m_original_vtable[index];
		}

		inline void restore_vtable() {
			if (m_original_vtable != nullptr) {
				protect_guard_t guard = protect_guard_t{ m_class_base, sizeof(uintptr_t), PAGE_READWRITE };
				*(uintptr_t**)(m_class_base) = m_original_vtable;
				m_original_vtable = nullptr;
			}
		}

	private:
		__forceinline constexpr uint32_t get_vtable_methods(uintptr_t* vtable_start) {
			uint32_t len = -1;

			while (vtable_start[len])
				len++;

			return len;
		}

		void* m_class_base{ nullptr };
		uint32_t m_method_count{};
		uintptr_t* m_original_vtable{ nullptr };
		uintptr_t* m_shadow_vtable{ nullptr };

		std::vector<int> m_indices{};
	};
} // namespace utils