#pragma once
#include "../src/game/netvar_manager.hpp"
#include "../src/utils/displacement.hpp"
#include "../src/utils/math.hpp"
#include "animation_state.hpp"
#include "base_handle.hpp"
#include "input.hpp"
#include "item_definitions.hpp"

namespace sdk {
	struct animation_state_t;
	struct client_class_t;
	struct player_info_t;
	struct studiohdr_t;
	struct model_t;
	struct studio_hdr_t;
	struct game_trace_t;
	struct ray_t;
	struct base_entity_t;

	using trace_t = game_trace_t;

	using local_data_t = std::array<uint8_t, 0x210>;

	enum class e_client_frame_stage {
		frame_undefined = -1, // (haven't run any frames yet)
		frame_start,

		// a network packet is being recieved
		frame_net_update_start,
		// data has been received and we're going to start calling postdataupdate
		frame_net_update_postdataupdate_start,
		// data has been received and we've called postdataupdate on all data recipients
		frame_net_update_postdataupdate_end,
		// we've received all packets, we can now do interpolation, prediction, etc..
		frame_net_update_end,

		// we're about to start rendering the scene
		frame_render_start,
		// we've finished rendering the scene.
		frame_render_end
	};

	enum e_player_flags {
		fl_onground = (1 << 0),	  // At rest / on the ground
		fl_ducking = (1 << 1),	  // Player flag -- Player is fully crouched
		fl_waterjump = (1 << 3),  // player jumping out of water
		fl_ontrain = (1 << 4),	  // Player is _controlling_ a train, so movement commands should be ignored on client during prediction.
		fl_inrain = (1 << 5),	  // Indicates the entity is standing in rain
		fl_frozen = (1 << 6),	  // Player is frozen for 3rd person camera
		fl_atcontrols = (1 << 7), // Player can't move, but keeps key inputs for controlling another entity
		fl_client = (1 << 8),	  // Is a player
		fl_fakeclient = (1 << 9), // Fake client, simulated server side; don't send network messages to them
		fl_inwater = (1 << 10)	  // In water
	};

	enum e_move_type {
		type_none = 0,
		type_isometric,
		type_walk,
		type_step,
		type_fly,
		type_flygravity,
		type_vphysics,
		type_push,
		type_noclip,
		type_ladder,
		type_observer,
		type_custom,
		type_last = type_custom,
		type_max_bits = 4,
		max_move_type
	};

	enum e_player_efl_flags {
		efl_ditry_abstransform = (1 << 11),
		efl_ditry_absvelocity = (1 << 12),
		efl_ditry_absangvelocity = (1 << 13),
		efl_setting_up_bones = (1 << 3),
	};

	enum e_bone_mask {
		bone_used_mask = 0x000FFF00,
		bone_used_by_anything = 0x000FFF00,
		bone_used_by_hitbox = 0x00000100,
		bone_used_by_attachment = 0x00000200,
		bone_used_by_vertex_mask = 0x0003FC00,
		bone_used_by_vertex_lod0 = 0x00000400,
		bone_used_by_vertex_lod1 = 0x00000800,
		bone_used_by_vertex_lod2 = 0x00001000,
		bone_used_by_vertex_lod3 = 0x00002000,
		bone_used_by_vertex_lod4 = 0x00004000,
		bone_used_by_vertex_lod5 = 0x00008000,
		bone_used_by_vertex_lod6 = 0x00010000,
		bone_used_by_vertex_lod7 = 0x00020000,
		bone_used_by_bone_merge = 0x00040000,
		bone_always_setup = 0x00080000
	};

	enum class e_hitgroup : int32_t {
		unknown = -1,
		generic = 0,
		head = 1,
		chest = 2,
		stomach = 3,
		leftarm = 4,
		rightarm = 5,
		leftleg = 7,
		rightleg = 6,
		neck = 8,
		gear = 10,
	};

	enum class e_hitbox : int32_t {
		head = 0,
		neck,
		pelvis,
		stomach,
		lower_chest,
		chest,
		upper_chest,
		right_thigh,
		left_thigh,
		right_shin,
		left_shin,
		right_foot,
		left_foot,
		right_hand,
		left_hand,
		right_upper_arm,
		right_forearm,
		left_upper_arm,
		left_forearm,
		hitbox_max
	};

	struct renderable_t;
	struct networkable_t;

	struct cs_weapon_info_t {
		uint32_t m_vtable;
		char m_console_name[4];
		char pad_0008[12];
		int m_max_ammo_1;
		char pad_0018[12];
		int m_max_ammo_2;
		char m_pad_0028[4];
		char m_world_model[4];
		char m_view_model[4];
		char m_droped_model[4];
		char pad_0038[80];
		char* m_hud_name;
		char m_weapon_name[4];
		char pad_0090[56];
		int m_weapon_type;
		char pad_00CC[4];
		int m_weapon_price;
		int m_kill_award;
		char* m_animation_prefix;
		float m_cycle_time;
		float m_cycle_time_alt;
		float m_time_to_idle;
		float m_idle_interval;
		bool m_full_auto;
		char pad_0x00e5[3];
		int m_damage;
		float m_crosshair_delta_distance;
		float m_armor_ratio;
		int m_bullets;
		float m_penetration;
		float m_flinch_velocity_modifier_large;
		float m_flinch_velocity_modifier_small;
		float m_range;
		float m_range_modifier;
		char pad_0114[32];
		float m_max_speed;
		float m_max_speed_alt;
		char pad_013C[12];
		float m_inaccuracy_crouch;
		float m_inaccuracy_crouch_alt;
		float m_inaccuracy_stand;
		float m_inaccuracy_stand_alt;
		float m_inaccuracy_jump;
		float m_inaccuracy_jump_alt;
		float m_inaccuracy_land;
		float m_inaccuracy_land_alt;
		char pad_0168[96];
		bool unk;
		char pad_0169[4];
		bool m_hide_viewmodel_in_zoom;
	};

	struct bone_accessor_t {
		__forceinline matrix3x4_t* get_bone_array_for_write() const { return m_bones; }
		__forceinline void set_bone_array_for_write(matrix3x4_t* bone_array) { m_bones = bone_array; }

		alignas(16) matrix3x4_t* m_bones;
		int m_readable_bones;
		int m_writable_bones;
	};

	struct collideable_t {
		virtual base_entity_t* get_entity_handle() = 0;
		virtual vec3d& mins() const = 0;
		virtual vec3d& maxs() const = 0;
		virtual void world_space_trigger_bounds(vec3d* mins, vec3d* maxs) const = 0;
		virtual bool test_collision(const ray_t& ray, unsigned int mask, trace_t& trace) = 0;
		virtual bool test_hitboxes(const ray_t& ray, unsigned int mask, trace_t& trace) = 0;
		virtual int get_collision_model_index() = 0;
		virtual const model_t* get_collision_model() = 0;
		virtual vec3d& get_collision_origin() const = 0;
		virtual vec3d& get_collision_angles() const = 0;
		virtual const matrix3x4_t& collision_to_world_transform() const = 0;
		virtual int get_solid() const = 0;
		virtual int get_solid_flags() const = 0;
		virtual void* get_client_unknown() = 0;
		virtual int get_collision_group() const = 0;
		virtual void world_space_surrounding_bounds(vec3d* mins, vec3d* maxs) = 0;
		virtual bool should_touch_trigger(int flags) const = 0;
		virtual const matrix3x4_t* get_root_parent_to_world_transform() const = 0;
	};

	struct handle_entity_t {
		virtual ~handle_entity_t() {}
		virtual void set_ref_handle(const base_handle_t& handle) = 0;
		virtual const base_handle_t& get_ref_handle() const = 0;
	};

	struct unknown_t : handle_entity_t {
		virtual collideable_t* get_collideable() = 0;
		virtual networkable_t* get_networkable_entity() = 0;
		virtual renderable_t* get_renderable_entity() = 0;
		virtual void* get_client_entity() = 0;
		virtual void* get_base_entity() = 0;
		virtual void* get_thinkable_entity() = 0;
		virtual void* get_client_alpha_property() = 0;
	};

	struct renderable_t {
		VFUNC(unknown(), unknown_t*(__thiscall*)(decltype(this)), 0);
		VFUNC(render_origin(), vec3d&(__thiscall*)(decltype(this)), 1);
		VFUNC(render_angles(), vec3d&(__thiscall*)(decltype(this)), 2);

		VFUNC(get_model(), model_t*(__thiscall*)(decltype(this)), 8);

		VFUNC(setup_bones(matrix3x4_t* bones, int max_bones, int mask, float curtime),
			  bool(__thiscall*)(decltype(this), matrix3x4_t*, int, int, float), 13, bones, max_bones, mask, curtime);
	};

	struct networkable_t {
		VFUNC(client_class(), client_class_t*(__thiscall*)(decltype(this)), 2);
		VFUNC(pre_data_update(int type), void(__thiscall*)(decltype(this), int), 6, type);
		VFUNC(dormant(), bool(__thiscall*)(decltype(this)), 9);
		VFUNC(index(), int(__thiscall*)(decltype(this)), 10);
	};

	struct base_entity_t {
		OFFSET_PTR(renderable, renderable_t, sizeof(uintptr_t));
		OFFSET_PTR(networkable, networkable_t, sizeof(uintptr_t) * 2u);

		//VFUNC(get_abs_origin(), vec3d(__thiscall*)(decltype(this)), 10);
		OFFSET(get_abs_origin, vec3d&, 0xA0);
		VFUNC(get_abs_angles(), vec3d&(__thiscall*)(decltype(this)), 11);

		VFUNC(is_player(), bool(__thiscall*)(decltype(this)), 158);
		VFUNC(is_weapon(), bool(__thiscall*)(decltype(this)), 166);

		VFUNC(get_pred_descmap(), datamap_t*(__thiscall*)(decltype(this)), 17);
		VFUNC(get_attachment(int index, vec3d& origin), bool(__thiscall*)(decltype(this), int, vec3d&), 84, index, std::ref(origin));

		NETVAR(mins, vec3d&, "DT_BaseEntity", "m_vecMins");
		NETVAR(maxs, vec3d&, "DT_BaseEntity", "m_vecMaxs");

		NETVAR(team, int, "DT_BaseEntity", "m_iTeamNum");
		NETVAR(model_index, int&, "DT_BaseEntity", "m_nModelIndex");
		NETVAR(moveparent_handle, base_handle_t, "DT_BaseEntity", "moveparent");
		NETVAR(owner, base_handle_t, "DT_BaseEntity", "m_hOwnerEntity");

		NETVAR_OFFSET(coordinate_frame, matrix3x4_t&, "DT_BaseEntity", "m_CollisionGroup", -48);

		OFFSET(origin, vec3d&, 0x138);
		OFFSET(lifestate, uint8_t, 0x25F);
		OFFSET(health, int, 0x100);
		OFFSET(flags, flags_t&, 0x104);
		OFFSET(spotted, bool&, 0x93D);
		OFFSET(velocity, vec3d&, 0x114);
		OFFSET(base_velocity, vec3d&, 0x120);
		OFFSET(movetype, e_move_type&, 0x25C);
		OFFSET(spawn_time, float&, 0x103C0);

		OFFSET(collision_change_height, float&, 0x9920);
		OFFSET(collision_change_time, float&, 0x9924)

		DATAMAP(eflags, flags_t&, "m_iEFlags");
		DATAMAP(vec_abs_origin, vec3d&, "m_vecAbsOrigin");

		__forceinline int index() { return networkable()->index(); }
		__forceinline bool dormant() { return networkable()->dormant(); }
		__forceinline client_class_t* client_class() { return networkable()->client_class(); }
		__forceinline void pre_data_update(int type) { return networkable()->pre_data_update(type); }

		__forceinline vec3d& render_origin() { return renderable()->render_origin(); }
		__forceinline vec3d& render_angles() { return renderable()->render_angles(); }
		__forceinline model_t* get_model() { return renderable()->get_model(); }

		__forceinline const base_handle_t& get_ref_handle() { return renderable()->unknown()->get_ref_handle(); }
		__forceinline collideable_t* collideable() { return renderable()->unknown()->get_collideable(); }

		__forceinline bool setup_bones(matrix3x4_t* bones, int max_bones, int mask, float curtime) {
			return renderable()->setup_bones(bones, max_bones, mask, curtime);
		}
		__forceinline base_entity_t* moveparent() {
			return (base_entity_t*)moveparent_handle().get();
		}

		VFUNC(get_layer_sequence_cycle_rate(animation_layer_t* layer, int sequence), float(__thiscall*)(decltype(this), animation_layer_t*, int), 223, layer, sequence);
	};

	struct base_animating_t : base_entity_t {
		NETVAR(sequence, int&, "DT_BaseAnimating", "m_nSequence");
		NETVAR(cycle, float&, "DT_BaseAnimating", "m_flCycle");
		NETVAR(hitbox_set, int, "DT_BaseAnimating", "m_nHitboxSet");
		NETVAR(clientside_anims, bool&, "DT_BaseAnimating", "m_bClientSideAnimation");
		NETVAR_PTR(poses, float, "DT_BaseAnimating", "m_flPoseParameter");
		OFFSET(last_setup_bone_time, float&, 0x2924);

		VFUNC(set_model_index(int idx), void(__thiscall*)(decltype(this), int), 75, idx);
		VFUNC(update_ik_locks(float time), void(__thiscall*)(decltype(this), float), 192, time);
		VFUNC(calculate_ik_locks(float time), void(__thiscall*)(decltype(this), float), 193, time);

		VFUNC(standard_blending_rules(studio_hdr_t* hdr, vec3d* vec, quaternion_t* q, float time, int mask), void(__thiscall*)(decltype(this), studio_hdr_t*, vec3d*, quaternion_t*, float, int), 206, hdr, vec, q, time, mask);
	};

	struct base_attributable_item_t {};

	struct base_grenade_t : base_animating_t, base_attributable_item_t {
		NETVAR_OFFSET(hegrenade_exploded, bool, "DT_BaseCSGrenadeProjectile", "m_nExplodeEffectTickBegin", 4);
		NETVAR(throw_time, float, "DT_BaseCSGrenade", "m_fThrowTime");
		NETVAR(smoke_effect_tick_begin, int, "DT_SmokeGrenadeProjectile", "m_nSmokeEffectTickBegin");
		NETVAR(did_smoke_effect, bool, "DT_SmokeGrenadeProjectile", "m_bDidSmokeEffect");
		NETVAR(thrower, base_handle_t, "DT_BaseGrenade", "m_hThrower");

		NETVAR(fire_count, int, "DT_Inferno", "m_fireCount");
		NETVAR_PTR(fire_is_burning, bool, "DT_Inferno", "m_bFireIsBurning");
		NETVAR_PTR(fire_delta_x, int, "DT_Inferno", "m_fireXDelta");
		NETVAR_PTR(fire_delta_y, int, "DT_Inferno", "m_fireYDelta");
		NETVAR_PTR(fire_delta_z, int, "DT_Inferno", "m_fireZDelta");
	};

	struct base_weapon_t : base_animating_t, base_attributable_item_t {
		NETVAR(ammo, int, "DT_BaseCombatWeapon", "m_iClip1");
		NETVAR(world_model, base_handle_t, "DT_BaseCombatWeapon", "m_hWeaponWorldModel");
		NETVAR(next_primary_attack, float&, "DT_BaseCombatWeapon", "m_flNextPrimaryAttack");
		NETVAR(next_secondary_attack, float&, "DT_BaseCombatWeapon", "m_flNextSecondaryAttack");
		NETVAR(item_definition_index, short&, "DT_BaseAttributableItem", "m_iItemDefinitionIndex");

		NETVAR_PTR(custom_name, char, "DT_BaseAttributableItem", "m_szCustomName");
		NETVAR(account_id, int&, "DT_BaseAttributableItem", "m_iAccountID");
		NETVAR(item_id_high, int&, "DT_BaseAttributableItem", "m_iItemIDHigh");
		NETVAR(entity_quality, int&, "DT_BaseAttributableItem", "m_iEntityQuality");
		NETVAR(fallback_paintkit, int&, "DT_BaseAttributableItem", "m_nFallbackPaintKit");
		NETVAR(fallback_seed, float&, "DT_BaseAttributableItem", "m_nFallbackSeed");
		NETVAR(fallback_wear, float&, "DT_BaseAttributableItem", "m_flFallbackWear");
		NETVAR(fallback_stattrack, int&, "DT_BaseAttributableItem", "m_nFallbackStatTrak");
		NETVAR(owner_xuid_high, int&, "DT_BaseAttributableItem", "m_OriginalOwnerXuidHigh");
		NETVAR(owner_xuid_low, int&, "DT_BaseAttributableItem", "m_OriginalOwnerXuidLow");

		OFFSET(next_attack, float&, 0x2D80);
	};

	struct weapon_cs_base_t : base_weapon_t {
		NETVAR(last_shot_time, float&, "DT_WeaponCSBase", "m_fLastShotTime");
		NETVAR(postpone_fire_ready_time, float&, "DT_WeaponCSBase", "m_flPostponeFireReadyTime");
	};

	struct base_combat_weapon_t : weapon_cs_base_t {
		OFFSET(in_reload, bool, 0x32B5);
		NETVAR(zoom_level, int, "DT_WeaponCSBaseGun", "m_zoomLevel");
		NETVAR(burst_shots_remaining, int, "DT_WeaponCSBaseGun", "m_iBurstShotsRemaining");
		NETVAR(next_burst_shot, float, "DT_WeaponCSBaseGun", "m_fNextBurstShot");

		VFUNC(get_inaccuracy(), float(__thiscall*)(decltype(this)), 483);
		VFUNC(get_spread(), float(__thiscall*)(decltype(this)), 453);
		VFUNC(update_accuracy_penalty(), void(__thiscall*)(decltype(this)), 484);
		VFUNC(get_cs_weapon_info(), cs_weapon_info_t*(__thiscall*)(decltype(this)), 461);
		VFUNC(get_zoom_in_sound(), const char*(__thiscall*)(decltype(this)), 445);
		VFUNC(get_zoom_out_sound(), const char*(__thiscall*)(decltype(this)), 446);

		VFUNC(get_muzzle_index_1st_person(base_entity_t* viewmodel), int(__thiscall*)(decltype(this), base_entity_t*), 468, viewmodel);
		VFUNC(get_muzzle_index_3rd_person(), int(__thiscall*)(decltype(this)), 469);

		bool is_knife();
		bool is_projectile();
		bool is_bomb();

		__forceinline bool is_sniper() {
			switch (this->item_definition_index()) {
				case e_weapon_type::weapon_scar20:
				case e_weapon_type::weapon_g3sg1:
				case e_weapon_type::weapon_awp:
				case e_weapon_type::weapon_ssg08:
					return true;
			}

			return false;
		}

		__forceinline bool is_gun() {
			if (this->is_knife() || this->get_cs_weapon_info() == nullptr)
				return false;

			switch (this->item_definition_index()) {
				case 0:
				case e_weapon_type::weapon_hegrenade:
				case e_weapon_type::weapon_decoy:
				case e_weapon_type::weapon_inc:
				case e_weapon_type::weapon_molotov:
				case e_weapon_type::weapon_c4:
				case e_weapon_type::weapon_flashbang:
				case e_weapon_type::weapon_smokegrenade:
				case e_weapon_type::weapon_snowball:
				case e_weapon_type::weapon_healthshot:
					return false;
				default:
					return true;
			}
		}
	};

	struct base_combat_character_t : base_animating_t {
		NETVAR_PTR(wearables, base_handle_t, "DT_BaseCombatCharacter", "m_hMyWearables");
		NETVAR(active_weapon_handle, base_handle_t, "DT_BaseCombatCharacter", "m_hActiveWeapon");
	};

	struct base_viewmodel_t : base_entity_t {
		NETVAR(weapon_handle, base_handle_t, "DT_BaseViewModel", "m_hWeapon");
		NETVAR(owner_handle, base_handle_t, "DT_BaseViewModel", "m_hOwner");
		NETVAR(viewmodel_index, uint32_t, "DT_BaseViewModel", "m_nViewModelIndex");

		DATAMAP(cycle, float&, "m_flCycle");
		DATAMAP(animtime, float&, "m_flAnimTime");
		DATAMAP(sequence, int&, "m_nSequence");
		DATAMAP(animation_parity, int&, "m_nAnimationParity");
	};

	struct base_player_t : base_combat_character_t {
		NETVAR(tickbase, int&, "DT_BasePlayer", "m_nTickBase");
		NETVAR(observer_target, base_handle_t, "DT_BasePlayer", "m_hObserverTarget");
		NETVAR(skybox_area, int&, "DT_BasePlayer", "m_skybox3d.area");
		NETVAR(view_offset, vec3d&, "DT_BasePlayer", "m_vecViewOffset[0]");
		NETVAR(fall_velocity, float&, "DT_BasePlayer", "m_flFallVelocity");
		NETVAR(viewmodel_handle, base_handle_t, "DT_BasePlayer", "m_hViewModel[0]");
		NETVAR(view_punch_angle, vec3d&, "DT_BasePlayer", "m_viewPunchAngle");
		NETVAR(max_speed, float, "DT_BasePlayer", "m_flMaxSpeed");

		OFFSET(animstate, animation_state_t*, 0x9960);
		OFFSET(animation_layers, animation_layer_t*, 0x2990);

		bool is_teammate();

		player_info_t get_player_info();
		std::string name();
	};

	struct cs_player_t : base_player_t {
		OFFSET(flash_amount, float&, 0x10470);
		OFFSET(origin, vec3d, 0x138);
		OFFSET(origin_pred, vec3d, 0xAC);
		OFFSET(next_think_tick, int&, 0xFC);
		OFFSET(ik_ctx, int&, 0x2670);

		NETVAR(simtime, float&, "DT_CSPlayer", "m_flSimulationTime");
		NETVAR_OFFSET(old_simtime, float&, "DT_CSPlayer", "m_flSimulationTime", 4);

		OFFSET(jiggle_bones, bool&, 0x2930);
		OFFSET(use_new_animstate, bool&, 0x9B14);

		NETVAR_OFFSET_PTR(bone_accessor, bone_accessor_t, "DT_BaseAnimating", "m_nForceBone", 0x1C);

		NETVAR(defusing, bool, "DT_CSPlayer", "m_bIsDefusing");
		NETVAR(player_state, int&, "DT_CSPlayer", "m_iPlayerState");
		NETVAR(wait_for_noattack, bool, "DT_CSPlayer", "m_bWaitForNoAttack");
		NETVAR(ragdoll_handle, base_handle_t, "DT_CSPlayer", "m_hRagdoll");
		NETVAR(duck_amount, float&, "DT_CSPlayer", "m_flDuckAmount");
		NETVAR(observer_mode, int&, "DT_CSPlayer", "m_iObserverMode");
		NETVAR(eye_angles, vec3d&, "DT_CSPlayer", "m_angEyeAngles");
		NETVAR(velocity_modifier, float&, "DT_CSPlayer", "m_flVelocityModifier");
		NETVAR(has_defuser, bool, "DT_CSPlayer", "m_bHasDefuser");
		NETVAR(armor, int, "DT_CSPlayer", "m_ArmorValue");
		NETVAR(helmet, bool, "DT_CSPlayer", "m_bHasHelmet");
		NETVAR(heavy_armor, bool, "DT_CSPlayer", "m_bHasHeavyArmor");
		NETVAR(gungame_immunity, bool, "DT_CSPlayer", "m_bGunGameImmunity");
		NETVAR(ducking, bool, "DT_CSPlayer", "m_bDucking");
		NETVAR(thirdperson_recoil, float&, "DT_CSPlayer", "m_flThirdpersonRecoil");
		NETVAR(lby, float&, "DT_CSPlayer", "m_flLowerBodyYawTarget");
		NETVAR(scoped, bool, "DT_CSPlayer", "m_bIsScoped");
		NETVAR(punch_angle, vec3d&, "DT_CSPlayer", "m_aimPunchAngle");
		NETVAR(punch_angle_vel, vec3d&, "DT_CSPlayer", "m_aimPunchAngleVel");

		OFFSET(command_context, cmd_ctx_t&, 0x350C);
		OFFSET_PTR(my_weapons, base_handle_t, 0x2DE8);
		OFFSET(next_attack, float&, 0x2D80);
		OFFSET(model_scale, float, 0x274C);
		OFFSET(strafing, bool&, 0x9AE0);
		OFFSET(duck_speed, float&, 0x2FC0);
		OFFSET(stamina, float&, 0x103D8);

		OFFSET(local, local_data_t&, 0x2FCC);
		OFFSET(ground_accel_linear_frac_last_time, float&, 0x103F0);

		DATAMAP(abs_velocity, vec3d&, "m_vecAbsVelocity");
		DATAMAP(effects, flags_t&, "m_fEffects");
		DATAMAP(ground_entity_handle, base_handle_t&, "m_hGroundEntity");
		DATAMAP(ground_entity_raw, int&, "m_hGroundEntity");

		DATAMAP(buttons, int&, "m_nButtons");
		DATAMAP(button_released, int&, "m_afButtonReleased");
		DATAMAP(button_pressed, int&, "m_afButtonPressed");
		DATAMAP(button_last, int&, "m_afButtonLast");

		NETVAR_OFFSET(button_forced, int&, "DT_BasePlayer", "m_hViewEntity", -0x8);
		NETVAR_OFFSET(button_disabled, int&, "DT_BasePlayer", "m_hViewEntity", -0xC);

		__forceinline bool is_alive() {
			return !lifestate() && health();
		}

		__forceinline vec3d eye_position() {
			return this->origin() + this->view_offset();
		}

		__forceinline vec3d render_eye_position() {
			return this->render_origin() + this->view_offset();
		}

		__forceinline void store_poses(float* poses) {
			memcpy(poses, this->poses(), sizeof(float[24]));
		}

		__forceinline void apply_poses(float* poses) {
			memcpy(this->poses(), poses, sizeof(float[24]));
		}

		__forceinline void store_layers(animation_layers_t layers) {
			memcpy(layers, this->animation_layers(), sizeof(animation_layers_t));
		}

		__forceinline void apply_layers(animation_layers_t layers) {
			memcpy(this->animation_layers(), layers, sizeof(animation_layers_t));
		}

		__forceinline base_combat_weapon_t* active_weapon() {
			return (base_combat_weapon_t*)active_weapon_handle().get();
		}

		//__forceinline c_base_viewmodel* viewmodel() {
		//	return viewmodel_handle().get<c_base_viewmodel*>();
		//}

		OFFSET(bone_cache, utils::utl_vector_t<matrix3x4_t>&, 0x2914);

		__forceinline void set_bone_cache(matrix3x4_t* dst) {
			auto& cache = this->bone_cache();
			utils::memcpy_sse(cache.base(), dst, sizeof(matrix3x4_t) * cache.count());
		}

		__forceinline void store_bone_cache(matrix3x4_t* src) {
			auto& cache = this->bone_cache();
			utils::memcpy_sse(src, cache.base(), sizeof(matrix3x4_t) * cache.count());
		}

		VFUNC(set_sequence(int sequence), void(__thiscall*)(decltype(this), int), 219, sequence);
		VFUNC(studio_frame_advance(), void(__thiscall*)(decltype(this)), 220);

		VFUNC(update_clientside_animations(), void(__thiscall*)(decltype(this)), 224);

		VFUNC(update_dispatch_layer(animation_layer_t* layers, studio_hdr_t* hdr, int sequence),
			  void(__thiscall*)(decltype(this), animation_layer_t*, studio_hdr_t*, int), 247, layers, hdr, sequence);

		VFUNC(update_collision_bounds(), void(__thiscall*)(decltype(this)), 340);

		int get_sequence_activity(int seq);
		std::vector<base_combat_weapon_t*> get_weapons();
		uint8_t* get_server_edict();
		void draw_server_hitboxes();

		__forceinline void invalidate_bone_cache(int framecount) {
			static DWORD addr = patterns::invalidate_bone_cache.as<DWORD>();

			*(uintptr_t*)((uintptr_t)this + 0xA30) = framecount;
			*(uintptr_t*)((uintptr_t)this + 0xA28) = 0;

			unsigned long model_bone_counter = **(unsigned long**)(addr + 10);
			*(uintptr_t*)((DWORD)this + 0x2924) = 0xFF7FFFFF;
			*(uintptr_t*)((DWORD)this + 0x2690) = (model_bone_counter - 1);
		}

		PATTERN(set_abs_origin(vec3d origin), void(__thiscall*)(decltype(this), const vec3d&), patterns::set_abs_origin, origin);
		PATTERN(set_abs_angles(vec3d angles), void(__thiscall*)(decltype(this), const vec3d&), patterns::set_abs_angles, angles);
		PATTERN(invalidate_physics_recursive(int32_t flags), void(__thiscall*)(decltype(this), int32_t), patterns::invalidate_physics_recursive, flags);
		PATTERN(attachment_helper(), void(__thiscall*)(decltype(this), studio_hdr_t*), patterns::attachment_helper, studio_hdr());

		__forceinline studio_hdr_t* studio_hdr() {
			static uintptr_t offset = (*(uintptr_t*)((uintptr_t)patterns::studio_hdr.as<void*>() + 2)) + 4;
			return *(studio_hdr_t**)((uintptr_t)this + offset);
		}

		float max_desync();
		int ping();
		int& personal_data_public_level();
	};
} // namespace sdk