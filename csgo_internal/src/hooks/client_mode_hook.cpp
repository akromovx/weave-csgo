#include "hooker.hpp"
#include "hooks.hpp"

#include "../globals.hpp"
#include "../interfaces.hpp"

#include "../features/hvh/exploits.hpp"
#include "../features/hvh/hvh.hpp"
#include "../features/hvh/peek_state.hpp"

#include "../features/knifebot.hpp"
#include "../features/bullets.hpp"
#include "../features/misc.hpp"
#include "../features/movement.hpp"

#include "../features/visuals/chams.hpp"
#include "../features/visuals/glow_esp.hpp"

#include "../game/engine_prediction.hpp"
#include "../game/grenades.hpp"

namespace hooks::client_mode {
	using namespace sdk;

	bool __fastcall should_draw_fog(void* ecx, void* edx) { return !settings->visuals.removals.at(5); }

	bool __stdcall create_move(float input_sample_frametime, user_cmd_t* cmd) {
		static auto original = vmt::client_mode->original<bool(__stdcall*)(float, user_cmd_t*)>(XOR32(24));

		const bool in_shift = hvh::exploits->m_in_shift;
		if (in_shift)
			*globals->m_send_packet = interfaces::client_state->m_choked_commands >= MAX_FAKELAG;

		if (cmd == nullptr || cmd->m_command_number == 0 || globals->m_local == nullptr || !ctx->is_valid() || interfaces::engine->is_hltv())
			return original(input_sample_frametime, cmd);

		if (!globals->m_local->is_alive())
			return original(input_sample_frametime, cmd);

		//interfaces::model_cache->begin_lock();

		globals->m_tickbase = [&]() {
			static int tickbase = 0;

			if (cmd != nullptr) {
				static user_cmd_t* last_cmd = nullptr;

				// if command was not predicted - increment tickbase
				if (last_cmd == nullptr || last_cmd->m_has_been_predicted)
					tickbase = globals->m_local->tickbase();
				else
					tickbase++;

				last_cmd = cmd;
			}

			return tickbase;
		}();

		globals->m_weapon = globals->m_local->active_weapon();
		globals->m_cmd = cmd;
		globals->m_original_angle = cmd->m_viewangles;

		if (globals->m_weapon != nullptr) {

			if (!interfaces::game_rules->is_valve_ds())
				cmd->m_buttons.add(in_bullrush);

			engine_prediction->store_unpredicted_data(cmd);

			if (!in_shift)
				bullet_impacts->on_pre_move();

			::movement->on_pre_move(cmd, globals->m_original_angle);
			hvh::on_pre_move(cmd);

			engine_prediction->begin(cmd);

			globals->m_weapon_inaccuracy = globals->m_weapon->get_inaccuracy();
			globals->m_weapon_spread = globals->m_weapon->get_spread();
			globals->m_eye_position = hvh::aimbot->get_shoot_position();


			projectiles->on_create_move(cmd);
			players->on_create_move();
			::movement->on_create_move(cmd);

			if (!in_shift) {
				peek_state->on_create_move();
				resolver->on_create_move();
			}

			hvh::on_create_move(cmd);
			knifebot->on_create_move(cmd);

			if (!in_shift)
				hvh::exploits->on_create_move(cmd);

			auto is_shooting = hvh::is_shooting(cmd);

			if (settings->ragebot.enable && globals->m_weapon->item_definition_index() == e_weapon_type::weapon_revolver)
				is_shooting = cmd->m_buttons.has(in_attack) && hvh::aimbot->m_revolver_fire;

			if (/*hvh::exploits->m_type != hvh::exploits_t::e_type::hideshot && */ globals->m_weapon->item_definition_index() != e_weapon_type::weapon_revolver) {
				if (!is_shooting && globals->m_weapon->is_gun())
					cmd->m_buttons.remove(globals->m_weapon->is_knife() ? (in_attack | in_attack2) : in_attack);
			}

			if (is_shooting && !hvh::aimbot->m_shot) {
				if (settings->ragebot.enable)
					cmd->m_viewangles -= globals->m_local->punch_angle() * CVAR_FLOAT("weapon_recoil_scale");
			}

			static int shot_command_number = 0;

			if (is_shooting) {
				engine_prediction->m_last_data.m_shot = true;
				shot_command_number = cmd->m_command_number;
				hvh::fakelag->m_need_reset_shot = true;

				if (hvh::aimbot->m_on_weapon_fire != nullptr) {
					hvh::aimbot->m_on_weapon_fire(&hvh::aimbot->m_last_fired_record);
					hvh::aimbot->m_on_weapon_fire = nullptr;
				}

				globals->m_last_shoot_position = globals->m_eye_position;
				globals->m_last_shoot_angles = cmd->m_viewangles;

				if (hvh::exploits->m_dt_shots > 0)
					++hvh::exploits->m_dt_shots;

				if (globals->m_weapon->is_gun()) {
					auto& tracer = bullet_tracer->push();
					tracer.m_eye_position = globals->m_last_shoot_position - vec3d{ 0.f, 0.f, 1.f };
					tracer.m_time = interfaces::global_vars->m_curtime;
				}
			} else {
				hvh::aimbot->m_shot = false;
				engine_prediction->m_last_data.m_shot = false;
			}

			if (cmd->m_command_number >= shot_command_number && shot_command_number >= cmd->m_command_number - interfaces::client_state->m_choked_commands)
				players->m_local_player.m_current_angle = interfaces::input->get_user_cmd(shot_command_number)->m_viewangles;
			else
				players->m_local_player.m_current_angle = cmd->m_viewangles;

			hvh::aimbot->m_on_weapon_fire = nullptr;

			if (!in_shift)
				::movement->on_post_move(cmd, globals->m_original_angle);

			::movement->fix(cmd, globals->m_original_angle);

			if (globals->m_local->velocity().length() < 5.f) {
				if (cmd->m_command_number % 16 == 0)
					globals->m_nades_eye_position = globals->m_eye_position;
			} else
				globals->m_nades_eye_position = globals->m_eye_position;

			globals->m_old_velocity = globals->m_local->velocity();
			cmd->m_viewangles = math::clamp_angles(cmd->m_viewangles);

			if (!in_shift) {
				peek_state->on_post_move();

				if (interfaces::client_state->m_choked_commands >= MAX_FAKELAG)
					*globals->m_send_packet = true;

				players->update_local_player(cmd);

				auto& out = globals->m_cmds.emplace_back();
				out.is_outgoing = *globals->m_send_packet;
				out.is_used = false;
				out.cmd_number = cmd->m_command_number;
				out.previous_command_number = 0;

				if (globals->m_cmds.size() > (size_t)globals->m_tickrate)
					globals->m_cmds.erase(globals->m_cmds.begin() + (size_t)globals->m_tickrate, globals->m_cmds.end());

				if (!interfaces::game_rules->is_valve_ds()) {
					auto net_channel = interfaces::client_state->m_net_channel;
					if (!*globals->m_send_packet && net_channel->m_choked_packets > 0 && !(net_channel->m_choked_packets % 4)) {
						int backup_choke = net_channel->m_choked_packets;
						net_channel->m_choked_packets = 0;

						net_channel->send_datagram();
						--net_channel->m_out_sequence_nr;

						net_channel->m_choked_packets = backup_choke;
					}
				}

				hvh::fakelag->m_last_send_packet = *globals->m_send_packet;

				if (hvh::exploits->m_recharge_ticks > 0) {
					hvh::exploits->m_charging = true;

					if (globals->m_weapon->item_definition_index() == e_weapon_type::weapon_revolver)
						cmd->m_buttons.remove(in_attack | in_attack2);
				}
			}

			engine_prediction->end(cmd);

		}

		return false;
	}

	STFI void vec_ma(const vec3d& start, float scale, const vec3d& direction, vec3d& dest) {
		dest.x = start.x + direction.x * scale;
		dest.y = start.y + direction.y * scale;
		dest.z = start.z + direction.z * scale;
	}

	void __stdcall override_view(view_setup_t* view_setup) {
		static auto original = vmt::client_mode->original<void(__stdcall*)(view_setup_t*)>(XOR32(18));

		if (ctx->unload)
			return original(view_setup);

		if (interfaces::engine->is_connected() && globals->m_local != nullptr) {
			const auto fov = std::clamp((float)settings->visuals.world_fov, 0.f, 50.f);

			if (globals->m_local_alive) {
				if (settings->visuals.removals.at(0))
					view_setup->m_angles -= globals->m_local->punch_angle() * 0.9f + globals->m_local->view_punch_angle();

				projectiles->on_override_view();

				if (globals->m_zoom_level > 0)
					view_setup->m_fov = (90.f + fov - (50.f * (settings->visuals.zoom_fov / 100.f))) / globals->m_zoom_level;
				else
					view_setup->m_fov = (90.f + fov);
			} else
				view_setup->m_fov = (90.f + fov);

			::misc->on_override_view();

			original(view_setup);

			if (globals->m_local_alive && globals->m_fake_duck) {
				view_setup->m_origin = globals->m_local->render_origin() + vec3d{ 0.0f, 0.0f, interfaces::game_movement->get_player_view_offset(false).z + 0.064f };

				if (interfaces::input->m_camera_in_third_person) {
					const auto camera_angles = vec3d{ interfaces::input->m_camera_offset.x, interfaces::input->m_camera_offset.y, 0.0f };
					vec3d camera_forward{};

					math::angle_vectors(camera_angles, camera_forward);
					vec_ma(view_setup->m_origin, -interfaces::input->m_camera_offset.z, camera_forward, view_setup->m_origin);
				}
			}
		} else
			original(view_setup);
	}

	float __stdcall get_viewmodel_fov() {
		return settings->visuals.viewmodel_fov == 0 ? CVAR_FLOAT("viewmodel_fov") : (float)settings->visuals.viewmodel_fov;
	}

	bool __fastcall do_post_screen_effects(void* ecx, void* edx, view_setup_t* setup) {
		static auto original = vmt::client_mode->original<bool(__thiscall*)(void*, view_setup_t*)>(XOR32(44));

		if (ctx->unload)
			return original(ecx, setup);

		esp::chams->on_post_screen_effects();
		esp::glow->on_post_screen_effects();

		return original(ecx, setup);
	}
} // namespace hooks::client_mode
