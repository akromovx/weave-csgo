#include "../globals.hpp"
#include "../interfaces.hpp"

#include "../features/hvh/exploits.hpp"
#include "../features/hvh/hvh.hpp"
#include "../features/misc.hpp"

#include "../utils/animation_handler.hpp"
#include "../utils/threading.hpp"

#include "../features/network.hpp"
#include "dormant.hpp"
#include "override_entity_list.hpp"
#include "players.hpp"
#include "voice_channel.hpp"

using namespace sdk;

STFI void clamp_bones_in_bbox(cs_player_t* player, matrix3x4_t* bones, uint32_t bone_mask) {
	patterns::clamp_bones_in_bbox.as<void(__thiscall*)(cs_player_t*, matrix3x4_t*, uint32_t)>()(player, bones, bone_mask);
}

#define ID_PLAYER_INFO XOR32(voice_channel_t::get_id(voice_channel_t::e_msg_type::player_info))

inline bool players_t::voice_data_received_t::is_expired() const {
	return std::abs(interfaces::global_vars->m_curtime - m_time) > 1.f || !m_verified;
}

#pragma optimize("", off)

void player_entry_t::update_box() {
	m_dormant = m_player->dormant();

	int backup_flags = m_player->flags();
	vec3d backup_origin = m_player->render_origin();

	auto idx = m_player->index();
	auto& sound_player = esp::dormant->m_sound_players[idx];
	const auto& shared_player = players->m_shared[idx];
	auto shared_origin = vec3d{ (float)shared_player.m_data.m_origin_x, (float)shared_player.m_data.m_origin_y, (float)shared_player.m_data.m_origin_z };
	if (shared_player.is_expired() || !m_dormant)
		shared_origin = m_render_origin;

	m_box.m_got = m_box.get(shared_origin, m_duck_amount);

	if (!m_box.m_got)
		m_box.reset_bounds();

	if (shared_player.m_friend_cheat == 1)
		m_player->personal_data_public_level() = XOR32(4444);
	else if (shared_player.m_friend_cheat == 2)
		m_player->personal_data_public_level() = XOR32(2512);
	else if (shared_player.m_friend_cheat == 3)
		m_player->personal_data_public_level() = XOR32(44440);
	else if (shared_player.m_friend_cheat == 6)
		m_player->personal_data_public_level() = XOR32(2001);
	else if (shared_player.m_friend_cheat == 7)
		m_player->personal_data_public_level() = XOR32(25120);

	if (!m_player->is_alive() || m_player->is_teammate()) {
		animations_fsn->lerp(m_visual_alpha, 0.f, 2.f);
		m_health = 0;
		animations_fsn->ease_lerp(m_visual_health, (float)m_health, 14.f);
		return;
	}

	if (m_dormant) {
		if (shared_player.is_expired()) {
			if (esp::dormant->is_sound_expired(m_player, m_visual_alpha)) {
				animations_fsn->lerp(m_visual_alpha, 0.f, 1.f);
				animations_fsn->ease_lerp(m_visual_health, (float)m_health, 8.f);
				return;
			}

			auto hud_radar = misc->get_hud_radar();
			if (hud_radar != nullptr && m_player->dormant() && m_player->spotted())
				m_health = hud_radar->m_radar_info[idx].m_health;
		} else {
			m_health = shared_player.m_data.m_health;

			if (std::abs(interfaces::global_vars->m_curtime - shared_player.m_time) < 0.5f)
				m_visual_alpha = 0.8f;
		}
	} else {
		m_health = m_player->health();
		sound_player.set(true, m_player->origin(), m_player->flags());
	}

	animations_fsn->ease_lerp(m_visual_health, (float)m_health, 14.f);
	if (m_health <= 0) {
		if (m_dormant) {
			animations_fsn->lerp(m_visual_alpha, 0.f, 1.0f);
		}

		return;
	}

	m_friend_cheat = players->m_shared[idx].m_friend_cheat;

	if (!m_dormant) {
		animations_fsn->lerp(m_visual_alpha, 1.f);
		sound_player.m_last_seen_time = sound_player.m_recieve_time = interfaces::global_vars->m_realtime;
	} else
		animations_fsn->lerp(m_visual_alpha, 0.4f, 0.5f);
}

#pragma optimize("", on)

void player_entry_t::update_data() {
	if (entities->m_mutex.try_lock()) {
		m_valid_for_esp = !m_player->is_teammate() && m_player != globals->m_local;
		m_duck_amount = m_player->duck_amount();
		m_name = m_player->name();
		m_health = m_player->health();
		m_scoped = m_player->scoped();
		m_flashed = m_player->flash_amount() > 0.f;
		m_has_kit = m_player->has_defuser();
		m_has_armor = m_player->armor() > 0;
		m_has_helmet = m_player->helmet();
		m_ping = m_player->ping();

		if (!globals->m_local->is_alive()) {
			m_zeus_warning = false;
			m_distance = -1;
		} else
			m_distance = globals->m_local->origin().dist(m_player->origin());

		auto player_weapon = m_player->active_weapon();
		if (player_weapon != nullptr) {
			m_zeus_warning = false;

			if (player_weapon->item_definition_index() == e_weapon_type::weapon_zeusx27 && globals->m_local->is_alive()) {
				if (m_distance <= 600 && m_distance > 0)
					m_zeus_warning = true;
			}

			m_weapon.m_valid = true;

			if (player_weapon->is_weapon()) {
				m_weapon.m_ammo = player_weapon->ammo();
				auto weapon_info = player_weapon->get_cs_weapon_info();
				if (weapon_info != nullptr) {
					m_weapon.m_max_ammo = weapon_info->m_max_ammo_1;

					m_weapon.m_name = interfaces::localize->find_utf8(weapon_info->m_hud_name);
					m_weapon.m_icon = (char*)misc->get_weapon_icon(player_weapon->item_definition_index());
				}

				auto& layer = m_player->animation_layers()[1];
				if (layer.m_owner) {
					auto sequence = m_player->get_sequence_activity(layer.m_sequence);
					m_weapon.m_in_reload = sequence == XOR32(967) && layer.m_weight != 0.f;
					m_weapon.m_reload_ratio = layer.m_cycle;
				}
			} else
				m_weapon.m_ammo = -1;
		} else
			m_weapon.m_valid = false;

		entities->m_mutex.unlock();
	}
}

void players_t::for_each_entry(std::function<void(player_entry_t*)> callback) {
	THREAD_SAFE(m_mutex);

	for (auto& [player, entry]: entities->m_players)
		callback(&entry);
}

player_entry_t* players_t::find_entry(cs_player_t* player) {
	THREAD_SAFE(m_mutex);

	auto it = std::find_if(entities->m_players.begin(), entities->m_players.end(), [player](std::pair<cs_player_t*, player_entry_t>& info) {
		return info.first == player;
	});

	if (it != entities->m_players.end())
		return &it->second;

	return nullptr;
}

#pragma optimize("", off)

STFI void update_viewmodel() {
	auto viewmodel = globals->m_local->viewmodel_handle().get();
	if (viewmodel != nullptr)
		patterns::update_all_viewmodel_addons.as<int(__fastcall*)(void*)>()(viewmodel);
}

void players_t::on_render_start() {
	if (!ctx->is_valid())
		return;

	globals->m_local_alive = globals->m_local->is_alive();
	globals->m_local_hp = globals->m_local_alive ? globals->m_local->health() : 0;

#ifdef _DEBUG
	globals->m_local->personal_data_public_level() = 44440;
#else
	globals->m_local->personal_data_public_level() = XOR32(4444);
#endif

	if (globals->m_local_alive) {
		if (!hvh::exploits->m_charging)
			update_viewmodel();
	} else
		players->m_local_player.m_valid = false;

	THREAD_SAFE(entities->m_mutex);

	for (auto& [player, entry]: entities->m_players) {
		if (player == globals->m_local)
			continue;

		entry.m_player = player;
		// esp::dormant->start();


		entry.update_box();
	}
}

#pragma optimize("", on)

void players_t::on_paint_traverse() {
	THREAD_SAFE(m_mutex);

	for (auto& [player, entry]: entities->m_players) {
		if (player == globals->m_local)
			continue;

		if (player->is_alive()) {
			entry.m_render_origin = player->render_origin();
			entry.m_view_offset = player->view_offset();
		}
	}
}

STFI void send_player_info(cs_player_t* player) {
	players_t::shared_esp_data_t data{};
	data.m_id = ID_PLAYER_INFO;

	const auto& origin = player->origin();
	data.m_origin_x = (int16_t)origin.x;
	data.m_origin_y = (int16_t)origin.y;
	data.m_origin_z = (int16_t)origin.z;
	data.m_user_id = player->index();
	data.m_health = player->health();
	data.m_tick = interfaces::global_vars->m_tickcount;

	data.m_user_info.m_id = network::user_id & 0xFFFF;
	data.m_user_info.m_boss = 0;
	data.m_user_info.m_crash = 0;

	utils::bytes_t bytes{};
	bytes.resize(sizeof(data));
	std::memcpy(bytes.data(), &data, sizeof(data));
	voice_channel->send_data(bytes);
}

static std::mutex players_mtx{};

STFI void process_player(int index) {
	auto& [player, entry] = entities->m_players[index];

	players_mtx.lock();
	const auto is_local = player == globals->m_local;
	players_mtx.unlock();

	if (is_local)
		return;

	auto& records = entry.m_records;
	entry.m_player = player;

	if (!player->is_alive()) {
		entry.m_initialized = false;
		if (!records.empty()) records.clear();
		return;
	}

	if (player->dormant()) {
		entry.m_initialized = false;
		entry.m_last_dormant_tick = true;
		if (!records.empty()) records.clear();
		return;
	}

	if (records.empty()) {
		entry.m_initialized = true;
		records.resize(std::min<size_t>((size_t)globals->m_tickrate, 64));

		const auto state = player->animstate();
		if (state != nullptr)
			state->reset();
	}

	entry.m_previous = &records[records.size() - 1];
	entry.m_prelast_record = &records[records.size() - 2];

	// owner will be nullptr if record doesn't exists
	// then set it to nullptr to check it later
	if (entry.m_previous->m_player == nullptr)
		entry.m_previous = nullptr;

	if (entry.m_prelast_record->m_player == nullptr)
		entry.m_prelast_record = nullptr;

	auto update = entry.m_previous == nullptr || player->simtime() != player->old_simtime();
	const auto came_from_dormant = entry.m_last_dormant_tick;

	if (came_from_dormant) {
		entry.m_previous = nullptr;
		entry.m_prelast_record = nullptr;
		entry.m_time_came_dormant = interfaces::global_vars->m_curtime;

		// when player is coming out of dormancy we don't know if he's breaking tickbase
		// so we need to delay atleast 16 ticks to make sure he's not breaking tickbase
		// in fact we're setting this value like it guy was shifting this tick and his tickbase is broken
		//entry.m_old_simulation_time = player->simtime() + TICKS_TO_TIME(16); // nigger
	}

	if (update && entry.m_previous != nullptr) {
		players_mtx.lock();

		uint32_t server_tick = interfaces::client_state->m_clock_drift_manager.m_server_tick - player->index() % interfaces::global_vars->m_time_stamp_randomize_window;
		uint32_t current_tick = server_tick - server_tick % interfaces::global_vars->m_time_stamp_networking_base;

		players_mtx.unlock();

		float old_simtime = player->old_simtime(), simtime = player->simtime();

		if (TIME_TO_TICKS(old_simtime) < (int)current_tick && TIME_TO_TICKS(simtime) == (int)current_tick) {
			auto layer = &player->animation_layers()[XOR32(11)];
			auto prev_layer = &entry.m_previous->m_layers[XOR32(11)];

			if (layer->m_cycle == prev_layer->m_cycle) {
				player->simtime() = old_simtime;
				update = false;
			}
		}
	}

	if (!update)
		return;

	players->m_backup_records[player->index()].store_data(entry);

	auto& new_record = records.add();
	new_record.store_data(entry);

	if (entry.m_previous != nullptr) {
		if (entry.m_previous->m_lag >= 2 && new_record.m_lag >= 2)
			++entry.m_choked_ticks;
		else
			entry.m_choked_ticks = 0;
	}

	new_record.m_may_has_fake = entry.m_choked_ticks > 2;
	new_record.m_came_from_dormant = came_from_dormant;
	new_record.m_valid = true;

	if (entry.m_previous != nullptr) {
		entry.m_previous->m_next = &new_record;
		new_record.m_previous = entry.m_previous;
	}

	entry.update_animations();

	players->m_backup_records[player->index()].restore();

	if (CVAR_BOOL("cl_lagcompensation")) {
		if (entry.m_previous != nullptr) {
			if (entry.m_old_simulation_time > player->simtime())
				new_record.m_valid = false;
			else if (player->simtime() <= player->old_simtime())
				new_record.m_valid = false;
			else
				entry.m_old_simulation_time = player->simtime();

			if (player->old_simtime() > player->simtime())
				new_record.m_valid = false;
		}
	} else {
		for (int last_index = records.size() - 1, i = last_index; i >= 0; --i) {
			auto& record = records[i];
			record.m_valid = false;
			record.m_valid_check = false;
			record.m_valid_visual = false;
		}

		new_record.m_valid = true;
		new_record.m_valid_check = true;
		new_record.m_valid_visual = true;
	}

	entry.update_data();
	entry.m_last_dormant_tick = false;
}

void players_t::on_net_update_end() {
	THREAD_SAFE(m_mutex);

	static auto r_jiggle_bones = GET_CVAR("r_jiggle_bones");
	if (r_jiggle_bones->get_int() != 0) {
		r_jiggle_bones->m_fn_change_callbacks.m_size = 0;
		r_jiggle_bones->set_value(0);
	}

	if (!interfaces::engine->is_connected() || globals->m_local == nullptr || globals->m_tickrate <= 0.f || !ctx->is_valid()) {
		for (auto& [player, entry]: entities->m_players) {
			if (!entry.m_records.empty()) entry.m_records.clear();
			entry.m_valid_data = false;
		}

		return;
	}

	threading_t::callbacks_t callbacks{};
	callbacks.reserve(entities->m_players.size());
	for (size_t i = 0; i < entities->m_players.size(); ++i)
		callbacks.emplace_back([i]() { process_player(i); });

	threading->run(callbacks);

	auto nci = interfaces::engine->get_net_channel();
	if (nci != nullptr && !nci->is_loopback()) {
		for (auto& [player, entry]: entities->m_players) {
			if (player->dormant() || !player->is_alive())
				continue;

			if (player->simtime() != player->old_simtime())
				send_player_info(player);
		}
	}
}

STFI bool is_playing_on_this_server(int user_id) {
	const auto& users = network::online;
	if (!users.has_value())
		return false;

	std::string ip{};
	if (auto nci = interfaces::engine->get_net_channel(); nci != nullptr)
		ip = nci->get_address();

	if (ip.empty())
		return false;

	for (const auto& user : users->m_users)
		if (user.m_id == user_id && user.m_ip == ip)
			return true;

	return false;
}

void players_t::on_voice_data_received(svc_msg_voice_data_t* msg) {
	voice_communication_data_t voice_data = msg->data();
	if (msg->m_format != 0)
		return;

	if (globals->m_local == nullptr || interfaces::engine->get_local_player() == msg->m_client + 1)
		return;

	if (voice_data.m_section_number == 0 && voice_data.m_sequence_bytes == 0 && voice_data.m_uncompressed_sample_offset == 0)
		return;

	const players_t::shared_esp_data_t& data = *(players_t::shared_esp_data_t*)(voice_data.data());
	const bool valid_id = (ID_PLAYER_INFO & ~0xFF00) == (data.m_id & ~0xFF00);

	if (!valid_id || data.m_user_id > 63 || data.m_user_id < 0) {
		return;
	}

	auto sender_id = msg->m_client + 1;
	if (sender_id >= 0 && sender_id <= 63) {
		auto& sender = players->m_shared[sender_id];

		sender.m_verified = is_playing_on_this_server(data.m_user_info.m_id);

		if (!sender.m_verified) {
			sender.m_friend_cheat = 0;
			sender.m_time = 0.f;
			return;
		}

		const auto old_id = sender.m_friend_cheat;

		switch (voice_channel_t::get_id(data.m_id)) {
			case voice_channel_t::weave_id:
				sender.m_friend_cheat = 1;
				break;
			case voice_channel_t::boss_id:
				sender.m_friend_cheat = 3;
				break;
				//case voice_channel_t::airflow_id: sender.m_friend_cheat = 2; break;
				//case voice_channel_t::airflow_boss_id: sender.m_friend_cheat = 2; break;
				//case voice_channel_t::furcore_id: sender.m_friend_cheat = 4; break;
				//case voice_channel_t::floridahook_id: sender.m_friend_cheat = 5; break;
				//case voice_channel_t::karnazity: sender.m_friend_cheat = 6; break;
			default:
				sender.m_friend_cheat = 0;
				break;
		}

		if (sender.m_friend_cheat != 0) {
			auto& shared = players->m_shared[data.m_user_id];
			if (data.m_tick > shared.m_data.m_tick) {
				std::memcpy(&shared.m_data, &data, sizeof(data));
				shared.m_time = interfaces::global_vars->m_curtime;
			}
		}
	}
}