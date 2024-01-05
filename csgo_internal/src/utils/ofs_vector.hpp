#pragma once
#include <iostream>
#include <vector>

namespace utils {
	template<typename type_t>
	struct ofs_vector_t : public std::vector<type_t> {
	protected:
		size_t m_offset = 0;
	public:
		__forceinline ofs_vector_t(const std::initializer_list<type_t>& initializer_list) : std::vector<type_t>(initializer_list) {}

		template<typename... args_t>
		__forceinline ofs_vector_t(args_t&&... args) : std::vector<type_t>(std::forward<args_t>(args)...) {}

		__forceinline const type_t& at(size_t index) const {
			return std::vector<type_t>::at((index + this->m_offset) % this->size());
		}

		__forceinline type_t& operator[](size_t index) {
			return std::vector<type_t>::operator[]((index + this->m_offset) % this->size());
		}

		__forceinline const type_t& operator[](size_t index) const {
			return std::vector<type_t>::operator[]((index + this->m_offset) % this->size());
		}

		__forceinline type_t& back() {
			return (*this)[(this->size() - 1)];
		}

		__forceinline const type_t& back() const {
			return (*this)[(this->size() - 1)];
		}

		__forceinline type_t& add() {
			++m_offset;
			return this->back();
		}

		type_t* begin() = delete;
		type_t* end() = delete;
		type_t* front() = delete;

		//void dump() const {
		//    ptrdiff_t len = this->size();
		//    for (ptrdiff_t i = 0; i < len; ++i)
		//        std::cout << (*this)[i] << " ";
		//    std::cout << std::endl;
		//}
	};
}
/*
	ofs_vector<int> test{ 0, 1, 2, 3, 4, 5, 6, 7 };

	test.dump(); // 0, 1, 2, 3, 4, 5, 6, 7

	test.add(8);

	test.dump(); // 1, 2, 3, 4, 5, 6, 7, 8

	test.add(9);
	test.dump(); // 2, 3, 4, 5, 6, 7, 8, 9

	test.add(10);
	test.dump(); // 3, 4, 5, 6, 7, 8, 9, 10

*/