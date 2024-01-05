#include "interfaces.hpp"
#include "utils/address.hpp"
#include "dump.hpp"

using namespace sdk;

namespace interfaces {
	CHEAT_INIT void init() {
		auto tier0 = GetModuleHandleA(STRC("tier0.dll"));
		if (tier0 == NULL)
			return;

		engine = utils::find_interface(STRSC("engine.dll"), STRSC("VEngineClient014")).as<engine_client_t*>();
		if (engine->get_engine_build_number() != XOR32S(disp::engine_build_number)) {
			MessageBoxA(0, STRC("Cheat doesn't updated to last game version! Please wait until update."), STRC("Error"), MB_ICONERROR | MB_SETFOREGROUND);
			exit(0);
		}

		client = utils::find_interface(STRSC("client.dll"), STRSC("VClient018")).as<client_t*>();
		entity_list = utils::find_interface(STRSC("client.dll"), STRSC("VClientEntityList003")).as<entity_list_t*>();

		client_mode = **utils::vfunc(client, XOR32(10)).add(XOR32(5)).as<void***>();

		surface = utils::find_interface(STRSC("vguimatsurface.dll"), STRSC("VGUI_Surface031")).as<surface_t*>();
		phys_surface = utils::find_interface(STRSC("vphysics.dll"), STRSC("VPhysicsSurfaceProps001")).as<physics_surface_props_t*>();
		convar = utils::find_interface(STRSC("vstdlib.dll"), STRSC("VEngineCvar007")).as<convars_t*>();
		v_panel = utils::find_interface(STRSC("vgui2.dll"), STRSC("VGUI_Panel009")).as<vpanel_t*>();
		debug_overlay = utils::find_interface(STRSC("engine.dll"), STRSC("VDebugOverlay004")).as<debug_overlay_t*>();
		prediction = utils::find_interface(STRSC("client.dll"), STRSC("VClientPrediction001")).as<prediction_t*>();
		game_movement = utils::find_interface(STRSC("client.dll"), STRSC("GameMovement001")).as<game_movement_t*>();
		engine_sound = utils::find_interface(STRSC("engine.dll"), STRSC("IEngineSoundClient003")).as<engine_sound_t*>();
		model_info = utils::find_interface(STRSC("engine.dll"), STRSC("VModelInfoClient004")).as<vmodel_info_t*>();
		traces = utils::find_interface(STRSC("engine.dll"), STRSC("EngineTraceClient004")).as<engine_trace_t*>();
		localize = utils::find_interface(STRSC("localize.dll"), STRSC("Localize_001")).as<localize_t*>();
		game_events = utils::find_interface(STRSC("engine.dll"), STRSC("GAMEEVENTSMANAGER002")).as<game_event_manager2_t*>();
		model_render = utils::find_interface(STRSC("engine.dll"), STRSC("VEngineModel016")).as<model_render_t*>();
		material_system = utils::find_interface(STRSC("materialsystem.dll"), STRSC("VMaterialSystem080")).as<material_system_t*>();
		studio_render = utils::find_interface(STRSC("studiorender.dll"), STRSC("VStudioRender026")).as<studio_render_t*>();
		render_view = utils::find_interface(STRSC("engine.dll"), STRSC("VEngineRenderView014")).as<render_view_t*>();
		model_cache = utils::find_interface(STRSC("datacache.dll"), STRSC("MDLCache004")).as<model_cache_t*>();

		net_string_container = utils::find_interface(STRSC("engine.dll"), STRSC("VEngineClientStringTable001")).as<net_string_table_container_t*>();

		input = *patterns::input.as<input_t**>();
		global_vars = **patterns::global_vars.as<global_vars_t***>();
		move_helper = **patterns::move_helper.as<move_helper_t***>();
		glow_manager = *patterns::glow_manager.as<glow_object_manager_t**>();

		beams = *patterns::beams.as<view_render_beams_t**>();

		mem_alloc = *(mem_alloc_t**)(GetProcAddress(tier0, STRC("g_pMemAlloc")));

		update();
	}

	CHEAT_INIT void update() {
		client_state = **patterns::client_state.as<client_state_t***>();
		game_rules = **patterns::game_rules.as<cs_game_rules_t***>();
	}

	CHEAT_INIT void reset() {
		client_state = nullptr;
		game_rules = nullptr;
	}
} // namespace interfaces