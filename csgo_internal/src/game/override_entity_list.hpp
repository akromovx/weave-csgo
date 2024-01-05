#pragma once
#include "../../sdk/entity_list.hpp"
#include "players.hpp"
#include "weapons.hpp"
#include "grenades.hpp"
#include <vector>
#include <mutex>

template<typename entity_t, typename entry_t>
struct entity_list_t : public std::vector<std::pair<entity_t, entry_t>> {};

struct override_listener_t : public sdk::entity_listener_t {
	std::mutex m_mutex;

	::entity_list_t<sdk::cs_player_t*, player_entry_t> m_players{};
	::entity_list_t<sdk::base_combat_weapon_t*, weapon_entry_t> m_weapons{};
	::entity_list_t<sdk::base_grenade_t*, projectile_entry_t> m_projectiles{};
	::entity_list_t<sdk::base_grenade_t*, inferno_entry_t> m_infernos{};

	uintptr_t m_player_resource{};

	virtual void on_entity_created(sdk::base_entity_t* entity) override;
	virtual void on_entity_deleted(sdk::base_entity_t* entity) override;

	void init();
	void remove();
};

GLOBAL_DYNPTR(override_listener_t, entities);