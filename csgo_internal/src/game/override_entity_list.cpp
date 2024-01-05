#include "override_entity_list.hpp"
#include "../interfaces.hpp"
#include "players.hpp"
#include "dormant.hpp"

using namespace sdk;

STFI bool is_inferno(e_class_id id) {
	return id == e_class_id::CInferno;
}

STFI bool is_grenade(e_class_id id) {
	switch (id) {
	case e_class_id::CBaseCSGrenadeProjectile:
	case e_class_id::CBreachChargeProjectile:
	case e_class_id::CBumpMineProjectile:
	case e_class_id::CDecoyProjectile:
	case e_class_id::CMolotovProjectile:
	case e_class_id::CSensorGrenadeProjectile:
	case e_class_id::CSmokeGrenadeProjectile:
	case e_class_id::CSnowballProjectile:
		return true;
	}

	return false;
}

STFI bool is_bomb(e_class_id id) {
	switch (id) {
	case e_class_id::CC4:
	case e_class_id::CPlantedC4:
		return true;
	}

	return false;
}

STFI bool is_weapon(base_entity_t* entity, e_class_id id) {
	return entity->is_weapon() /*|| is_bomb(id)*/;
}

STFI bool is_player_resource(e_class_id id) {
	return id == e_class_id::CCSPlayerResource;
}

template <typename entity_t, typename entry_t>
STFI bool erase_entity(base_entity_t* target, ::entity_list_t<entity_t, entry_t>& vec) {
	auto it = std::find_if(vec.begin(), vec.end(), [=](std::pair<entity_t, entry_t>& info) {
		return info.first == (entity_t)target;
		});

	if (it == vec.end())
		return false;

	vec.erase(it);
	return true;
}

void override_listener_t::on_entity_created(base_entity_t* entity) {
	THREAD_SAFE(m_mutex);

	if (entity == nullptr)
		return;

	int index = entity->index();
	auto client_class = entity->client_class();
	e_class_id id = client_class->m_class_id;

	if (entity->is_player()) {
		m_players.emplace_back((cs_player_t*)entity, player_entry_t{});
		return;
	}

	if (is_weapon(entity, id)) {
		m_weapons.emplace_back((base_combat_weapon_t*)entity, weapon_entry_t{});
		return;
	}

	if (is_grenade(id)) {
		m_projectiles.emplace_back((base_grenade_t*)entity, projectile_entry_t{});
		return;
	}

	if (is_inferno(id)) {
		m_infernos.emplace_back((base_grenade_t*)entity, inferno_entry_t{});
		return;
	}

	if (is_player_resource(id))
		m_player_resource = (uintptr_t)entity;
}

void override_listener_t::on_entity_deleted(base_entity_t* entity) {
	THREAD_SAFE(m_mutex);

	if (entity == nullptr)
		return;

	int index = entity->index();
	if (index < 0)
		return;

	if (erase_entity(entity, m_players)) {
		esp::dormant->m_sound_players[index].set();
		players->m_shared[index].reset();
	}

	erase_entity(entity, m_weapons);
	erase_entity(entity, m_projectiles);
	erase_entity(entity, m_infernos);

	if (m_player_resource == (uintptr_t)entity)
		m_player_resource = 0u;
}

void override_listener_t::init() {
	interfaces::entity_list->add_listener(this);
}

void override_listener_t::remove() {
	interfaces::entity_list->remove_listener(this);
}