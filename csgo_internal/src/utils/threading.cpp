#include "threading.hpp"
#include "utils.hpp"

STFI size_t calculate_remaining_tls_slots() {
	constexpr auto max_threads = 32;
	int highest_index = 1;
	static uint8_t* thread_id_allocated = (uint8_t*)((uintptr_t)GetModuleHandleA(STRSC("tier0.dll")) + XOR32S(0x54E30));

	while (highest_index < 128 && thread_id_allocated[highest_index] != 0)
		highest_index++;

	return std::clamp(max_threads - highest_index, 0, max_threads);
}

bool threading_t::init() {
	auto tier0 = GetModuleHandleA(STRC("tier0.dll"));
	if (tier0 == nullptr)
		return false;

	m_allocate_thread_id = (decltype(m_allocate_thread_id))GetProcAddress(tier0, STRSC("AllocateThreadID"));
	m_free_thread_id = (decltype(m_free_thread_id))GetProcAddress(tier0, STRSC("FreeThreadID"));

	const auto slots = std::min<size_t>(std::thread::hardware_concurrency() - 1, calculate_remaining_tls_slots());
	if (slots < 1) {
		MessageBoxA(0, dformat(STRS("failed to allocate threads: {}"), slots).c_str(), STRSC("Error"), MB_ICONERROR | MB_SETFOREGROUND);
		return false;
	}

	for (int i = 0; i < slots; i++) {
		m_threads.emplace_back([this, i]() {
			SetThreadAffinityMask(GetCurrentThread(), 1 << i);

			m_allocate_thread_id();

			while (true) {
				std::unique_lock<std::mutex> lock{ m_queue_mtx };
				m_queue_cond.wait(lock, [this] { return !m_queue.empty() || m_stop; });

				if (m_stop)
					break;

				const callback_t current = std::move(m_queue.back());
				m_queue.pop_back();
				lock.unlock();

				current();

				lock.lock();
				--m_to_run;
				lock.unlock();

				m_queue_cond.notify_all();
			}

			m_free_thread_id();
		});
	}

	return true;
}

void threading_t::run(const callbacks_t& tasks) {
	std::unique_lock<std::mutex> lock{ m_queue_mtx };
	m_queue_cond.wait(lock, [this] { return m_to_run == 0; });
	m_queue.insert(m_queue.end(), tasks.begin(), tasks.end());
	m_to_run = m_queue.size();

	m_queue_cond.notify_all();
	m_queue_cond.wait(lock, [this] { return m_to_run == 0; });
}

void threading_t::remove() {
	std::unique_lock<std::mutex> lock{ m_queue_mtx };

	if (m_stop)
		return;

	m_stop = true;
	lock.unlock();
	m_queue_cond.notify_all();

	for (auto& thread: m_threads)
		thread.join();
}