#include <map>
#include "entities.h"

using namespace Entities;

static entity_id entity_id_next = 42;
Entity::Entity()
{
	this->id = entity_id_next++;
	this->gold = 0;
	this->health = 1.0;
}

Entity Inventory::FindEntity(entity_id id)
{
	entity_store.at(id);
}

bool Inventory::Contains(entity_id id)
{
	return entity_store.count(id) == 1;
}

void Inventory::AddEntity(Entity entity)
{
	entity_store[entity.GetId()] = entity;
}


