#include <map>
#include <list>
#include <vector>

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

static int bullet_next_id;
static int rocket_next_id;

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

		int createKart(entity_id kart_id);

		// after entity system exists this
		// will update the entitie's locations
		int saveoutState();

		// Only physics should call this.
		// Needs to be public because 
		// of reasons.
		void substepEnforcer(btDynamicsWorld *, btScalar);
		
		struct rocket
		{
			// These are the 4 sides (up down left and right) of a rocket
			std::vector<btVector3> positions;
			btVector3 position;
			btVector3 direction;
			float time_to_live; // Once this is 0, the bullet will be removed from the list by physics.
			int rocket_id;
			entity_id kart_id;

			rocket() 
			{
				position = btVector3(0,0,0);

				//bullet_id = bullet_next_id;
				positions.push_back(btVector3(0,0,0));
				positions.push_back(btVector3(0,0,0));
				positions.push_back(btVector3(0,0,0));
				positions.push_back(btVector3(0,0,0));

				direction.setZero();
				time_to_live = 0;
				rocket_id = rocket_next_id;
				rocket_next_id++;
			}
		};

		struct bullet
		{
			btVector3 position;
			btVector3 direction;
			float time_to_live; // Once this is 0, the bullet will be removed from the list by physics.
			int bullet_id;
			entity_id kart_id;

			bullet() 
			{
				//bullet_id = bullet_next_id;
				position = btVector3(0,0,0);
				direction.setZero();
				time_to_live = 0;
				bullet_id = bullet_next_id;
				bullet_next_id++;
			}
		};

		// Used to let AI know whom it should / could shoot and when.
		private:
		Events::Mailbox mb;

		bool gamePaused;

		std::map<int, struct bullet *> list_of_bullets;
		std::map<int, struct rocket *> list_of_rockets;

		std::list<int> rockets_to_remove;

		void handle_bullets(double);

		btDiscreteDynamicsWorld *m_world;

		// Handles to objects for GC use
		btAlignedObjectArray<btCollisionShape*> m_collisionShapes;
		btTriangleInfoMap *m_triangleInfoMap;

		std::map<entity_id, btRigidBody *> m_kart_bodies;
		struct phy_obj 
		{
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
			Vector3 powerup_pos;

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

		enum col_type_t 
		{
			NULL_TO_NULL,
			KART_TO_KART,
			KART_TO_POWERUP,
			KART_TO_ARENA,
			BULLET_TO_ARENA,
			BULLET_TO_KART
		};

		struct col_report 
		{
			entity_id kart_id;
			col_type_t type;
			Real impact;
			Vector3 pos;
			// Kart&Kart
			entity_id kart_id_alt;
			// Powerup
			Entities::powerup_t powerup_type;
			powerup_id_t powerup_id;
			// bullet
			int bullet_id;
		};

		std::map<entity_id, struct col_report> m_col_reports;

		class btBroadphaseInterface* m_broadphase;
		class btCollisionDispatcher* m_dispatcher;
		class btConstraintSolver*    m_solver;
		class btDefaultCollisionConfiguration* m_collisionConfiguration;

		void actOnBulletCollision(struct Simulation::bullet * bullet, phy_obj *B);
		btRigidBody *addRigidBody(double mass, const btTransform& startTransform, btCollisionShape* shape);
		void UpdateGameState(double, entity_id);
		void resetKart(entity_id id);
		void removePowerup(powerup_id_t id);
		void actOnCollision(btPersistentManifold *, phy_obj *A = NULL, phy_obj *B = NULL);

		void fireBullet(entity_id);
		void fireRocket(entity_id);

		float get_distance(Vector3 a, Vector3 b);
		void solveBulletFiring(entity_id firing_kart_id, btScalar min_angle, btScalar max_dist);
		Events::Event* makeRerportEvent(entity_id kart_shooting , entity_id kart_shot);

		void do_pulse_powerup(entity_id kart_id);

		void handle_rockets(double time);
		void sendRocketEvent(entity_id kart_hit_id, entity_id shooting_kart_id , btVector3 * hit_pos);
	};
};
