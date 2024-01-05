#include "hooks.hpp"
#include "hooker.hpp"
#include "../interfaces.hpp"

#include "../features/visuals/chams.hpp"

namespace hooks::model_render {
	using namespace sdk;
	using namespace esp;

	STFI void remove_smoke() {
		static auto mat1 = interfaces::material_system->find_material(STRC("particle/vistasmokev1/vistasmokev1_smokegrenade"), STRC("Other textures"));
		static auto mat2 = interfaces::material_system->find_material(STRC("particle/vistasmokev1/vistasmokev1_emods"), STRC("Other textures"));
		static auto mat3 = interfaces::material_system->find_material(STRC("particle/vistasmokev1/vistasmokev1_emods_impactdust"), STRC("Other textures"));
		static auto mat4 = interfaces::material_system->find_material(STRC("particle/vistasmokev1/vistasmokev1_fire"), STRC("Other textures"));

		const auto flag = settings->visuals.removals.at(1);
		mat1->set_material_var_flag(e_material_flags::material_var_no_draw, flag);
		mat2->set_material_var_flag(e_material_flags::material_var_no_draw, flag);
		mat3->set_material_var_flag(e_material_flags::material_var_no_draw, flag);
		mat4->set_material_var_flag(e_material_flags::material_var_no_draw, flag);

		if (flag)
			**patterns::smoke_count.as<int**>() = 0;
	}

	void __fastcall draw_model_execute(void* ecx, void* edx, void* ctx, const draw_model_state_t& state, const model_render_info_t& info, matrix3x4_t* bone_to_world) {
		static auto original = vmt::model_render->original<
				void(__thiscall*)(void*, void*, const draw_model_state_t&, const model_render_info_t&, matrix3x4_t*)>(XOR32(21));

		// skip glow / skin / custom model calls
		if (interfaces::studio_render->is_forced_material_override())
			return original(interfaces::model_render, ctx, state, info, bone_to_world);

		if (info.m_model != nullptr) {
			if (settings->visuals.removals.at(7) && std::strstr(info.m_model->m_name, STRC("player/contactshadow")) != nullptr)
				return;

			if (std::strstr(info.m_model->m_name, STRC("props")) != nullptr)
				interfaces::render_view->set_blend(interfaces::render_view->get_blend() * settings->visuals.nightmode.prop_color.a() / 255.f);

			remove_smoke();
		}

		// don't render models and other shit twice
		if (!chams->is_working(state, info, [&](matrix3x4_t* matrix) {
				original(interfaces::model_render, ctx, state, info, matrix != nullptr ? matrix : bone_to_world);
			}))
			original(interfaces::model_render, ctx, state, info, bone_to_world);

		interfaces::model_render->forced_material_override(nullptr);
		interfaces::render_view->set_blend(1.f);
	}
} // namespace hooks::model_render