#pragma once
#include "../../Standard.h"
#include <string>
#include <map>

typedef UInt64 entity_id;

// Entities do not define models or
// sounds or similar. Instead the components
// should match the entity type
// to data.
namespace Entities {
	// This is not encapsulated.
	// Make sure you issue an event
	// when you change most things.
	class Entity {
		public:
			Entity();
			// -- Read & Write Values
			// Position
			Float64 health; // 1.0 == full 0.0 == death
			Int64 gold;

		private:
			entity_id id;
		protected:
			std::string name;

		public:
			// -- Read Only Values
			std::string GetName() { return name; };
			entity_id GetId() { return id; };
	};

	typedef enum {
		NullPowerup, // default
		BulletPowerup,
		GoldCasePowerup,
		GoldCoinPowerup
	} powerup_t;

	class PowerupEntity : public Entity {
		powerup_t type;
	};

	class CarEntity : public Entity {
		public:
		PowerupEntity *powerup_slot;

		CarEntity(std::string name) : Entity()
		{
			this->name = name;
			this->powerup_slot = NULL;
		}
	};

	// Thin wrapper over map
	class Inventory {
		public:
		Entity FindEntity(entity_id id);
		bool Contains(entity_id id);
		void AddEntity(Entity entity);

		private:
		std::map<entity_id, Entity> entity_store;
	};
}
