#include "hooker.hpp"
#include "hooks.hpp"

#include "../globals.hpp"
#include "../interfaces.hpp"

#include "../features/bullets.hpp"
#include "../features/hvh/exploits.hpp"

#include "../game/engine_prediction.hpp"
#include "../game/players.hpp"

#include <intrin.h>

namespace hooks::misc {
	using namespace sdk;

	void __fastcall fire_bullet(cs_player_t* ecx, void* edx, vec3d eye_position,
								const vec3d& shoot_angles, float distance, float penetration,
								int penetration_count, int bullet_type, int damage,
								float range_modifier, base_entity_t* pev_attacker,
								bool do_effects, float x_spread, float y_spread) {
		static auto original = detour->original(&fire_bullet);

		original(
				ecx, edx, eye_position, shoot_angles, distance,
				penetration, penetration_count, bullet_type, damage,
				range_modifier, pev_attacker, do_effects,
				0.f, 0.f // remove spread for client impacts
		);
	}

	vec3d* __fastcall weapon_shootpos(cs_player_t* ecx, void* edx, vec3d& eye) {
		static auto original = detour->original(&weapon_shootpos);

		if (ecx == globals->m_local) {
			eye = globals->m_last_shoot_position;
			return &eye;
		}

		return original(ecx, edx, eye);
	}

	void __fastcall add_viewmodel_bob(void* ecx, void* edx, sdk::base_viewmodel_t* model, vec3d& origin, vec3d& angles) {
		static auto original = detour->original(&add_viewmodel_bob);

		if (settings->visuals.removals.at(8))
			return;

		original(ecx, edx, model, origin, angles);
	}

	int __fastcall list_leaves_in_box(void* bsp, void* edx, vec3d& mins, vec3d& maxs, uint16_t* list, int list_max) {
		static auto original = vmt::query->original<int(__thiscall*)(void*, const vec3d&, const vec3d&, uint16_t*, int)>(XOR32S(6));
		static auto list_leaves = patterns::list_leaves.as<void*>();

		if (_ReturnAddress() != list_leaves)
			return original(bsp, mins, maxs, list, list_max);

		auto info = *(renderable_info_t**)((uintptr_t)_AddressOfReturnAddress() + 0x14);
		if (!info || !info->m_renderable)
			return original(bsp, mins, maxs, list, list_max);

		auto base_entity = (base_entity_t*)info->m_renderable->unknown()->get_base_entity();
		if (base_entity == nullptr || !base_entity->is_player())
			return original(bsp, mins, maxs, list, list_max);

		info->m_flags &= ~0x100;
		info->m_translucency_type = base_entity == globals->m_local ? 1 : 2;

		constexpr auto max_coord_float = 16384.f;
		constexpr auto min_coord_float = -max_coord_float;
		constexpr vec3d map_min = vec3d{ min_coord_float, min_coord_float, min_coord_float };
		constexpr vec3d map_max = vec3d{ max_coord_float, max_coord_float, max_coord_float };

		return original(bsp, map_min, map_max, list, list_max);
	}

	bool __fastcall should_draw_shadow(void* ecx, void* edx) {
		return !settings->visuals.removals.at(6);
	}

	void __fastcall draw_model_array(void* ecx, void* edx,
									 const studio_model_array_info2_t& info, int count, studio_array_data_t* array_data, int stride, int flags) {
		static auto original = vmt::studio_render->original<int(__thiscall*)(void*, const studio_model_array_info2_t&, int, studio_array_data_t*, int, int)>(XOR32S(48));
		original(ecx, info, count, array_data, stride, 0);
	}

	void __fastcall clip_ray_to_collideable(void* ecx, void* edx, const ray_t& ray, unsigned int mask, collideable_t* collideable, game_trace_t* trace) {
		static auto original = vmt::traces->original<void(__thiscall*)(void*, const ray_t&, unsigned int, collideable_t*, game_trace_t*)>(XOR32S(4));
		auto backup_maxs = collideable->maxs().z;
		collideable->maxs().z += 5.f;

		original(ecx, ray, mask, collideable, trace);
		collideable->maxs().z = backup_maxs;
	}

	void __fastcall begin_frame(void* thisptr) {
		static auto original = vmt::studio_render->original<void(__thiscall*)(void*)>(XOR32S(9));
		//bullet_tracer->draw_tracers();
		original(thisptr);
	}

	void* __fastcall model_renderable_animating(void* ecx, void* edx) {
		static auto original = detour->original(model_renderable_animating);

		auto player = (cs_player_t*)((uintptr_t)ecx - 4);
		if (player == nullptr || player->client_class()->m_class_id != e_class_id::CCSRagdoll)
			return original(ecx, edx);

		return nullptr;
	}

	void __fastcall get_color_modulation(void* ecx, void* edx, float* r, float* g, float* b) {
		static auto original = detour->original(get_color_modulation);

		if (!settings->visuals.nightmode.enable)
			return original(ecx, edx, r, g, b);

		original(ecx, edx, r, g, b);

		const auto material = reinterpret_cast<material_t*>(ecx);

		if (!material || material->is_error_material())
			return original(ecx, edx, r, g, b);

		const auto name = material->get_texture_group_name();

		if (strstr(name, STRC("World"))) {
			const auto clr = settings->visuals.nightmode.color;
			*r *= clr.r() / 255.f;
			*g *= clr.g() / 255.f;
			*b *= clr.b() / 255.f;
		} else if (strstr(name, STRC("StaticProp"))) {
			const auto clr = settings->visuals.nightmode.prop_color;
			*r *= clr.r() / 255.f;
			*g *= clr.g() / 255.f;
			*b *= clr.b() / 255.f;
		} else if (strstr(name, STRC("SkyBox"))) {
			const auto clr = settings->visuals.nightmode.skybox_color;
			*r *= clr.r() / 255.f;
			*g *= clr.g() / 255.f;
			*b *= clr.b() / 255.f;
		}
	}

	bool __fastcall is_using_static_prop_debug_modes(void* ecx, void* edx) {
		return settings->visuals.nightmode.enable;
	}

	void __fastcall setup_per_instance_color_modulation(void* ecx, void* edx, int cnt, model_list_by_type_t* list) {
		static auto original = detour->original(&setup_per_instance_color_modulation);

		if (!settings->visuals.nightmode.enable)
			return original(ecx, edx, cnt, list);

		for (int i = 0; i < cnt; ++i) {
			auto& l = list[i];
			if (!l.m_count)
				continue;

			for (int j = 0; j < l.m_count; ++j) {
				auto model = &l.m_render_models[j];
				auto renderable = (renderable_t*)model->m_entry.m_renderable;
				if (renderable == nullptr)
					continue;

				auto entity = (base_entity_t*)renderable->unknown()->get_client_entity();
				if (entity == nullptr)
					continue;

				auto client_class = entity->client_class();
				if (client_class == nullptr)
					continue;

				auto clr = settings->visuals.nightmode.prop_color;
				model->m_diffuse_modulation.x = clr.r() / 255.f;
				model->m_diffuse_modulation.y = clr.g() / 255.f;
				model->m_diffuse_modulation.z = clr.b() / 255.f;

				float alpha = (model->m_entry.m_instance_data.m_alpha / 255.f);
				model->m_diffuse_modulation.w = alpha;

				auto class_id = client_class->m_class_id;

				if (!entity->is_weapon() && class_id != e_class_id::CPhysicsProp && class_id != e_class_id::CChicken && class_id != e_class_id::CHostage && class_id != e_class_id::CHostageCarriableProp)
					model->m_diffuse_modulation.w *= clr.base()[3];
			}
		}
	}

	bool __fastcall svc_msg_voice_data(void* ecx, uint32_t edx, sdk::svc_msg_voice_data_t* msg) {
		auto original = detour->original(&svc_msg_voice_data);
		players->on_voice_data_received(msg);
		return original(ecx, edx, msg);
	}

	bool __fastcall interpolate(void* ecx, void* edx, float time) {
		static auto original = detour->original(&interpolate);

		auto base_entity = (base_viewmodel_t*)ecx;

		auto owner = interfaces::entity_list->get_client_entity_handle(base_entity->owner_handle().value());
		if (owner == nullptr || owner != globals->m_local || !hvh::exploits->m_charging)
			return original(ecx, edx, time);

		const auto interp_backup = interfaces::global_vars->m_interpolation_amount;
		interfaces::global_vars->m_interpolation_amount = 0.f;
		bool ret = original(ecx, edx, time);
		interfaces::global_vars->m_interpolation_amount = interp_backup;
		return ret;
	}

	void __fastcall emit_sound(engine_sound_t* thisptr, uint32_t edx, void* filter, int ent_index, int channel, const char* sound_entry, unsigned int sound_entry_hash,
							   const char* sample, float volume, float attenuation, int seed, int flags, int pitch, const vec3d* origin, const vec3d* direction,
							   void* vec_origins, bool update_positions, float sound_time, int speaker_entity, int test) {

		static auto original = vmt::sounds->original<decltype(&emit_sound)>(XOR32S(5));

		auto call_original = [&]() {
			original(thisptr, edx, filter, ent_index, channel, sound_entry, sound_entry_hash, sample, volume, attenuation, seed, flags,
					 pitch, origin, direction, vec_origins, update_positions, sound_time, speaker_entity, test);
		};

		if (globals->m_local_alive && engine_prediction->m_is_predicted) {
			flags |= 1 << 2;
			return call_original();
		}

		call_original();
	}
} // namespace hooks::misc