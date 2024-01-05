#pragma once
#include "../../base_includes.hpp"
#include <deque>
#include <mutex>

struct console_t {
	void clear();
	void print(const std::string& text);
	void print_colored_id(const std::string& str);
	void print_gradient_text(std::string text, std::pair<color_t, color_t> colors, int line = 0, int total_lines = 0);
};

GLOBAL_DYNPTR(console_t, game_console);

struct cheat_logs_t {
	std::mutex mtx{};

	CONSTS(animation_time_in = 0.5f,
		   animation_time_out = 0.5f,
		   min_showtime = 4.f,
		   max_showtime = 20.f,

		   showtime_per_symbol = 1.f / 16.f);

	struct message_t {
		std::string m_text{};
		enum class e_type : hash_t {
			miss = HASH("message-type:miss"),
			hit = HASH("message-type:hit"),
			buy = HASH("message-type:buy"),
			info = HASH("message-type:info"),
			error = HASH("message-type:error"),
		} m_type;

		message_t(const std::string& text, e_type type, float time, float offset)
			: m_text(text), m_type(type), m_time(time), m_offset(offset), m_outdated(false), m_size(0.f) {}

		float m_offset{}, m_velocity{}, m_size{};

		constexpr auto get_time() const { return m_time; }
		constexpr auto get_showtime() const { return min_showtime; }

		bool m_outdated{};

	private:
		float m_time{};
	};

	std::vector<message_t> logs{};

	__forceinline void add_miss(const std::string& message) { return add(message, message_t::e_type::info); }
	__forceinline void add_hit(const std::string& message) { return add(message, message_t::e_type::hit); }
	__forceinline void add_buy(const std::string& message) { return add(message, message_t::e_type::buy); }
	__forceinline void add_info(const std::string& message) { return add(message, message_t::e_type::info); }
	__forceinline void add_error(const std::string& message) {
		game_console->print_colored_id(dformat(STRS("%4{}\n"), message));
		return add(message, message_t::e_type::error);
	}

	void add(const std::string& message, message_t::e_type type);
	void render();
};

GLOBAL_DYNPTR(cheat_logs_t, cheat_logs);