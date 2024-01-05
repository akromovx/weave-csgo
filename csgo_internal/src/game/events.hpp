#pragma once
#include "../interfaces.hpp"
#include <map>

class event_listener_t : public sdk::game_event_listener2_t {
private:
	using game_event_t = std::function<void(sdk::game_event_t*)>;
	std::map<std::string, std::vector<game_event_t>> m_events;

public:
	__forceinline event_listener_t() : m_events{} {
		m_debug_id = 42;
	}

	void init();

	__forceinline void add(const std::string& name, game_event_t fn) {
		m_events[name].push_back(fn);
	}

	__forceinline void register_events() {
		if (!interfaces::game_events->add_listener_global(this, false))
			throw std::runtime_error(STRS("Can't create entity list"));
	}

	void fire_game_event(sdk::game_event_t* e) override {
		const std::string name = e->get_name();

		if (m_events.count(name) != 0) {
			for (auto& callback: m_events[name])
				callback(e);
		}
	}
};

GLOBAL_DYNPTR(event_listener_t, game_events);