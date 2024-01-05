#pragma once
#include "../src/utils/utils.hpp"
#include "surface.hpp"

namespace sdk {
	using fn_change_callback_t = void(__cdecl*)(void*, const char*, float);

	struct convar_t {
		VFUNC(get_name(), const char*(__thiscall*)(decltype(this)), 5);

		VFUNC(set_value(const char* value), void(__thiscall*)(decltype(this), const char*), 14, value);
		VFUNC(set_value(float value), void(__thiscall*)(decltype(this), float), 15, value);
		VFUNC(set_value(int value), void(__thiscall*)(decltype(this), int), 16, value);
		VFUNC(set_value(color_t value), void(__thiscall*)(decltype(this), color_t), 16, value);

		__forceinline float get_float() const {
			std::uint32_t xored = *(uintptr_t*)(&m_parent->m_float_value) ^ (uintptr_t)this;
			return *(float*)&xored;
		}

		__forceinline int get_int() const {
			return m_parent->m_int_value ^ (uintptr_t)this;
		}

		__forceinline bool get_bool() const {
			return get_int() != 0 ? true : false;
		}

		__forceinline const char* get_string() const {
			char const* value = m_parent->m_string;
			return value ? value : STRC("");
		}

		std::byte pad0[0x4];
		convar_t* m_next;
		bool m_registered;
		const char* m_name;
		const char* m_help_string;
		int m_flags;
		std::byte pad1[0x4];
		convar_t* m_parent;
		const char* m_default_value;
		char* m_string;
		int m_string_length;
		float m_float_value;
		int m_int_value;
		bool m_has_min;
		float m_min_value;
		bool m_has_max;
		float m_max_value;
		utils::utl_vector_t<fn_change_callback_t> m_fn_change_callbacks;
	};

	// command to convars and concommands
	// @credits: https://github.com/ValveSoftware/source-sdk-2013/blob/master/sp/src/public/tier1/iconvar.h
	enum e_convar_flag : int {
		// convar systems
		fcvar_none = 0,
		fcvar_unregistered = (1 << 0),	  // if this is set, don't add to linked list, etc.
		fcvar_developmentonly = (1 << 1), // hidden in released products. flag is removed automatically if allow_development_cvars is defined.
		fcvar_gamedll = (1 << 2),		  // defined by the game dll
		fcvar_clientdll = (1 << 3),		  // defined by the client dll
		fcvar_hidden = (1 << 4),		  // hidden. doesn't appear in find or autocomplete. like developmentonly, but can't be compiled out.

		// convar only
		fcvar_protected = (1 << 5),		   // it's a server cvar, but we don't send the data since it's a password, etc.  sends 1 if it's not bland/zero, 0 otherwise as value
		fcvar_sponly = (1 << 6),		   // this cvar cannot be changed by clients connected to a multiplayer server.
		fcvar_archive = (1 << 7),		   // set to cause it to be saved to vars.rc
		fcvar_notify = (1 << 8),		   // notifies players when changed
		fcvar_userinfo = (1 << 9),		   // changes the client's info string
		fcvar_cheat = (1 << 14),		   // only useable in singleplayer / debug / multiplayer & sv_cheats
		fcvar_printableonly = (1 << 10),   // this cvar's string cannot contain unprintable characters ( e.g., used for player name etc ).
		fcvar_unlogged = (1 << 11),		   // if this is a fcvar_server, don't log changes to the log file / console if we are creating a log
		fcvar_never_as_string = (1 << 12), // never try to print that cvar

		// it's a convar that's shared between the client and the server.
		// at signon, the values of all such convars are sent from the server to the client (skipped for local client, ofc )
		// if a change is requested it must come from the console (i.e., no remote client changes)
		// if a value is changed while a server is active, it's replicated to all connected clients
		fcvar_server = (1 << 13),				   // server setting enforced on clients, replicated
		fcvar_demo = (1 << 16),					   // record this cvar when starting a demo file
		fcvar_dontrecord = (1 << 17),			   // don't record these command in demofiles
		fcvar_reload_materials = (1 << 20),		   // if this cvar changes, it forces a material reload
		fcvar_reload_textures = (1 << 21),		   // if this cvar changes, if forces a texture reload
		fcvar_not_connected = (1 << 22),		   // cvar cannot be changed by a client that is connected to a server
		fcvar_material_system_thread = (1 << 23),  // indicates this cvar is read from the material system thread
		fcvar_archive_xbox = (1 << 24),			   // cvar written to config.cfg on the xbox
		fcvar_accessible_from_threads = (1 << 25), // used as a debugging tool necessary to check material system thread convars
		fcvar_server_can_execute = (1 << 28),	   // the server is allowed to execute this command on clients via clientcommand/net_stringcmd/cbaseclientstate::processstringcmd.
		fcvar_server_cannot_query = (1 << 29),	   // if this is set, then the server is not allowed to query this cvar's value (via iserverpluginhelpers::startquerycvarvalue).
		fcvar_clientcmd_can_execute = (1 << 30),   // ivengineclient::clientcmd is allowed to execute this command.
		fcvar_material_thread_mask = (fcvar_reload_materials | fcvar_reload_textures | fcvar_material_system_thread)
	};

	using cvar_dll_identifier_t = int;
	struct con_command_t;

	struct con_command_base_t {
		void* m_vmt_base{};
		con_command_base_t* m_next;
		bool m_registered;
		const char* m_name;
		const char* m_help_string;
		flags_t m_flags;
		con_command_base_t* s_con_command_bases;
		void* m_accessor;
	};

	struct convars_t : app_system_t {
		virtual cvar_dll_identifier_t allocate_dll_indentifier() = 0;
		virtual void register_con_command(con_command_base_t* base) = 0;
		virtual void unregister_con_command(con_command_base_t* base) = 0;
		virtual void unregister_con_commands(cvar_dll_identifier_t id) = 0;
		virtual const char* get_command_line_value(const char* name) = 0;
		virtual con_command_base_t* find_command_base(const char* name) = 0;
		virtual const con_command_base_t* find_command_base(const char* name) const = 0;
		virtual convar_t* find_var_virtual(const char* var_name) = 0;
		virtual const convar_t* find_var_virtual(const char* var_name) const = 0;
		virtual con_command_t* find_command(const char* name) = 0;
		virtual const con_command_t* find_command(const char* name) const = 0;
		virtual void install_global_change_callback(fn_change_callback_t callback) = 0;
		virtual void remove_global_change_callback(fn_change_callback_t callback) = 0;
		virtual void call_global_change_callbacks(convar_t* var, const char* old_str, float old_val) = 0;
		virtual void install_console_display_func(void* func) = 0;
		virtual void remove_console_display_func(void* func) = 0;
		virtual void console_color_printf(const color_t& clr, const char* format, ...) const = 0;
		virtual void console_printf(const char* format, ...) const = 0;
		virtual void dconsole_dprintf(const char* format, ...) const = 0;
		virtual void rever_flagged_convars(int flag) = 0;

		OFFSET(commands, convar_t*, 0x30);
		OFFSET_PTR(commands_base, con_command_base_t**, 0x34);

		__forceinline convar_t* find_var(hash_t var_hash) {
			for (auto it = commands(); it != nullptr; it = it->m_next)
				if (fnva1(it->m_name) == var_hash)
					return it;

			return nullptr;
		}
	};
} // namespace sdk