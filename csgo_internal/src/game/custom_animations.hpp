#include "../../sdk/entities.hpp"
#include <algorithm>

enum activity_t {
	act_csgo_defuse = 958,
	act_csgo_defuse_with_kit,
	act_csgo_flashbang_reaction,
	act_csgo_fire_primary,
	act_csgo_fire_primary_opt_1,
	act_csgo_fire_primary_opt_2,
	act_csgo_fire_secondary,
	act_csgo_fire_secondary_opt_1,
	act_csgo_fire_secondary_opt_2,
	act_csgo_reload,
	act_csgo_reload_start,
	act_csgo_reload_loop,
	act_csgo_reload_end,
	act_csgo_operate,
	act_csgo_deploy,
	act_csgo_catch,
	act_csgo_silencer_detach,
	act_csgo_silencer_attach,
	act_csgo_twitch,
	act_csgo_twitch_buyzone,
	act_csgo_plant_bomb,
	act_csgo_idle_turn_balanceadjust,
	act_csgo_idle_adjust_stoppedmoving,
	act_csgo_alive_loop,
	act_csgo_flinch,
	act_csgo_flinch_head,
	act_csgo_flinch_molotov,
	act_csgo_jump,
	act_csgo_fall,
	act_csgo_climb_ladder,
	act_csgo_land_light,
	act_csgo_land_heavy,
	act_csgo_exit_ladder_top,
	act_csgo_exit_ladder_bottom,
	act_csgo_parachute,
	act_csgo_uiplayer_idle,
	act_csgo_uiplayer_walkup,
	act_csgo_uiplayer_celebrate,
	act_csgo_uiplayer_confirm,
	act_csgo_uiplayer_buymenu,
	act_csgo_uiplayer_patch
};

static __forceinline float remap_value(float val, float A, float B, float C, float D) {
	if (A == B)
		return val >= B ? D : C;
	float cVal = (val - A) / (B - A);
	cVal = std::clamp(cVal, 0.0f, 1.0f);

	return C + (D - C) * cVal;
}

extern void play_custom_animations(sdk::animation_state_t* state, sdk::animation_layers_t layers, sdk::user_cmd_t* cmd);
extern void clamp_bones_in_bbox(sdk::cs_player_t* player, matrix3x4_t* matrix, int mask, float curtime, const vec3d& eye_angles);

namespace bone_merge {
	std::uintptr_t& get_bone_merge(sdk::cs_player_t* player);
	void update_cache(std::uintptr_t& bone_merge);
} // namespace bone_merge

class c_bone_builder {
private:
	bool can_be_animated(sdk::cs_player_t* player);
	void get_skeleton(vec3d* position, quaternion_t* q);
	void studio_build_matrices(sdk::studio_hdr_t* hdr, const matrix3x4_t& world_transform, vec3d* pos, quaternion_t* q, int mask, matrix3x4_t* out, uint32_t* bone_computed);

public:
	bool filled{};

	bool ik_ctx{};
	bool attachments{};
	bool dispatch{};

	int mask{};
	int layer_count{};

	float time{};

	matrix3x4_t* matrix{};
	sdk::studio_hdr_t* hdr{};
	sdk::animation_layer_t* layers{};
	sdk::cs_player_t* animating{};

	vec3d origin{};
	vec3d angles{};
	vec3d eye_angles{};

	std::array<float, 24> poses{};
	std::array<float, 24> poses_world{};

	void store(sdk::cs_player_t* player, matrix3x4_t* matrix, int mask);
	void setup();

	void reset() {
		filled = false;
		ik_ctx = false;
		attachments = false;
		dispatch = false;

		mask = 0;
		layer_count = 0;

		time = 0.f;

		matrix = nullptr;
		hdr = nullptr;
		layers = nullptr;
		animating = nullptr;

		origin.set();
		angles.set();
		eye_angles.set();

		poses = {};
		poses_world = {};
	}
};