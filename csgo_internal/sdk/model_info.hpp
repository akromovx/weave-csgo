#pragma once
#include "../src/utils/displacement.hpp"
#include "../src/utils/matrix.hpp"
#include "../src/utils/utils.hpp"
#include "../src/utils/vector.hpp"

namespace sdk {
	struct mstudiobbox_t {
		int m_bone;
		int m_group;
		vec3d m_min;
		vec3d m_max;
		int m_name_id;
		vec3d m_rotation;
		float m_radius;
		char pad[0x10];
	};

	enum e_bones : int {
		bone_pelvis = 0,
		lean_root,
		cam_driver,
		bone_hip,
		bone_lower_spinal_column,
		bone_middle_spinal_column,
		bone_upper_spinal_column,
		bone_neck,
		bone_head,
	};

	struct radian_euler_t {
		__forceinline radian_euler_t(void) {}
		__forceinline radian_euler_t(float X, float Y, float Z) {
			x = X;
			y = Y;
			z = Z;
		}

		__forceinline void init(float ix = 0.0f, float iy = 0.0f, float iz = 0.0f) {
			x = ix;
			y = iy;
			z = iz;
		}

		__forceinline float* base() { return &x; }
		__forceinline const float* base() const { return &x; }
		float x, y, z;
	};

	using mdl_handle_t = uint32_t;

	struct brushdata_t {
		void* m_shared;
		int m_first_model_surface;
		int m_num_model_surfaces;
		int m_light_style_last_computed_frame;
		unsigned short m_light_style_index;
		unsigned short m_light_style_count;
		unsigned short m_render_handle;
		unsigned short m_first_node;
	};

	struct spritedata_t {
		int m_num_frames;
		int m_width;
		int m_height;
		void* m_sprite;
	};

	struct model_t {
		void* m_handle;
		char m_name[260];
		int32_t m_load_flags;
		int32_t m_server_count;
		int32_t m_type;
		int32_t m_flags;
		vec3d m_mins;
		vec3d m_maxs;
		float m_radius;
		void* m_key_values;

		union {
			brushdata_t m_brush;
			mdl_handle_t m_studio;
			spritedata_t m_sprite;
		};
	};

	struct mstudiohitboxset_t {
		int m_name_index;
		int m_num_hitboxes;
		int m_hitbox_index;

		__forceinline mstudiobbox_t* hitbox(int i) const {
			return (mstudiobbox_t*)(((BYTE*)this) + m_hitbox_index) + i;
		}
	};

	struct mstudiobone_t {
		int m_name_index;
		int m_parent;
		int m_bone_controller[6];
		vec3d m_pos;
		quaternion_t m_quat;
		radian_euler_t m_rot;
		vec3d m_posscale;
		vec3d m_rotscale;

		matrix3x4_t m_pose_to_bone;
		quaternion_t m_alignment;
		int m_flags;
		int m_proc_type;
		int m_proc_index;
		mutable int m_physics_bone;
		int m_surface_prop_index;

		int m_contents;
		int m_surface_prop_lookup;
		int unused[7];

		__forceinline char* const pszName() const {
			return ((char*)this) + m_name_index;
		}

		__forceinline void* procedure() const {
			if (m_proc_index == 0)
				return NULL;
			else
				return (void*)(((BYTE*)this) + m_proc_index);
		};

		__forceinline char* const surface_prop() const {
			return ((char*)this) + m_surface_prop_index;
		}
		__forceinline int get_surface_prop(void) const {

			return m_surface_prop_lookup;
		}
	};

	struct mstudiobonecontroller_t {
		int m_bone;
		int m_type;
		float m_start;
		float m_end;
		int m_rest;
		int m_input_field;
		int unused[8];
	};

	struct mstudioattachment_t {
		int m_name_index;
		uint32_t m_flags;
		int m_local_bone;
		matrix3x4_t m_local;
		int unused[8];

		__forceinline char* const pszName() const {
			return ((char*)this) + m_name_index;
		}
	};

	struct mstudioiklink_t {
		int bone;
		vec3d knee_dir;
		vec3d unused0;

		mstudioiklink_t() {}

	private:
		mstudioiklink_t(const mstudioiklink_t& vOther);
	};

	struct mstudioikchain_t {
		int sznameindex;
		inline char* const name(void) const { return ((char*)this) + sznameindex; }
		int linktype;
		int numlinks;
		int linkindex;
		inline mstudioiklink_t* link(int i) const { return (mstudioiklink_t*)(((byte*)this) + linkindex) + i; }
	};

	struct studiohdr_t {
		int m_id{};
		int m_version{};
		long m_checksum{};
		char m_name[64]{};
		int m_length{};
		vec3d m_eye_pos{};
		vec3d m_ilum_pos{};
		vec3d m_hull_min{};
		vec3d m_hull_max{};
		vec3d m_bb_min{};
		vec3d m_bb_max{};
		int m_flags{};
		int m_num_bones{};
		int m_bone_id{};
		int m_bone_controllers{};
		int m_bone_controller_index{};
		int m_hitbox_sets{};
		int m_hitbox_set_index{};
		int m_local_anim{};
		int m_local_anim_index{};
		int m_local_seq{};
		int m_local_seq_index{};
		int m_activity_list_version{};
		int m_events_indexed{};
		int m_textures{};
		int m_texture_index{};
		int m_numcdtextures;
		int m_cdtextureindex;
		int m_numskinref{};
		int m_numskinfamilies{};
		int m_skinindex{};
		int m_numbodyparts{};
		int m_bodypartindex{};
		int m_numlocalattachments{};
		int m_localattachmentindex{};
		int m_numlocalnodes{};
		int m_localnodeindex{};
		int m_localnodenameindex{};
		int m_numflexdesc{};
		int m_flexdescindex{};
		int m_numflexcontrollers{};
		int m_flexcontrollerindex{};
		int m_numflexrules{};
		int m_flexruleindex{};
		int m_numikchains{};
		int m_ikchainindex{};
		int m_nummouths{};
		int m_mouthindex{};
		int m_numlocalposeparameters{};
		int m_localposeparamindex{};

		__forceinline mstudiobbox_t* hitbox(int i, int set) {
			mstudiohitboxset_t* s = hitbox_set(set);
			if (!s)
				return NULL;

			return s->hitbox(i);
		};

		__forceinline const char* get_name() const {
			return m_name;
		}

		__forceinline mstudiohitboxset_t* hitbox_set(int i) {
			if (i > m_hitbox_sets)
				return nullptr;

			return (mstudiohitboxset_t*)((uint8_t*)this + m_hitbox_set_index) + i;
		}

		__forceinline mstudiobone_t* get_bone(int i) {
			if (i > m_num_bones)
				return nullptr;

			return (mstudiobone_t*)((uint8_t*)this + m_bone_id) + i;
		}

		__forceinline mstudioikchain_t* ik_chain(int i) const {
			return (mstudioikchain_t*)(((std::uint8_t*)this) + m_ikchainindex) + i;
		}
	};

	struct virtualgroup_t {
		void* m_cache;
		utils::utl_vector_t<int> m_bone_map;
		utils::utl_vector_t<int> m_master_none;
		utils::utl_vector_t<int> m_master_seq;
		utils::utl_vector_t<int> m_master_anim;
		utils::utl_vector_t<int> m_master_attachment;
		utils::utl_vector_t<int> m_master_pose;
		utils::utl_vector_t<int> m_master_node;

		__forceinline const studiohdr_t* get_studio_hdr(void) const { return (studiohdr_t*)m_cache; }
	};

	struct virtualsequence_t {
		int m_flags;
		int m_activity;
		int m_group;
		int m_index;
	};

	struct virtualgeneric_t {
		int m_group;
		int m_index;
	};

	struct virtualmodel_t {
		char pad_8[0x8];
		utils::utl_vector_t<virtualsequence_t> m_seq;
		utils::utl_vector_t<virtualgeneric_t> m_anim;
		utils::utl_vector_t<virtualgeneric_t> m_attachment;
		utils::utl_vector_t<virtualgeneric_t> m_pose;
		utils::utl_vector_t<virtualgroup_t> m_group;
		utils::utl_vector_t<virtualgeneric_t> m_node;
		utils::utl_vector_t<virtualgeneric_t> m_iklock;
		utils::utl_vector_t<unsigned short> m_autoplay_sequences;
	};

	struct studio_hdr_t {
		mutable studiohdr_t* m_studio_hdr;
		mutable virtualmodel_t* m_vmodel;
		char pad_unknown[0x4];
		mutable utils::utl_vector_t<const studiohdr_t*> m_studio_hdr_cache;
		mutable int m_frame_unlock_counter;
		int* m_frame_unlock_counter_ptr;
		char pad_mutex[0x8];
		utils::utl_vector_t<int> m_bone_flags;
		utils::utl_vector_t<int> m_bone_parent;

		__forceinline int num_bones(void) const {
			return m_studio_hdr->m_num_bones;
		};
		__forceinline mstudiobone_t* bone(int i) const {
			return m_studio_hdr->get_bone(i);
		};

		__forceinline virtualmodel_t* get_virtual_model(void) const {
			return m_vmodel;
		};
		__forceinline const studiohdr_t* group_studio_hdr(int i) {
			const studiohdr_t* studio_hdr = m_studio_hdr_cache[i];

			if (!studio_hdr)
				studio_hdr = m_vmodel->m_group[i].get_studio_hdr();

			return studio_hdr;
		}
	};

	struct studiohwdata_t {
		int m_root_lod;
		int m_mum_lods;
		void* m_lods;
		int m_num_studio_meshes;

		__forceinline float lod_metricts(float unit_sphere_size) const {
			return (unit_sphere_size != 0.0f) ? (100.0f / unit_sphere_size) : 0.0f;
		}
		__forceinline int get_lod_for_metrics(float lod_metric) const {
			return 0;
		}
	};

	struct vmodel_info_t {
		VFUNC(get_model(int index), void*(__thiscall*)(void*, int), 1, index);
		VFUNC(get_model_index(const char* name), int(__thiscall*)(void*, const char*), 2, name);
		VFUNC(get_model_name(const char* mod), char*(__thiscall*)(void*, const char*), 3, mod);
		VFUNC(get_studio_model(const model_t* mod), studiohdr_t*(__thiscall*)(void*, const model_t* mod), 32, mod);
	};

	struct vmodel_info_client_t : vmodel_info_t {};

	struct virtualterrainparams_t {
		int index;
	};
} // namespace sdk