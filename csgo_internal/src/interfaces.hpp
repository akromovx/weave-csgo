#pragma once
#include "../sdk/client.hpp"
#include "../sdk/client_state.hpp"
#include "../sdk/engine_client.hpp"
#include "../sdk/surface.hpp"
#include "../sdk/debug_overlay.hpp"
#include "../sdk/entity_list.hpp"
#include "../sdk/physics_surface_props.hpp"
#include "../sdk/convar.hpp"
#include "../sdk/global_vars.hpp"
#include "../sdk/move_helper.hpp"
#include "../sdk/game_movement.hpp"
#include "../sdk/vpanel.hpp"
#include "../sdk/prediction.hpp"
#include "../sdk/engine_sound.hpp"
#include "../sdk/model_info.hpp"
#include "../sdk/engine_trace.hpp"
#include "../sdk/localize.hpp"
#include "../sdk/glow_manager.hpp"
#include "../sdk/input.hpp"
#include "../sdk/model_render.hpp"
#include "../sdk/studio_render.hpp"
#include "../sdk/material_system.hpp"
#include "../sdk/game_events.hpp"
#include "../sdk/game_rules.hpp"
#include "../sdk/i_mem_alloc.hpp"
#include "../sdk/render_view.hpp"
#include "../sdk/model_cache.hpp"
#include "../sdk/beams.hpp"
#include "../sdk/net_string_container.hpp"

namespace interfaces {
	inline sdk::client_t* client{};
	inline void* client_mode{};
	inline sdk::client_state_t* client_state{};
	inline sdk::engine_client_t* engine{};
	inline sdk::entity_list_t* entity_list{};
	inline sdk::surface_t* surface{};
	inline sdk::physics_surface_props_t* phys_surface{};
	inline sdk::convars_t* convar{};
	inline sdk::global_vars_t* global_vars{};
	inline sdk::vpanel_t* v_panel{};
	inline sdk::debug_overlay_t* debug_overlay{};
	inline sdk::prediction_t* prediction{};
	inline sdk::move_helper_t* move_helper{};
	inline sdk::game_movement_t* game_movement{};
	inline sdk::engine_sound_t* engine_sound{};
	inline sdk::vmodel_info_t* model_info{};
	inline sdk::engine_trace_t* traces{};
	inline sdk::localize_t* localize{};
	inline sdk::game_event_manager2_t* game_events{};
	inline sdk::glow_object_manager_t* glow_manager{};
	inline sdk::input_t* input{};
	inline sdk::model_render_t* model_render{};
	inline sdk::material_system_t* material_system{};
	inline sdk::studio_render_t* studio_render{};
	inline sdk::cs_game_rules_t* game_rules{};
	inline sdk::mem_alloc_t* mem_alloc{};
	inline sdk::render_view_t* render_view{};
	inline sdk::model_cache_t* model_cache{};
	inline sdk::view_render_beams_t* beams{};
	inline sdk::net_string_table_container_t* net_string_container{};

	extern void init();
	extern void update();
	extern void reset();
} // namespace interfaces

#define GET_CVAR(name) []() {                                                  \
	static const auto cvar = interfaces::convar->find_var(XOR32S(HASH(name))); \
	return cvar;                                                               \
}()

#define CVAR_INT(name) GET_CVAR(name)->get_int()
#define CVAR_FLOAT(name) GET_CVAR(name)->get_float()
#define CVAR_BOOL(name) GET_CVAR(name)->get_bool()
#define CVAR_STRING(name) GET_CVAR(name)->get_string()