#pragma once
#include "../src/utils/utils.hpp"

namespace sdk {
	enum e_cs_round_end_reason {
		UNKNOWN1 = 0,
		BOMB_DETONATED, // terrorists planted bomb and it detonated.
		UNKNOWN2,
		UNKNOWN3,
		T_ESCAPED,		   // dunno if used.
		CT_STOPPED_ESCAPE, // dunno if used.
		T_STOPPED,		   // dunno if used
		BOMB_DEFUSED,	   // counter-terrorists defused the bomb.
		CT_WIN,			   // counter-terrorists killed all terrorists.
		T_WIN,			   // terrorists killed all counter-terrorists.
		ROUND_DRAW,		   // draw ( likely due to time ).
		HOSTAGE_RESCUED,   // counter-terrorists rescued a hostage.
		CT_WIN_TIME,
		T_WIN_TIME,
		T_NOT_ESACPED,
		UNKNOWN4,
		GAME_START,
		T_SURRENDER,
		CT_SURRENDER,
	};

	struct key_values_t;

	struct game_event_callback_t {
		void* m_callback;
		int m_listener_type;
	};

	struct game_event_descriptor_t {
		char m_name[32];
		int m_id;
		key_values_t* m_keys;
		bool m_is_local;
		bool m_is_reliable;
		utils::utl_vector_t<game_event_callback_t*> m_listeners;
	};

	struct game_event_t {
		game_event_descriptor_t* m_descriptor;
		key_values_t* m_keys;

		virtual ~game_event_t(){};
		virtual const char* get_name() const = 0;
		virtual bool is_reliable() const = 0;
		virtual bool is_local() const = 0;
		virtual bool is_empty(const char* keyName = nullptr) = 0;
		virtual bool get_bool(const char* keyName = nullptr, bool defaultValue = false) = 0;
		virtual int get_int(const char* keyName = nullptr, int defaultValue = 0) = 0;
		virtual unsigned long long get_uint64(char const* keyName = nullptr, unsigned long long defaultValue = 0) = 0;
		virtual float get_float(const char* keyName = nullptr, float defaultValue = 0.0f) = 0;
		virtual const char* get_string(const char* keyName = nullptr, const char* defaultValue = "") = 0;
		virtual const wchar_t* get_wstring(char const* keyName = nullptr, const wchar_t* defaultValue = L"") = 0;
		virtual void set_bool(const char* keyName, bool value) = 0;
		virtual void set_int(const char* keyName, int value) = 0;
		virtual void set_uint64(const char* keyName, unsigned long long value) = 0;
		virtual void set_float(const char* keyName, float value) = 0;
		virtual void set_string(const char* keyName, const char* value) = 0;
		virtual void set_wstring(const char* keyName, const wchar_t* value) = 0;
	};

	struct game_event_listener2_t {
		virtual ~game_event_listener2_t(){};
		virtual void fire_game_event(game_event_t* event) = 0;
		virtual int get_event_debug_id(void) { return m_debug_id; }

		int m_debug_id;
	};

	struct game_event_manager2_t {
		utils::utl_vector_t<game_event_descriptor_t> m_events;
		utils::utl_vector_t<game_event_callback_t*> m_listeners;

		virtual ~game_event_manager2_t(){};
		virtual int load_events_from_file(const char* filename) = 0;
		virtual void reset() = 0;
		virtual bool add_listener(game_event_listener2_t* listener, const char* name, bool bServerSide) = 0;
		virtual bool find_listener(game_event_listener2_t* listener, const char* name) = 0;
		virtual void remove_listener(game_event_listener2_t* listener) = 0;
		virtual bool add_listener_global(game_event_listener2_t* listener, bool server_side) = 0;
		virtual game_event_t* create_event(const char* name, bool force = false, int* cookie = NULL) = 0;
		virtual bool fire_event(game_event_t* event, bool dont_broadcast = false) = 0;
		virtual bool fire_event_client_side(game_event_t* event) = 0;
	};
} // namespace sdk