#include "chams.hpp"
#include "../../game/players.hpp"
#include "../../globals.hpp"
#include "../../hooks/hooks.hpp"
#include "../../interfaces.hpp"
#include <xmmintrin.h>

namespace esp {
	using namespace sdk;

	STFI int lookup_bone(base_entity_t* entity, const char* name) {
		return patterns::lookup_bone.as<int(__thiscall*)(base_entity_t*, const char*)>()(entity, name);
	}

	STFI vec3d matrix_get_origin(const matrix3x4_t& src) {
		return { src[0][3], src[1][3], src[2][3] };
	}

	STFI vec3d get_head_pos(cs_player_t* player, matrix3x4_t* bones) {
		const auto head = lookup_bone(player, STRC("head_0"));
		return matrix_get_origin(bones[head]);
	}

	STFI bool get_backtrack_chams(cs_player_t* player, matrix3x4_t* result, float& override_alpha) {
		const auto& set = settings->player_esp.chams[(int)incheat_vars::e_chams::chams_backtrack];
		if (!set.enable)
			return false;

		constexpr auto alpha_blend_distance = 25.f;
		constexpr auto blend_distance_sqr = alpha_blend_distance * alpha_blend_distance;
		constexpr auto blend_distance_sqr_inv = 1.f / blend_distance_sqr;

		auto entry = players->find_entry(player);
		if (entry == nullptr || !entry->m_initialized)
			return false;

		const auto latest_record = players->find_record_if(entry, [](lag_record_t*) { return true; });

		lag_record_t* old_record = nullptr;
		const auto [first, second] = players->find_interp_records(entry, old_record);

		if (first == nullptr || second == nullptr)
			return false;

		if (first->m_came_from_dormant || second->m_came_from_dormant)
			return false;

		if (first->m_last_received_time == 0.f) {
			first->m_last_received_time = interfaces::global_vars->m_realtime;
			first->m_lerp_time = 0.f;
			first->m_begin_time = latest_record->m_received_time;
		} else {
			float compensate_time_delta = first->m_begin_time - first->m_last_received_time;
			first->m_lerp_time = std::clamp(interfaces::global_vars->m_realtime - first->m_last_received_time - compensate_time_delta, 0.f, FLT_MAX);
		}

		float time_delta = TICKS_TO_TIME(std::abs(first->m_tick_received - second->m_tick_received));
		float current_tick = std::clamp(first->m_lerp_time * CVAR_FLOAT("host_timescale"), 0.f, time_delta);

		float delta = std::clamp(1.f - current_tick / time_delta, 0.f, 1.f);

		vec3d interpolated_position = first->m_origin;
		if (old_record != nullptr && !old_record->m_came_from_dormant)
			interpolated_position = math::hermit_spline(old_record->m_origin, first->m_origin, second->m_origin, delta);
		else
			interpolated_position = math::lerp(first->m_origin, second->m_origin, delta);

		//for (int i = 0; i < 128; ++i) {
		//	__m128 v_delta = _mm_set1_ps(delta);
		//	for (int j = 0; j < 3; ++j) {
		//		__m128 v_first = _mm_load_ps(&first->m_bones[i][j][0]);
		//		__m128 v_second = _mm_load_ps(&second->m_bones[i][j][0]);
		//		__m128 v_res = _mm_mul_ps(v_delta, _mm_sub_ps(v_second, v_first));
		//		v_res = _mm_add_ps(v_res, v_first);
		//		_mm_store_ps(&result[i][j][0], v_res);
		//	}
		//}

		std::memcpy(result, second->m_bones, sizeof(matrix3x4_t) * max_bones);

	//	override_alpha = std::min<float>((player->origin() - interpolated_position).length_sqr() * blend_distance_sqr_inv, 1.f);

		if (time_delta > TICKS_TO_TIME(2)) {
			static const auto calc_eye_delta = [](float a, float b, float lerp) {
				a = math::normalize_yaw(a);
				b = math::normalize_yaw(b);

				if (std::abs(a - b) > 180.f)
					return -(std::lerp(math::normalize_yaw(180.f - a), math::normalize_yaw(180.f - b), lerp) + 180.f);

				return std::lerp(math::normalize_yaw(a), math::normalize_yaw(b), lerp);
			};

			const auto eye_delta = calc_eye_delta(first->m_eye_angles.y, second->m_eye_angles.y, delta);
			const auto eye_delta_fixed = -math::normalize_yaw(eye_delta - math::normalize_yaw(first->m_eye_angles.y));

			if (std::abs(eye_delta_fixed) > 10.f)
				math::rotate_matrix(second->m_bones, result, eye_delta_fixed, second->m_origin);
		}

		math::change_bones_position(result, max_bones, second->m_origin, interpolated_position);

		override_alpha = std::clamp(get_head_pos(player, result).dist(get_head_pos(player, player->bone_cache().base())) * 0.01f, 0.f, 1.f);
		return true;
	}

	void chams_t::add_hitmatrix(cs_player_t* player, matrix3x4_t* bones) {
		if (settings->player_esp.shot_chams_last_only) {
			if (!m_hits.empty())
				m_hits.clear();
		}

		auto& hit = m_hits.emplace_back();

		std::memcpy(hit.m_bones, bones, sizeof(matrix3x4_t) * max_bones);
		hit.time = interfaces::global_vars->m_realtime + 4.f /*vars.visuals.shot_chams_duration*/;

		static unsigned int m_nSkin = find_in_datamap(player->get_pred_descmap(), HASH("m_nSkin"));
		static unsigned int m_nBody = find_in_datamap(player->get_pred_descmap(), HASH("m_nBody"));

		hit.info.m_origin = player->origin();
		hit.info.m_angles = player->get_abs_angles();

		auto renderable = player->renderable();
		if (renderable == nullptr)
			return;

		auto model = player->get_model();
		if (model == nullptr)
			return;

		auto hdr = interfaces::model_info->get_studio_model(model);
		if (hdr == nullptr)
			return;

		hit.state.m_studio_hdr = hdr;
		hit.state.m_studio_hdr_data = interfaces::model_cache->get_hardware_data(model->m_studio);

		hit.state.m_renderable = renderable;
		hit.state.m_draw_flags = 0;

		hit.info.m_renderable = renderable;
		hit.info.m_model = model;
		hit.info.m_lightning_offset = nullptr;
		hit.info.m_lightning_origin = nullptr;
		hit.info.m_hitboxset = player->hitbox_set();
		hit.info.m_skin = (int)(uintptr_t(player) + m_nSkin);
		hit.info.m_body = (int)(uintptr_t(player) + m_nBody);
		hit.info.m_entity_index = player->index();
		hit.info.m_instance = utils::vfunc<uint16_t(__thiscall*)(void*)>(renderable, 30)(renderable);
		hit.info.m_flags = 0x1;

		hit.info.m_model_to_world = &hit.model_to_world;
		hit.state.m_model_to_world = &hit.model_to_world;
		math::angle_matrix(hit.info.m_angles, hit.info.m_origin, hit.model_to_world);
		hit.m_valid = true;
	}

	void chams_t::on_post_screen_effects() {
		static auto draw_model_execute = hooks::vmt::model_render->original<void(__thiscall*)(void*, void*, const draw_model_state_t&, const model_render_info_t&, matrix3x4_t*)>(XOR32(21));

		if (!ctx->is_valid())
			return;

		if (m_hits.empty())
			return;

		auto ctx = interfaces::material_system->get_render_context();
		if (ctx == nullptr)
			return;

		for (auto& hit: m_hits) {
			if (!hit.state.m_model_to_world || !hit.state.m_renderable || !hit.state.m_studio_hdr || !hit.state.m_studio_hdr_data ||
				!hit.info.m_renderable || !hit.info.m_model_to_world || !hit.info.m_model)
				continue;

			if (interfaces::entity_list->get_client_entity(hit.info.m_entity_index) == nullptr) {
				hit.m_valid = false;
				continue;
			}

			auto alpha = 1.0f;
			auto delta = interfaces::global_vars->m_realtime - hit.time;
			if (delta > 0.0f) {
				alpha -= delta * 1.5f;
				if (delta > 1.0f) {
					hit.m_valid = false;
					continue;
				}
			}

			m_hook.m_state = hit.state;
			m_hook.m_info = hit.info;
			m_hook.m_callback = [&](matrix3x4_t* bones) {
				draw_model_execute(interfaces::model_render, ctx, hit.state, hit.info, bones);
			};

			draw_chams(settings->player_esp.chams[(int)incheat_vars::e_chams::chams_shot], true, hit.m_bones, alpha);
		}

		m_hits.erase(std::remove_if(m_hits.begin(), m_hits.end(), [](const hit_matrix_t& hit) {
						 return !hit.m_valid;
					 }),
					 m_hits.end());
	}

	inline bool chams_t::draw_chams(incheat_vars::chams_settings_t& set, bool pass_xqz, matrix3x4_t* bones, float override_alpha) {
		if (!set.enable)
			return false;

		if (override_alpha <= 0.0001f)
			return false;

		auto material = m_materials.get(set.material);
		if (material == nullptr)
			return false;

		float backup_color[3]{};
		interfaces::render_view->get_color_modulation(backup_color);

		const auto base_clr = set.color;
		constexpr auto alpha_epsilon = 1.f / 255.f - FLT_EPSILON;

		if (base_clr.a() / 255.f > alpha_epsilon) {
			material->color_modulate(base_clr.r() / 255.f, base_clr.g() / 255.f, base_clr.b() / 255.f);
			material->alpha_modulate(base_clr.a() * override_alpha / 255.f);

			material->set_material_var_flag(e_material_flags::material_var_ignorez, pass_xqz);

			if (set.material == 2) {
				auto mclr = set.metallic_color;
				auto var = material->find_var(STRC("$phongtint"), nullptr);
				const auto alpha_mod = mclr.a() / 255.f;
				if (var != nullptr)
					var->set_vec_value({ mclr.r() / 255.f * alpha_mod, mclr.g() / 255.f * alpha_mod, mclr.b() / 255.f * alpha_mod });

				float phong_exp = std::clamp(((set.phong_amount / 10.f) - 1.f), 0.f, 10.f);
				{
					auto phong_exponent = material->find_var(STRC("$phongexponent"), nullptr);
					phong_exponent->set_vec_value(10.f - phong_exp);

					auto phong_boost = material->find_var(STRC("$phongboost"), nullptr);
					phong_boost->set_vec_value(set.phong_amount / 100.f);
				}

				float rim_amt = (float)set.rim_amount;
				{
					auto rim_exponent = material->find_var(STRC("$rimlightexponent"), nullptr);
					rim_exponent->set_vec_value(rim_amt);

					auto rim_boost = material->find_var(STRC("$rimlightboost"), nullptr);
					rim_boost->set_vec_value(rim_amt);
				}
			}

			interfaces::model_render->forced_material_override(material);
			m_hook.m_callback(bones);
			interfaces::model_render->forced_material_override(nullptr);
		}

		for (uint8_t i = 0; i < 8; ++i) {
			if (!set.overlays.at(i))
				continue;

			auto overlay_material = m_materials.get_overlay(i);
			auto clr = set.overlays_colors[i];

			if (clr.base()[3] <= alpha_epsilon)
				continue;

			if (overlay_material == nullptr)
				continue;

			overlay_material->set_material_var_flag(e_material_flags::material_var_ignorez, pass_xqz);

			switch (i) {
				case 0:
				case 1:
				case 3: {
					auto var = overlay_material->find_var(STRC("$envmaptint"), nullptr);
					if (var != nullptr)
						var->set_vec_value({ clr.r() / 255.f, clr.g() / 255.f, clr.b() / 255.f });
				} break;

				case 2:
				case 4: {
					float clr_[3] = { clr.r() / 255.f, clr.g() / 255.f, clr.b() / 255.f };
					interfaces::render_view->set_color_modulation(clr_);
				} break;
			}

			overlay_material->alpha_modulate(clr.base()[3] * override_alpha);

			interfaces::model_render->forced_material_override(overlay_material);
			m_hook.m_callback(bones);
			interfaces::model_render->forced_material_override(nullptr);
		}

		interfaces::render_view->set_color_modulation(backup_color);
		return true;
	}

	bool chams_t::is_working(const sdk::draw_model_state_t& state, const sdk::model_render_info_t& info, std::function<void(matrix3x4_t*)> fn) {
		auto renderable = state.m_renderable;
		if (renderable == nullptr)
			return false;

		auto unknown = renderable->unknown();
		if (unknown == nullptr)
			return false;

		auto entity = (base_entity_t*)unknown->get_base_entity();
		if (entity == nullptr)
			return false;

		if (info.m_model == nullptr)
			return false;

		auto client_class = entity->client_class();
		if (client_class == nullptr)
			return false;

		m_hook.m_state = state;
		m_hook.m_info = info;
		m_hook.m_callback = fn;

		if (std::strstr(info.m_model->m_name, STRC("models/player")) != nullptr) {
			auto player = (cs_player_t*)entity;
			if (!player->is_player() || !player->is_alive() || player->dormant() || player->gungame_immunity())
				return false;

			if (player == globals->m_local) {
				if (settings->player_esp.local_blend_in_scope && globals->m_local->scoped())
					interfaces::render_view->set_blend(settings->player_esp.local_blend_in_scope_amount / 100.f);

				interfaces::render_view->set_blend(interfaces::render_view->get_blend() * std::min<float>(globals->m_local_alpha, globals->m_thirdperson_alpha));

				const auto real = draw_chams(settings->player_esp.chams[(int)incheat_vars::e_chams::chams_local]);

				{
					const float override_alpha = std::clamp(players->m_local_player.m_head_delta / 10.f, 0.f, 1.f);
					math::change_bones_position(players->m_local_player.m_fake_bones, sdk::max_bones, vec3d{}, globals->m_local->render_origin());
					draw_chams(settings->player_esp.chams[(int)incheat_vars::e_chams::chams_desync], false, players->m_local_player.m_fake_bones, override_alpha);
					math::change_bones_position(players->m_local_player.m_fake_bones, sdk::max_bones, globals->m_local->render_origin(), vec3d{});
				}

				return real;
			} else if (!player->is_teammate()) {
				static matrix3x4_t interpolated[max_bones]{};

				if (settings->ragebot.enable && globals->m_local != nullptr && globals->m_local->is_alive() && CVAR_BOOL("cl_lagcompensation")) {
					float override_alpha = 1.f;
					if (get_backtrack_chams(player, interpolated, override_alpha))
						draw_chams(settings->player_esp.chams[(int)incheat_vars::e_chams::chams_backtrack], true, interpolated, override_alpha);
				}

				const auto xqz = draw_chams(settings->player_esp.chams[(int)incheat_vars::e_chams::chams_xqz], true);
				const auto vis = draw_chams(settings->player_esp.chams[(int)incheat_vars::e_chams::chams_vis]);

				return vis || xqz;
			}
		} else if (std::strstr(info.m_model->m_name, STRC("arms")) != nullptr)
			return draw_chams(settings->player_esp.chams[(int)incheat_vars::e_chams::chams_arms]);
		else if (std::strstr(info.m_model->m_name, STRC("models/weapons/v_")) != nullptr)
			return draw_chams(settings->player_esp.chams[(int)incheat_vars::e_chams::chams_weapon]);

		const auto move_parent = entity->moveparent();
		if (move_parent != nullptr && move_parent == globals->m_local) {
			if (settings->player_esp.local_blend_in_scope && globals->m_local->scoped())
				interfaces::render_view->set_blend(settings->player_esp.local_blend_in_scope_amount / 100.f);

			interfaces::render_view->set_blend(interfaces::render_view->get_blend() * std::min<float>(globals->m_local_alpha, globals->m_thirdperson_alpha));

			if (draw_chams(settings->player_esp.chams[(int)incheat_vars::e_chams::chams_attachments]))
				return true;
		}

		return false;
	}

	STFI material_t* create_material(bool lit, const std::string& material_data) {
		static int materials_created = 0;
		std::string material_name = STR("weave_mat_") + std::to_string(++materials_created);
		static auto init_key_values = patterns::init_key_values.as<void(__thiscall*)(void*, const char*, int, int)>();
		static auto load_from_buffer = patterns::load_from_buffer.as<void(__thiscall*)(void*, const char*, const char*, void*, const char*, void*, void*)>();

		key_values_t* key_values = new key_values_t;
		init_key_values(key_values, lit ? STRC("VertexLitGeneric") : STRC("UnlitGeneric"), 0, 0);
		load_from_buffer(key_values, material_name.c_str(), material_data.c_str(), NULL, NULL, NULL, NULL);

		material_t* material = interfaces::material_system->create_material(material_name.c_str(), key_values);
		material->increment_reference_count();

		return material;
	}

	void chams_t::init() {
		static bool materials_created = false;

		if (materials_created)
			return;

		m_materials.m_default = create_material(true, STRS(R"#("VertexLitGeneric" {
			"$ambientonly"                  "1"
			"$nofog"						"0"
			"$ingorez"                      "0"
		})#"));

		m_materials.m_randomized = create_material(false, STRS(R"#("UnlitGeneric" {
			$baseTexture						"models\props_interiors/TVebstest"
			$color2								"[.43 .43 .43]"
			$surfaceprop						"glass"
		})#"));

		m_materials.m_animated = create_material(false, STRS(R"#("UnlitGeneric" {
			"$basetexture" "models\inventory_items\music_kit\darude_01\mp3_detail"
			"$additive" "1"
			"$scrollpos" "[0.0 0.0]"
			"$x1" "0.0"
			"$x2" "0.0"
			"$x3" "0.0"
			"$div" "3.0"
			"$half" "0.5"
			"$zero" "0.0"
			"$rescale" "0.667"
			"$offset1" "0.0"
			"$offset2" "0.333"
			"$offset3" "0.667"
			"$offset_temp" "0.0"
			"$offset" "0.0"
			"Proxies"
			{
				"LinearRamp"
				{
					"rate" "0.08"
					"initialValue"	"0"
					"resultVar"	"$x1"
				}
				"Frac"
				{
					"srcVar1" "$x1"
					"resultVar" "$x2"
				}
				"LessOrEqual"
				{
					"srcVar1"	"$x2"
					"srcVar2"	"$offset2"
					"lessequalVar" "$offset1"
					"greaterVar" "$offset2"
					"resultVar" "$offset_temp"
				}
				"LessOrEqual"
				{
					"srcVar1"	"$x2"
					"srcVar2"	"$offset3"
					"lessequalVar" "$offset_temp"
					"greaterVar" "$offset3"
					"resultVar" "$offset"
				}
				"Multiply"
				{
					"srcVar1"	"$x2" // between 0 and 1
					"srcVar2"	"$div"
					"resultVar"	"$x1" // x1*3
				}
				"Frac"
				{
					"srcVar1" "$x1"
					"resultVar" "$x3"
				}
				"Subtract"
				{
					"srcVar1" "$x3"
					"srcVar2" "$half"
					"resultVar" "$x2"
				}
				"LessOrEqual" // x3 cycles three times between 0 and 1 before the whole system resets
				{
					"srcVar1"	"$x3"
					"srcVar2"	"$half"
					"lessequalVar" "$zero" // 0.0 - 0.5, stomps to 0
					"greaterVar" "$x2" // 0.5 - 1.0, 0-0.5
					"resultVar" "$x1"
				}
				"Multiply"
				{
					"srcVar1"	"$x1"
					"srcVar2"	"$rescale"
					"resultVar" "$x2"
				}
				"Add"
				{
					"srcVar1" "$x2"
					"srcVar2" "$offset"
					"resultVar" "$scrollpos[0]"
				}
				"TextureTransform"
				{
					"translateVar" "$scrollpos"
					"resultVar"	"$basetexturetransform"
				}
			}
		})#"));

		m_materials.m_flat = create_material(false, STRS(R"#("UnlitGeneric" {
			"$basetexture"						"vgui/white_additive"
			"$nofog"							"1"
			"$model"							"1"
			"$nocull"							"0"
			"$selfillum"						"1"
			"$halflambert"						"1"
			"$znearer"							"0"
			"$flat"								"1"
		})#"));

		m_materials.m_wireframe = create_material(false, STRS(R"#("UnlitGeneric" {
			"$basetexture"						"vgui/white_additive"
			"$nofog"							"1"
			"$model"							"1"
			"$nocull"							"0"
			"$selfillum"						"1"
			"$halflambert"						"1"
			"$znearer"							"0"
			"$flat"								"1"
			"$wireframe"						"1"
		})#"));

		m_materials.m_glow_fade = create_material(true, STRS(R"#("VertexLitGeneric" {
			"$additive"							"1"
			"$envmap"							"models/effects/cube_white"
			"$envmaptint"						"[1 1 1]"
			"$envmapfresnel"					"1"
			"$envmapfresnelminmaxexp"			"[0 1 2]"
			"$alpha"							"1"
		})#"));

		m_materials.m_glow_line = create_material(true, STRS(R"#("VertexLitGeneric" {
          	"$additive"							"1"
			"$envmap"							"models/effects/cube_white"
			"$envmaptint"						"[255 0 0]"
			"$envmapfresnel"					"1"
			"$envmapfresnelminmaxexp"			"[0 255 18]"
			"$alpha""1"
        })#"));

		m_materials.m_glass = create_material(true, STRS(R"#("VertexLitGeneric" {
			"$baseTexture" 						"black"
			"$bumpmap"							"models\inventory_items\trophy_majors\matte_metal_normal"
			"$additive"							"1"
			"$envmap"							"Editor\cube_vertigo"
			"$envmapcontrast"					"16"
			"$envmaptint"						"[.2 .2 .2]"
			"$envmapsaturation"					"[.5 .5 .5]"
			"$envmapfresnel"					"1"
			"$normalmapalphaenvmapmask"			"1"
			"$phong"							"1"
			"$phongfresnelranges"				"[.1 .4 1]"
			"$phongboost"						"20"
			"$phongtint"						"[.8 .9 1]"
			"$phongexponent"					"3000"
			"$phongdisablehalflambert"			"1"
        })#"));

		m_materials.m_metallic = create_material(true, STRS(R"#("VertexLitGeneric" {
			"$basetexture"						"vgui/white"
			"$phong"							"1"
			"$phongexponent"					"10"
			"$phongboost"						"1.0"
			"$rimlight"							"1"
			"$rimlightexponent"					"1"
			"$rimlightboost"					"1"
			"$model"							"1"
			"$nocull"							"0"
			"$halflambert"						"0"
			"$lightwarptexture"					"metalic"
		})#"));

		materials_created = true;
	}
} // namespace esp