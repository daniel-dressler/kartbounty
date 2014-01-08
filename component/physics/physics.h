#include "../events/events.h"
#include <BulletDynamics/Dynamics/btDiscreteDynamicsWorld.h>
#include <btBulletDynamicsCommon.h>

namespace Physics {
	class Simulation {
		public:
		Simulation();
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
		btDiscreteDynamicsWorld *world;
	};
};
