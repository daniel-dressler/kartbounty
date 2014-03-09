#include <map>

#include <BulletDynamics/Dynamics/btDiscreteDynamicsWorld.h>
#include <btBulletDynamicsCommon.h>
#include <BulletCollision/CollisionDispatch/btInternalEdgeUtility.h>
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
		void enableDebugView(); // auto called
		void step(double seconds); // Need real time class

		// after the content loader exists
		// loadWorld will take our models and
		// generate our bounding boxes
		int loadWorld();

		// after entity system exists this
		// will update the entitie's locations
		int saveoutState();

		private:
		Events::Mailbox mb;

		btDiscreteDynamicsWorld *m_world;
		//btVehicleRaycaster*	m_vehicleRayCaster[NUM_KARTS];
		//btRaycastVehicle*	m_vehicle[NUM_KARTS];
		//btRaycastVehicle::btVehicleTuning	m_tuning[NUM_KARTS];
		//btRigidBody* m_carChassis[NUM_KARTS];
		btRigidBody* m_arena;
		btAlignedObjectArray<btCollisionShape*> m_collisionShapes;

		std::map<entity_id, btRigidBody *> m_kart_bodies;
		std::map<entity_id, btRaycastVehicle *> m_karts;

		class btBroadphaseInterface* m_broadphase;
		class btCollisionDispatcher* m_dispatcher;
		class btConstraintSolver*    m_solver;
		class btDefaultCollisionConfiguration* m_collisionConfiguration;

		btRigidBody *addRigidBody(double mass, const btTransform& startTransform, btCollisionShape* shape);
		void UpdateGameState(double,int);
	};
};
