#include <map>
#include "entities.h"

Entities::Inventory *g_inventory;

void init_inventory() {
	if (g_inventory == NULL) {
		g_inventory = new Entities::Inventory();
	}
}

void shutdown_inventory() {
	if (g_inventory != NULL) {
		delete g_inventory;
		g_inventory = NULL;
	}
}

using namespace Entities;

static entity_id entity_id_next = 42;
Entity::Entity()
{
	this->id = entity_id_next++;
	this->gold = 0;
	this->health = 1.0;
}

Entity *Inventory::FindEntity(entity_id id)
{
	if (this->Contains(id) != 1)
		return NULL;

	return entity_store.at(id);
}

bool Inventory::Contains(entity_id id)
{
	return entity_store.count(id) == 1;
}

entity_id Inventory::AddEntity(Entity *entity)
{
	entity_store[entity->GetId()] = entity;
}


