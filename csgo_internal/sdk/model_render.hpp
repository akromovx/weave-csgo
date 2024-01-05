#pragma once
#include "../src/utils/displacement.hpp"
#include "../src/utils/matrix.hpp"
#include "../src/utils/utils.hpp"

namespace sdk {
	inline constexpr int max_bones = 128;

	enum e_material_flags {
		material_var_debug = (1 << 0),
		material_var_no_debug_override = (1 << 1),
		material_var_no_draw = (1 << 2),
		material_var_use_in_fillrate_mode = (1 << 3),
		material_var_vertexcolor = (1 << 4),
		material_var_vertexalpha = (1 << 5),
		material_var_selfillum = (1 << 6),
		material_var_additive = (1 << 7),
		material_var_alphatest = (1 << 8),
		material_var_multipass = (1 << 9),
		material_var_znearer = (1 << 10),
		material_var_model = (1 << 11),
		material_var_flat = (1 << 12),
		material_var_nocull = (1 << 13),
		material_var_nofog = (1 << 14),
		material_var_ignorez = (1 << 15),
		material_var_decal = (1 << 16),
		material_var_envmapsphere = (1 << 17),
		material_var_noalphamod = (1 << 18),
		material_var_envmapcameraspace = (1 << 19),
		material_var_basealphaenvmapmask = (1 << 20),
		material_var_translucent = (1 << 21),
		material_var_normalmapalphaenvmapmask = (1 << 22),
		material_var_needs_software_skinning = (1 << 23),
		material_var_opaquetexture = (1 << 24),
		material_var_envmapmode = (1 << 25),
		material_var_suppress_decals = (1 << 26),
		material_var_halflambert = (1 << 27),
		material_var_wireframe = (1 << 28),
		material_var_allowalphatocoverage = (1 << 29),
		material_var_ignore_alpha_modulation = (1 << 30),
		material_var_vertexfog = (1 << 31),
	};

	struct model_t;
	struct renderable_t;
	struct key_values_t;
	struct studiohwdata_t;

	struct draw_model_state_t {
		void* m_studio_hdr;
		studiohwdata_t* m_studio_hdr_data;
		renderable_t* m_renderable;
		const matrix3x4_t* m_model_to_world;
		void* m_decals;
		int m_draw_flags;
		int m_lod;

		__forceinline draw_model_state_t operator=(const draw_model_state_t& other) {
			std::memcpy(this, &other, sizeof(draw_model_state_t));
			return *this;
		}
	};

	struct renderable_info_t {
		renderable_t* m_renderable;
		void* m_alpha_roperty;
		int32_t m_enum_count;
		int32_t m_render_frame;
		uint16_t m_first_shadow;
		uint16_t m_leaf_list;
		int16_t m_area;
		uint16_t m_flags;
		uint16_t m_render_in_fast_reflection : 1;
		uint16_t m_disable_shadow_depth_rendering : 1;
		uint16_t m_disable_csm_rendering : 1;
		uint16_t m_disable_shadow_depth_caching : 1;
		uint16_t m_splitscreen_enabled : 2;
		uint16_t m_translucency_type : 2;
		uint16_t m_model_type : 8;
		vec3d m_bloated_abs_mins;
		vec3d m_bloated_abs_maxs;
		vec3d m_abs_mins;
		vec3d m_abs_maxs;
	};

	struct model_render_info_t {
		vec3d m_origin;
		vec3d m_angles;
		char pad[4];
		renderable_t* m_renderable;
		const model_t* m_model;
		const matrix3x4_t* m_model_to_world;
		const matrix3x4_t* m_lightning_offset;
		const vec3d* m_lightning_origin;
		int m_flags;
		int m_entity_index;
		int m_skin;
		int m_body;
		int m_hitboxset;
		unsigned short m_instance;

		inline model_render_info_t operator=(const model_render_info_t& other) {
			memcpy(this, &other, sizeof(model_render_info_t));
			return *this;
		}
	};

	struct material_var_t {
		VFUNC(set_vec_value(float value), void(__thiscall*)(decltype(this), float), 4, value);
		VFUNC(set_vec_value(int value), void(__thiscall*)(decltype(this), int), 5, value);
		VFUNC(set_vec_value(const char* value), void(__thiscall*)(decltype(this), const char*), 6, value);
		VFUNC(set_vec_value(vec3d vec), void(__thiscall*)(decltype(this), float, float, float), 11, vec.x, vec.y, vec.z);
	};

	struct material_t {
		virtual const char* get_name() const = 0;
		virtual const char* get_texture_group_name() const = 0;

		virtual int get_preview_image_properties(int* width, int* height, void* image_format, bool* translucent) const = 0;
		virtual int get_preview_image(unsigned char* data, int width, int height, void* image_format) const = 0;

		virtual int get_mapping_width() = 0;
		virtual int get_mapping_height() = 0;

		virtual int get_num_animation_frames() = 0;

		virtual bool in_material_page() = 0;

		virtual void get_material_offset(float* offset) = 0;
		virtual void get_material_scale(float* scale) = 0;

		virtual material_t* get_material_page() = 0;
		virtual material_var_t* find_var(const char* name, bool* found, bool complain = true) = 0;

		virtual void increment_reference_count() = 0;
		virtual void decrement_reference_count() = 0;

		virtual int get_enum_id() const = 0;

		virtual void get_low_res_clr_sample(float s, float t, float* color) const = 0;

		virtual void recompute_state_snapshots() = 0;

		virtual bool is_translucent() = 0;
		virtual bool is_alpha_tested() = 0;
		virtual bool is_vertex_lit() = 0;

		virtual int get_vertex_format() const = 0;

		virtual bool has_proxy() const = 0;
		virtual bool use_cube_map() = 0;

		virtual bool needs_tangent_space() = 0;
		virtual bool needs_power_of_two_frame_buffer_texture(bool check_specific_to_this_frame = true) = 0;
		virtual bool needs_full_frame_buffer_texture(bool check_specific_to_this_frame = true) = 0;
		virtual bool needs_software_skinning() = 0;

		virtual void alpha_modulate(float alpha) = 0;
		virtual void color_modulate(float r, float g, float b) = 0;

		virtual void set_material_var_flag_virtual(e_material_flags flag, bool on) = 0;
		virtual bool get_material_var_flag(e_material_flags flag) const = 0;

		virtual void get_reflectivity(vec3d& reflect) = 0;

		virtual bool get_property_flag(int type) = 0;

		virtual bool is_two_sided() = 0;

		virtual void set_shader(const char* shader_name) = 0;

		virtual int get_num_passes() = 0;
		virtual int get_texture_memory_bytes() = 0;

		virtual void refresh() = 0;

		virtual bool needs_lightmap_blend_alpha() = 0;
		virtual bool needs_software_lightning() = 0;

		virtual int shader_param_count() const = 0;
		virtual material_var_t** get_shader_params() = 0;

		virtual bool is_error_material() const = 0;
		virtual void unused() = 0;

		virtual float get_alpha_modulation() = 0;
		virtual void get_color_modulation(float* r, float* g, float* b) = 0;
		virtual bool is_translucent_under_modulation(float alpha_modulation = 1.0f) const = 0;

		virtual material_var_t* find_var_fast(char const* name, unsigned int* token) = 0;

		virtual void set_shader_and_params(key_values_t* pKeyValues) = 0;
		virtual const char* get_shader_name() const = 0;

		virtual void delete_if_unreferenced() = 0;

		virtual bool is_spite_card() = 0;

		virtual void call_bind_proxy(void* data) = 0;

		virtual void refresh_preversing_material_vars() = 0;

		virtual bool was_reloaded_from_whitelist() = 0;

		virtual bool set_tem_excluded(bool set, int excluded_dimension_limit) = 0;

		virtual int get_reference_count() const = 0;

		__forceinline void set_material_var_flag(e_material_flags flag, bool on) {
			utils::vfunc<void(__thiscall*)(void*, e_material_flags, bool)>(this, 29)(this, flag, on);
		}

		__forceinline void add_ref() { increment_reference_count(); }
		__forceinline void release() { decrement_reference_count(); }
	};

	struct model_render_t {
		VFUNC(forced_material_override(material_t* mat, int type = 0, int overrides = 0),
			  void(__thiscall*)(decltype(this), material_t*, int, int),
			  1, mat, type, overrides);

		VFUNC(draw_model_execute(void* ctx,
								 const draw_model_state_t& state,
								 const model_render_info_t& info,
								 matrix3x4_t* bone_to_world = NULL),
			  void(__thiscall*)(decltype(this),
								void*,
								const draw_model_state_t&,
								const model_render_info_t&,
								matrix3x4_t*),
			  21, ctx, state, info, bone_to_world);
	};
} // namespace sdk