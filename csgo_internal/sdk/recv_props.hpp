#pragma once
#include "../src/base_includes.hpp"

namespace sdk {
	enum e_send_prop_type {
		_int = 0,
		_float,
		_vec,
		_vec_xy,
		_string,
		_array,
		_data_table,
		_int_64,
	};

	struct variant_t;
	struct recv_table_t;
	struct recv_prop_t;
	struct recv_proxy_data_t;

	using recv_var_proxy_fn = void (*)(const recv_proxy_data_t* data, void* struct_ptr, void* out_ptr);
	using array_length_recv_proxy_fn = void (*)(void* struct_ptr, int object_id, int current_array_length);
	using data_table_recv_var_proxy_fn = void (*)(const recv_prop_t* prop, void** out_ptr, void* data_ptr, int object_id);

	struct variant_t {
		union {
			float m_float;
			long m_int;
			char* m_string;
			void* m_data;
			float m_vector[3];
			__int64 m_int64;
		};

		e_send_prop_type type;
	};

	struct recv_proxy_data_t {
		const recv_prop_t* m_recv_prop;
		variant_t m_value;
		int m_element_index;
		int m_object_id;
	};

	struct recv_prop_t {
		char* m_prop_name;
		e_send_prop_type m_prop_type;
		int m_prop_flags;
		int m_buffer_size;
		int m_is_inside_of_array;
		const void* m_extra_data_ptr;
		recv_prop_t* m_array_prop;
		array_length_recv_proxy_fn m_array_length_proxy;
		recv_var_proxy_fn m_proxy_fn;
		data_table_recv_var_proxy_fn m_data_table_proxy_fn;
		recv_table_t* m_data_table;
		int m_offset;
		int m_element_stride;
		int m_elements_count;
		const char* m_parent_array_prop_name;
	};

	struct recv_table_t {
		recv_prop_t* m_props;
		int m_props_count;
		void* m_decoder_ptr;
		char* m_table_name;
		bool m_is_initialized;
		bool m_is_in_main_list;
	};

	struct recv_prop_hook_t {
		~recv_prop_hook_t() { m_target_property->m_proxy_fn = m_original_proxy_fn; }
		const recv_var_proxy_fn original() { return m_original_proxy_fn; }
		void hook(hash_t table, hash_t netvar, recv_var_proxy_fn user_proxy_fn);

	private:
		recv_prop_t* m_target_property{};
		recv_var_proxy_fn m_original_proxy_fn{};
	};
} // namespace sdk