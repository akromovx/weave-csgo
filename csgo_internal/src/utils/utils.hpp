#pragma once
#include "address.hpp"
#include <emmintrin.h>

#define TIME_TO_TICKS(t) ((int)(0.5f + (float)(t) / interfaces::global_vars->m_interval_per_tick))
#define TICKS_TO_TIME(t) (interfaces::global_vars->m_interval_per_tick * (float)(t))

static constexpr auto PI = 3.14159265358979323846;
static constexpr auto fPI = (float)PI;
static constexpr auto f2PI = fPI * 2.f;

#define DEG2RAD(x) ((float)(x) * (float)((float)(PI) / 180.0f))
#define RAD2DEG(x) ((float)(x) * (float)(180.0f / (float)(PI)))

namespace utils {
	extern address_t find_pattern(const std::string& image_name, const std::string& signature);
	extern address_t find_pattern(uintptr_t start, const std::string& signature, uintptr_t size);
	extern address_t find_interface(const char* library, const char* name);
	extern void wait_for_module(std::string mdl);
	extern bytes_t read_file_bin(const std::string& path);
	extern void write_file_bin(const std::string& path, const bytes_t& bytes);
	extern std::string file_hash(const std::string& path);

	template<typename T = address_t>
	__forceinline constexpr T vfunc(const void* ptr, const unsigned int idx) {
		return (T)((*(void***)ptr)[idx]);
	}

	template<typename T>
	static constexpr auto relative_to_absolute(uintptr_t address) noexcept {
		return (T)(address + 4 + *reinterpret_cast<std::int32_t*>(address));
	}

	STFI void memcpy_sse(void* dst, const void* src, size_t size) {
		auto src_sse = (__m128i*)src;
		auto dst_sse = (__m128i*)dst;
		auto sse_size = size / 16; // 16 bytes in a SSE2 register

		for (size_t i = 0; i < sse_size; ++i) {
			__m128i reg = _mm_loadu_si128(src_sse + i); // unaligned load from src
			_mm_storeu_si128(dst_sse + i, reg);			// unaligned store to dst
		}

		// copying remaining part, if any
		size_t remaining = size % 16;
		if (remaining != 0) {
			auto src_char = (char*)src;
			auto dst_char = (char*)dst;
			std::memcpy(dst_char + size - remaining, src_char + size - remaining, remaining);
		}
	}

	__forceinline static int calc_new_allocation_count(int nAllocationCount, int nGrowSize, int nNewSize, int nBytesItem) {
		if (nGrowSize)
			nAllocationCount = ((1 + ((nNewSize - 1) / nGrowSize)) * nGrowSize);
		else {
			if (!nAllocationCount)
				nAllocationCount = (31 + nBytesItem) / nBytesItem;

			while (nAllocationCount < nNewSize)
				nAllocationCount *= 2;
		}

		return nAllocationCount;
	}

	template<class T, class I = int>
	class utl_memory_t {
	public:
		__forceinline bool is_valid_index(I i) const {
			long x = i;
			return (x >= 0) && (x < m_alloc_count);
		}

		T& operator[](I i);
		const T& operator[](I i) const;

		__forceinline T* base() { return m_memory; }

		__forceinline int num_allocated() const { return m_alloc_count; }

		__forceinline void grow(int num) {
			auto old_alloc_count = m_alloc_count;
			// Make sure we have at least numallocated + num allocations.
			// Use the grow rules specified for this memory (in m_grow_size)
			int alloc_requested = m_alloc_count + num;

			int new_alloc_count = calc_new_allocation_count(m_alloc_count, m_grow_size, alloc_requested, sizeof(T));

			// if m_alloc_requested wraps index type I, recalculate
			if ((int)(I)new_alloc_count < alloc_requested) {
				if ((int)(I)new_alloc_count == 0 && (int)(I)(new_alloc_count - 1) >= alloc_requested)
					--new_alloc_count; // deal w/ the common case of m_alloc_count == MAX_USHORT + 1
				else {
					if ((int)(I)alloc_requested != alloc_requested) {
						// we've been asked to grow memory to a size s.t. the index type can't address the requested amount of memory
						return;
					}

					while ((int)(I)new_alloc_count < alloc_requested)
						new_alloc_count = (new_alloc_count + alloc_requested) / 2;
				}
			}

			m_alloc_count = new_alloc_count;

			if (m_memory) {
				auto ptr = new unsigned char[m_alloc_count * sizeof(T)];

				memcpy(ptr, m_memory, old_alloc_count * sizeof(T));
				m_memory = (T*)ptr;
			} else
				m_memory = (T*)new unsigned char[m_alloc_count * sizeof(T)];
		}

	protected:
		T* m_memory;
		int m_alloc_count;
		int m_grow_size;
	};

	template<class T, class I>
	__forceinline T& utl_memory_t<T, I>::operator[](I i) { return m_memory[i]; }

	template<class T, class I>
	__forceinline const T& utl_memory_t<T, I>::operator[](I i) const { return m_memory[i]; }

	template<class T>
	__forceinline void destruct(T* memory) { memory->~T(); }

	template<class T>
	__forceinline T* construct(T* memory) { return ::new (memory) T; }

	template<class T>
	__forceinline T* copy_construct(T* memory, T const& src) { return ::new (memory) T(src); }

	template<class T, class A = utl_memory_t<T>>
	class utl_vector_t {
		typedef A allocator_t;

		typedef T* iterator;
		typedef const T* const_iterator;

	public:
		T& operator[](int i);
		const T& operator[](int i) const;

		__forceinline T& element(int i) {
			return m_memory[i];
		}

		__forceinline const T& element(int i) const {
			return m_memory[i];
		}

		__forceinline T* base() {
			return m_memory.base();
		}

		__forceinline int count() const {
			return m_size;
		}

		__forceinline void remove_all() {
			for (int i = m_size; --i >= 0;)
				destruct(&element(i));

			m_size = 0;
		}

		__forceinline bool is_valid_index(int i) const {
			return (i >= 0) && (i < m_size);
		}

		__forceinline void shift_elements_right(int elem, int num = 1) {
			int num_to_move = m_size - elem - num;
			if ((num_to_move > 0) && (num > 0))
				memmove(&element(elem + num), &element(elem), num_to_move * sizeof(T));
		}

		__forceinline void shift_elements_left(int elem, int num = 1) {
			int numToMove = m_size - elem - num;
			if ((numToMove > 0) && (num > 0))
				memmove(&element(elem), &element(elem + num), numToMove * sizeof(T));
		}

		__forceinline void grow_vector(int num = 1) {
			if (m_size + num > m_memory.num_allocated()) {
				m_memory.grow(m_size + num - m_memory.num_allocated());
			}

			m_size += num;
		}

		__forceinline int insert_before(int elem) {
			grow_vector();
			shift_elements_right(elem);
			construct(&element(elem));
			return elem;
		}

		__forceinline int add_to_head() {
			return insert_before(0);
		}

		__forceinline int add_to_tail() { return insert_before(m_size); }

		__forceinline int insert_before(int elem, const T& src) {
			grow_vector();
			shift_elements_right(elem);
			copy_construct(&element(elem), src);
			return elem;
		}

		__forceinline int add_to_tail(const T& src) {
			return insert_before(m_size, src);
		}

		__forceinline int find(const T& src) const {
			for (int i = 0; i < count(); ++i) {
				if (element(i) == src)
					return i;
			}
			return -1;
		}

		__forceinline void remove(int elem) {
			destruct(&element(elem));
			shift_elements_left(elem);
			--m_size;
		}

		__forceinline bool find_and_remove(const T& src) {
			int elem = find(src);
			if (elem != -1) {
				remove(elem);
				return true;
			}

			return false;
		}

		__forceinline iterator begin() { return base(); }
		__forceinline const_iterator begin() const { return base(); }
		__forceinline iterator end() { return base() + count(); }
		__forceinline const_iterator end() const { return base() + count(); }

		allocator_t m_memory;
		int m_size;
		T* m_elements;
	};

	template<typename T, class A>
	T& utl_vector_t<T, A>::operator[](int i) {
		return m_memory[i];
	}

	template<typename T, class A>
	const T& utl_vector_t<T, A>::operator[](int i) const {
		return m_memory[i];
	}
} // namespace utils