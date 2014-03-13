#pragma once
#include <string>
#include <map>

#include "../../Standard.h"

typedef uint64_t entity_id;

// Entities do not define models or
// sounds or similar. Instead the system
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

	typedef enum power_up_t {
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
		CarEntity(std::string name);
		PowerupEntity *powerup_slot;

		// Kart
		Vector3 Pos;
		Quaternion Orient;
		// Camera
		struct Camera {
			Real fFOV;
			Vector3 vFocus;
			Vector3 vPos;
			Quaternion orient_old;
		} camera;
		// Misc Data
		btVector3 forDirection;
		Vector3 Up;
		float Speed;
	};

	// Thin wrapper over map
	class Inventory {
		public:
		Entity *FindEntity(entity_id id);
		bool Contains(entity_id id);
		entity_id AddEntity(Entity *entity);

		private:
		std::map<entity_id, Entity *> entity_store;
	};
}

extern Entities::Inventory *g_inventory;
void init_inventory();
void shutdown_inventory();
#define GETENTITY(id, type) static_cast<Entities::type *>(g_inventory->FindEntity(id))
