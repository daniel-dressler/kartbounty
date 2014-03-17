#include <map>

#include <BulletDynamics/Dynamics/btDiscreteDynamicsWorld.h>
#include <btBulletDynamicsCommon.h>
#include <BulletCollision/CollisionDispatch/btInternalEdgeUtility.h>
#include <BulletCollision/CollisionDispatch/btCollisionObject.h>
#include <BulletCollision/CollisionDispatch/btGhostObject.h>
#include <BulletCollision/CollisionShapes/btTriangleShape.h>

#include <BulletDynamics/Vehicle/btRaycastVehicle.h>
#include <BulletDynamics/ConstraintSolver/btHingeConstraint.h>
#include <BulletDynamics/ConstraintSolver/btSliderConstraint.h>

#include "../events/events.h"
#include "../entities/entities.h"

namespace Physics {
	class Simulation {
		public:
		Simulation();
		~Simulation();
		void step(double seconds);

		// after the content loader exists
		// loadWorld will take our models and
		// generate our bounding boxes
		int loadWorld();

		// after entity system exists this
		// will update the entitie's locations
		int saveoutState();

		// Only physics should call this.
		// Needs to be public because 
		// of reasons.
		void substepEnforcer(btDynamicsWorld *, btScalar);


		private:
		Events::Mailbox mb;

		btDiscreteDynamicsWorld *m_world;

		// Handles to objects for GC use
		btAlignedObjectArray<btCollisionShape*> m_collisionShapes;
		btTriangleInfoMap *m_triangleInfoMap;

		std::map<entity_id, btRigidBody *> m_kart_bodies;
		struct phy_obj {
			bool is_kart;
			bool is_powerup;
			bool is_arena;
			// Vehicle
			btRaycastVehicle *vehicle;
			btVehicleRaycaster *raycaster;
			Real lastspeed;
			Vector3 lastofs;
			entity_id kart_id;

			// Powerup
			Entities::powerup_t powerup_type;
			powerup_id_t powerup_id;
			btGhostObject *powerup_body;

			// Arena
			btRigidBody* arena;
			phy_obj() {
				lastspeed = 0;
				is_kart = false;
				is_powerup = false;
				is_arena = false;
				lastofs = Vector3( 0, 1.0f, -1.5f );
			}
		};
		std::map<entity_id, struct phy_obj *> m_karts;
		std::map<powerup_id_t, struct phy_obj *> m_powerups;
		struct phy_obj *m_arena;

		enum col_type_t {
			NULL_TO_NULL,
			KART_TO_KART,
			KART_TO_POWERUP,
			KART_TO_ARENA
		};
		struct col_report {
			entity_id kart_id;
			col_type_t type;
			Real impact;
			Vector3 pos;
			// Kart&Kart
			entity_id kart_id_alt;
			// Powerup
			Entities::powerup_t powerup_type;
			powerup_id_t powerup_id;
		};
		std::map<entity_id, struct col_report> m_col_reports;


		class btBroadphaseInterface* m_broadphase;
		class btCollisionDispatcher* m_dispatcher;
		class btConstraintSolver*    m_solver;
		class btDefaultCollisionConfiguration* m_collisionConfiguration;

		btRigidBody *addRigidBody(double mass, const btTransform& startTransform, btCollisionShape* shape);
		void UpdateGameState(double, entity_id);
		void resetKart(entity_id id);
		void removePowerup(powerup_id_t id);
		void actOnCollision(btPersistentManifold *, phy_obj *A = NULL, phy_obj *B = NULL);
	};
};
