#pragma once
#include "../../deps/minhook/MinHook.h"
#include "../utils/displacement.hpp"
#include <algorithm>
#include <vector>

namespace hooks {
	struct hook_t {
		bool m_enabled{};
		void* m_target{};
		void* m_original{};
		void* m_custom{};

		__forceinline void enable() {
			MH_EnableHook(m_target);
			m_enabled = true;
		}

		__forceinline void disable() {
			MH_DisableHook(m_target);
			m_enabled = false;
		}
	};

	class c_hooks {
		std::vector<hook_t> m_hooks;

	public:
		__forceinline c_hooks() {
			MH_Initialize();
		}
		__forceinline ~c_hooks() {
			MH_Uninitialize();
		}

		template<typename fn = uintptr_t>
		__forceinline bool hook(fn custom_func, void* o_func) {
			hook_t h = {};
			h.m_target = o_func;
			h.m_custom = custom_func;
			if (MH_CreateHook(o_func, custom_func, &h.m_original) == MH_OK) {
				m_hooks.emplace_back(h);
				return true;
			}

			return false;
		}

		__forceinline void hook_all() {
			for (auto& h: m_hooks)
				h.enable();
		}

		__forceinline void unhook_all() {
			for (auto& h: m_hooks)
				h.disable();
		}

		template<typename fn_t = uintptr_t, class ret_t = fn_t>
		__forceinline ret_t original(fn_t custom_func) {
			auto it = std::find_if(m_hooks.begin(), m_hooks.end(), [&](hook_t hook) {
				return hook.m_custom == custom_func;
			});

			if (it != m_hooks.end())
				return (ret_t)it->m_original;

			return nullptr;
		}
	};
};// namespace hooks

GLOBAL_DYNPTR(hooks::c_hooks, detour);