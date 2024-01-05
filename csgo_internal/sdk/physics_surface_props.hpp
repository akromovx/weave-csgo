#pragma once
#include <cstdint>
#include <cstddef>

namespace sdk {
	// @credits: https://github.com/ValveSoftware/source-sdk-2013/blob/master/mp/src/public/vphysics_interfaceV30.h
	struct surfacephysicsparams_t {
		float m_friction;
		float m_elasticity;
		float m_density;
		float m_thickness;
		float m_dampening;
	};

	struct surfaceaudioparams_t {
		float m_reflectivity;			// like elasticity, but how much sound should be reflected by this surface
		float m_hardness_factor;		// like elasticity, but only affects impact sound choices
		float m_roughness_factor;		// like friction, but only affects scrape sound choices
		float m_rough_treshold;			// surface roughness > this causes "rough" scrapes, < this causes "smooth" scrapes
		float m_hard_treshold;			// surface hardness > this causes "hard" impacts, < this causes "soft" impacts
		float m_hard_velocity_treshold; // collision velocity > this causes "hard" impacts, < this causes "soft" impacts
		float m_high_pitch_occlusion;	// a value betweeen 0 and 100 where 0 is not occluded at all and 100 is silent (except for any additional reflected sound)
		float m_mid_pitch_occlusion;
		float m_low_pitch_occlusion;
	};

	struct surfacesoundnames_t {
		uint16_t m_walk_step_left;
		uint16_t m_walk_step_right;
		uint16_t m_run_step_left;
		uint16_t m_run_step_right;
		uint16_t m_impact_soft;
		uint16_t m_impact_hard;
		uint16_t m_scrape_smooth;
		uint16_t m_scrape_rough;
		uint16_t m_bullet_impact;
		uint16_t m_rolling;
		uint16_t m_break_sound;
		uint16_t m_strain_sound;
	};

	struct surfacesoundhandles_t {
		uint16_t m_walk_step_left;
		uint16_t m_walk_step_right;
		uint16_t m_run_step_left;
		uint16_t m_run_step_right;
		uint16_t m_impact_soft;
		uint16_t m_impact_hard;
		uint16_t m_scrape_smooth;
		uint16_t m_scrape_rough;
		uint16_t m_bullet_impact;
		uint16_t m_rolling;
		uint16_t m_break_sound;
		uint16_t m_strain_sound;
	};

	using material_handle_t = uint16_t;
	struct surfacegameprops_t {
		float m_max_speed_factor;
		float m_jump_factor;
		float m_penetration_modifier;
		float m_damage_modifier;
		material_handle_t m_material;
		uint8_t m_climable;
		uint8_t pad0[0x4];
	};

	struct surface_data_t {
		surfacephysicsparams_t m_physics;
		surfaceaudioparams_t m_audio;
		surfacesoundnames_t m_sounds;
		surfacegameprops_t m_game;
		surfacesoundhandles_t m_soundhandles;
	};

	struct physics_surface_props_t {
		virtual ~physics_surface_props_t() {}
		virtual int parse_surface_data(const char* file_name, const char* text_file) = 0;
		virtual int surface_prop_count() const = 0;
		virtual int get_surface_index(const char* surface_prop_name) const = 0;
		virtual void get_physics_properties(int surface_data_index, float* density, float* thickness, float* friction, float* elasticity) const = 0;
		virtual surface_data_t* get_surface_data(int surface_data_index) = 0;
		virtual const char* get_string(unsigned short string_table_index) const = 0;
		virtual const char* get_prop_name(int surface_data_index) const = 0;
		virtual void set_world_material_index_table(int* map_array, int map_size) = 0;
		virtual void get_physics_parameters(int surface_data_index, surfacephysicsparams_t* params) const = 0;
	};
} // namespace sdk