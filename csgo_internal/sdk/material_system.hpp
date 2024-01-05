#pragma once
#include "../src/utils/utils.hpp"
#include "../src/utils/displacement.hpp"
#include "model_info.hpp"

namespace sdk {
	enum e_shader_stencil_op {
		shader_stencilop_keep = 1,
		shader_stencilop_zero = 2,
		shader_stencilop_set_to_reference = 3,
		shader_stencilop_increment_clamp = 4,
		shader_stencilop_decrement_clamp = 5,
		shader_stencilop_invert = 6,
		shader_stencilop_increment_wrap = 7,
		shader_stencilop_decrement_wrap = 8,

		shader_stencilop_force_dword = 0x7FFFFFFF
	};

	enum e_shader_stencil_func {
		shader_stencilfunc_never = 1,
		shader_stencilfunc_less = 2,
		shader_stencilfunc_equal = 3,
		shader_stencilfunc_lequal = 4,
		shader_stencilfunc_greater = 5,
		shader_stencilfunc_notequal = 6,
		shader_stencilfunc_gequal = 7,
		shader_stencilfunc_always = 8,

		shader_stencilfunc_force_dword = 0x7FFFFFFF
	};

	enum e_renderable_lightning_model {
		lighting_model_none = -1,
		lighting_model_standard = 0,
		lighting_model_static_prop,
		lighting_model_physics_prop,

		lighting_model_count,
	};

	struct material_t;
	struct key_values_system_t {
		virtual void register_sizeof_key_values(int size) = 0;
		virtual void pad_0x0001() = 0;
		virtual void* alloc_key_values_memory(int size) = 0;
		virtual void free_key_values_memory(void* memory) = 0;
		virtual int get_symbol_for_string(const char* name, bool create = true) = 0;
		virtual const char* get_string_for_symbol(int symbol) = 0;
		virtual void add_key_values_to_memory_leak_list(void* memory, int name) = 0;
		virtual void remove_key_values_to_memory_leak_list(void* memory) = 0;

		template<typename T = key_values_system_t*>
		static __forceinline T get() {
			static auto key_values_factory = address_t{ GetProcAddress(GetModuleHandleA(STRSC("vstdlib.dll")), STRSC("KeyValuesSystem")) };
			return key_values_factory.as<T (*)()>()();
		}

		//VFUNC(get_symbol_for_string(int symbol), const char* (__thiscall*)(void*, int), 4, symbol);
	};

	using get_symbol_proc_t = bool(__cdecl*)(const char*);

	struct key_values_t {
		enum types_t {
			TYPE_NONE = 0,
			TYPE_STRING,
			TYPE_INT,
			TYPE_FLOAT,
			TYPE_PTR,
			TYPE_WSTRING,
			TYPE_COLOR,
			TYPE_UINT64,
			TYPE_NUMTYPES,
		};

		__forceinline key_values_t* find_key(hash_t hash) {
			auto system = key_values_system_t::get();
			if (!hash || !system)
				return nullptr;

			for (auto dat = this->m_sub; dat != nullptr; dat = dat->m_peer) {
				const char* string = system->get_string_for_symbol(dat->m_key_name_id);

				if (string != nullptr && fnva1(string) == hash)
					return dat;
			}

			return nullptr;
		}

		__forceinline int get_int(int default_value = 0) {
			int result{};

			switch (this->m_data_type) {
				case TYPE_STRING:
					result = atoi(this->m_string);
					break;
				case TYPE_WSTRING:
					result = _wtoi(this->m_wide_string);
					break;
				case TYPE_FLOAT:
					result = (signed int)floor(this->m_float_value);
					break;
				case TYPE_UINT64:
					result = 0;
					break;
				default:
					result = this->m_int_value;
					break;
			}

			return (result ? result : default_value);
		}

		__forceinline bool get_bool() {
			return get_int() != 0 ? true : false;
		}

		__forceinline const char* get_name() const {
			auto system = key_values_system_t::get();
			return system->get_string_for_symbol((uint16_t(m_key_name_case_sensitive2) << 8) | uint8_t(m_key_name_case_sensitive));
		}

		__forceinline float get_float(float default_value = 0.f) {
			switch (this->m_data_type) {
				case TYPE_STRING:
					return (float)atof(this->m_string);
				case TYPE_WSTRING:
					return (float)_wtof(this->m_wide_string);
				case TYPE_FLOAT:
					return this->m_float_value;
				case TYPE_INT:
					return (float)this->m_int_value;
				case TYPE_UINT64:
					return (float)(*((uint64_t*)this->m_string));
				case TYPE_PTR:
				default:
					return 0.0f;
			};
		}

		uint32_t m_key_name_id : 24;
		uint32_t m_key_name_case_sensitive : 8;

		char* m_string;
		wchar_t* m_wide_string;

		union {
			int m_int_value;
			float m_float_value;
			void* m_ptr_value;
			unsigned char m_color_value[4];
		};

		char m_data_type;
		char m_has_escape_sequences;

		uint16_t m_key_name_case_sensitive2;

		uint8_t pad_0x0001[4];
		bool m_has_case_insensitive_key_symbol;

		key_values_t* m_peer;
		key_values_t* m_sub;
		key_values_t* m_chain;

		get_symbol_proc_t m_expression_get_symbol_proc;
	};

	struct light_desc_t {
		int m_type;
		vec3d m_color;
		vec3d m_position;
		vec3d m_direction;
		float m_range;
		float m_falloff;
		float m_attenuation0;
		float m_attenuation1;
		float m_attenuation2;
		float m_theta;
		float m_phi;
		float m_theta_dot;
		float m_phi_dot;
		float m_over_theta_dot_minus_dot;
		uint32_t m_flags;
		float m_range_squared;
	};

	struct render_instance_t {
		uint8_t m_alpha;
	};

	struct flash_light_instance_t {
		material_t* m_debug_material;
		char pad[248];
		matrix3x4_t m_world_to_texture;
		void* m_flash_depth_light_texture;
	};

	struct studio_array_data_t {
		studiohdr_t* m_studio_hdr;
		studiohwdata_t* m_hardware_data;
		void* m_instance_data;
		int m_count;
	};

	struct studio_model_array_info2_t {
		int m_flash_count;
		flash_light_instance_t* m_flashlights;
	};

	struct studio_model_array_info_t : public studio_model_array_info2_t {
		studiohdr_t* m_studio_hdr;
		studiohwdata_t* m_hardware_data;
	};

	struct model_render_system_data_t {
		void* m_renderable;
		void* m_renderable_model;
		render_instance_t m_instance_data;
	};

	struct shader_stencil_state_t {
		bool m_enable;
		e_shader_stencil_op m_fail_op;
		e_shader_stencil_op m_z_fail_op;
		e_shader_stencil_op m_pass_op;
		e_shader_stencil_func m_compare_func;
		int m_reference_value;
		uint32_t m_test_mask;
		uint32_t m_write_masl;

		__forceinline shader_stencil_state_t() {
			m_enable = false;
			m_fail_op = m_z_fail_op = m_pass_op = shader_stencilop_keep;
			m_compare_func = shader_stencilfunc_always;
			m_reference_value = 0;
			m_test_mask = m_write_masl = 0xFFFFFFFF;
		}
	};

	struct studio_shadow_array_instance_data_t {
		int m_lod;
		int m_body;
		int m_skin;
		matrix3x4_t* m_pose_to_world;
		float* m_flex_weights;
		float* m_delayed_flex_weights;
	};

	struct color_mesh_info_t {
		void* m_mesh;
		void* m_all_allocator;
		int m_vert_offset;
		int m_num_verts;
	};

	struct material_lightning_state_t {
		vec3d m_aimbient_cube[6];
		vec3d m_lightning_origin;
		int m_light_count;
		light_desc_t m_light_desc[4];
	};

	struct studio_array_instance_data_t : public studio_shadow_array_instance_data_t {
		material_lightning_state_t* m_light_state;
		material_lightning_state_t* m_decal_light_state;
		void* m_env_cubemap_textuer;
		void* m_decals;
		uint32_t m_flash_usage;
		shader_stencil_state_t* m_stencil_state;
		color_mesh_info_t* m_color_mesh_info;
		bool m_mesh_has_light_only;
		vec4d m_diffuse_modulation;
	};

	struct model_list_mode_t {
		model_render_system_data_t m_entry;
		int32_t m_initial_list_index : 24;
		uint32_t m_bone_merge : 1;
		int32_t m_lod : 7;
		shader_stencil_state_t* m_stencil_state;
		model_list_mode_t* m_next;
	};

	struct render_model_info_t : public studio_array_instance_data_t {
		model_render_system_data_t m_entry;
		unsigned short m_instance;
		matrix3x4_t* m_bone_to_world;
		uint32_t m_list_idx : 24;
		uint32_t m_setup_bones_only : 1;
		uint32_t m_bone_merge : 1;
	};

	struct model_list_by_type_t : public studio_model_array_info_t {
		e_renderable_lightning_model m_light_model;
		const model_t* m_model;
		model_list_mode_t* m_first_node;
		int m_count;
		int m_setup_bone_count;
		uint32_t m_parent_depth : 31;
		uint32_t m_wants_stencil : 1;
		render_model_info_t* m_render_models;
		model_list_by_type_t* m_next_lightning_model;

		__forceinline model_list_by_type_t& operator=(const model_list_by_type_t& rhs) {
			std::memcpy(this, &rhs, sizeof(model_list_by_type_t));
			return *this;
		}

		__forceinline model_list_by_type_t() {}

		__forceinline model_list_by_type_t(const model_list_by_type_t& rhs) {
			std::memcpy(this, &rhs, sizeof(model_list_by_type_t));
		}
	};

	struct material_system_t {
		VFUNC(create_material(const char* name, key_values_t* key),
			  material_t*(__thiscall*)(decltype(this), const char*, key_values_t*), 83,
			  name, key);

		VFUNC(find_material(const char* material_name, const char* group_name, bool complain = true, const char* complain_prefix = NULL),
			  material_t*(__thiscall*)(decltype(this), const char*, const char*, bool, const char*), 84,
			  material_name, group_name, complain, complain_prefix);

		VFUNC(get_render_context(), void*(__thiscall*)(decltype(this)), 115);
	};
} // namespace sdk