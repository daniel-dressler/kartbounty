#pragma once
#include <string>
#include <map>

#include "../../Standard.h"

typedef int32_t entity_id;
typedef int32_t powerup_id_t;

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

		private:
			entity_id id;
		protected:
			std::string name;

		public:
			// -- Read Only Values
			std::string GetName() { return name; };
			entity_id GetId() { return id; };
	};

	typedef enum powerup_t {
		NullPowerup, // default
		BulletPowerup,
		GoldCasePowerup,
		GoldCoinPowerup,
		SpeedPowerup,
		HealthPowerup,
		RocketPowerup,
		PulsePowerup,
		FloatingGoldPowerup
	} powerup_t;

	class CarEntity : public Entity {
		public:
		CarEntity(std::string name);
		powerup_t powerup_slot;
		int playerNumber;		// Indicated which player this kart is for this round, different than id
		bool isHumanPlayer;

		// Kart
		Vector3 Pos;
		btVector3 respawnLocation;
		Quaternion Orient;
		Float64 health; // 1.0 == full 0.0 == death
		bool isExploding;
		Int64 gold;
		Real heightOffGround;
		Vector3 groundHit;
		Vector3 groundNormal;

		Vector3 tirePos[4];
		Quaternion tireOrient[4];

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
		float shoot_timer; // cooldown on shooting
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
