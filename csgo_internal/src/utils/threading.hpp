#pragma once
#include "../base_includes.hpp"
#include <functional>
#include <mutex>

struct threading_t final {
	std::vector<std::thread> m_threads{};

	using callback_t = std::function<void()>;
	using callbacks_t = std::vector<callback_t>;

	void run(const callbacks_t& tasks);
	bool init();
	void remove();

	~threading_t() { remove(); }

private:
	using allocate_thread_id_t = int32_t (*)();
	using free_thread_id_t = void (*)();

	allocate_thread_id_t m_allocate_thread_id{};
	free_thread_id_t m_free_thread_id{};

	std::vector<callback_t> m_queue{};
	std::mutex m_queue_mtx{};
	std::condition_variable m_queue_cond{};
	size_t m_to_run{};
	std::atomic<bool> m_stop{};
};

GLOBAL_DYNPTR(threading_t, threading);