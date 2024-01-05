#include "../src/game/override_entity_list.hpp"
#include "../src/globals.hpp"
#include "../src/interfaces.hpp"
#include "trace_define.hpp"

namespace sdk {
	inline void* base_handle_t::get() {
		return interfaces::entity_list->get_client_entity_handle(m_value);
	}

	int cs_player_t::get_sequence_activity(int seq) {
		auto model = this->renderable()->get_model();
		if (model == nullptr)
			return -1;

		auto hdr = interfaces::model_info->get_studio_model(model);
		if (hdr == nullptr)
			return -1;

		static auto get_sequence_activity_fn = patterns::sequence_activity.as<int(__fastcall*)(void*, studiohdr_t*, int)>();
		return get_sequence_activity_fn(this, hdr, seq);
	}

	std::vector<base_combat_weapon_t*> cs_player_t::get_weapons() {
		auto weapons = this->my_weapons();
		std::vector<base_combat_weapon_t*> list{};
		for (int i = 0; i < 64; ++i) {
			auto weapon = (base_combat_weapon_t*)weapons[i].get();
			if (weapon != nullptr)
				list.emplace_back(weapon);
		}

		return list;
	}

	player_info_t base_player_t::get_player_info() {
		player_info_t info{};
		interfaces::engine->get_player_info(this->index(), &info);
		return info;
	}

	std::string base_player_t::name() {
		auto result = std::string{ get_player_info().m_name };
		ranges::transform(result, result.begin(), [](char c) {
			if (c == '\n' || c == '\r' || c == '\t')
				return ' ';

			return c;
		});

		return result;
	}

	bool base_player_t::is_teammate() {
		if (globals->m_local == nullptr)
			return false;

		return this != globals->m_local && this->team() == globals->m_local->team();
	}

	__forceinline uint8_t* cs_player_t::get_server_edict() {
		static uintptr_t server_addr = **(uintptr_t**)((patterns::get_server_edict) + 0x2);
		int max_clients = *(int*)((uintptr_t)server_addr + 0x18);
		int index = this->index();
		if (index > 0 && max_clients >= 1) {
			if (index <= max_clients) {
				int v10 = index * 16;
				uintptr_t v11 = *(uintptr_t*)(server_addr + 96);
				if (v11 != 0) {
					if (!((*(uintptr_t*)(v11 + v10) >> 1) & 1)) {
						uintptr_t v12 = *(uintptr_t*)(v10 + v11 + 12);
						if (v12 != 0) {
							uint8_t* _return = nullptr;
							__asm
							{
								pushad
								mov ecx, v12
								mov eax, dword ptr[ecx]
								call dword ptr[eax + 0x14]
								mov _return, eax
								popad
							}

							return _return;
						}
					}
				}
			}
		}
		return nullptr;
	}

	bool base_combat_weapon_t::is_knife() {
		auto id = this->client_class();
		if (id == nullptr)
			return false;

		return id->m_class_id == e_class_id::CKnife || id->m_class_id == e_class_id::CKnifeGG;
	}

	bool base_combat_weapon_t::is_projectile() {
		auto id = this->client_class();
		if (id == nullptr)
			return false;

		if (id->m_class_id == e_class_id::CSnowball)
			return true;

		switch (this->item_definition_index()) {
			case weapon_flashbang:
			case weapon_hegrenade:
			case weapon_molotov:
			case weapon_inc:
			case weapon_smokegrenade:
			case weapon_decoy:
				return true;
		}

		return false;
	}

	bool base_combat_weapon_t::is_bomb() {
		auto id = this->client_class();
		if (id == nullptr)
			return false;

		return id->m_class_id == e_class_id::CC4;
	}

	void cs_player_t::draw_server_hitboxes() {
		float duration = interfaces::global_vars->m_interval_per_tick * 2.0f;
		auto server_player = get_server_edict();

		if (server_player != nullptr) {
			__asm {
				pushad
				movss xmm1, duration
				push 1
				mov ecx, server_player
				call patterns::server_hitbox
				popad
			}
		}
	}

	int cs_player_t::ping() {
		static auto ping_offset = netvars->get_offset(XOR32S(HASH("DT_CSPlayerResource")), XOR32S(HASH("m_iPing")));
		if (entities->m_player_resource != 0)
			return *(int*)(entities->m_player_resource + ping_offset + this->index() * 4);

		return -1;
	}

	int& cs_player_t::personal_data_public_level() {
		static int invalid = -1;
		static auto person_data_public_level_offset = netvars->get_offset(XOR32S(HASH("DT_CSPlayerResource")), XOR32S(HASH("m_nPersonaDataPublicLevel")));
		if (entities->m_player_resource != 0)
			return *(int*)(entities->m_player_resource + person_data_public_level_offset + this->index() * 4);

		return invalid;
	}

	float cs_player_t::max_desync() {
		auto state = animstate();
		auto speedfraction = std::max<float>(0.f, std::min<float>(state->m_speed_as_portion_of_walk_top_speed, 1.f));
		auto speedfactor = std::max<float>(0.f, std::min<float>(state->m_speed_as_portion_of_crouch_top_speed, 1.f));

		auto v1 = ((state->m_walk_run_transition * -0.3f) - 0.2f) * speedfraction + 1.f;

		if (state->m_anim_duck_amount > 0.0f)
			v1 += ((state->m_anim_duck_amount * speedfactor) * (0.5f - v1));

		return (state->m_aim_yaw_max * v1);
	}

	bool game_trace_t::did_hit_world() const {
		return m_entity == interfaces::entity_list->get_client_entity(0);
	}

	bool game_trace_t::did_hit_non_world_entity() const {
		return m_entity != nullptr && !did_hit_world();
	}
} // namespace sdk