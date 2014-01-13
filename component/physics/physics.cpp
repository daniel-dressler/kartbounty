#include "physics.h"
#include "GLDebugDrawer.h"

using namespace Physics;

Simulation::Simulation()
{
	// TODO: don't leak this stuff
	btBroadphaseInterface* broadphase = new btDbvtBroadphase();
	btDefaultCollisionConfiguration* collisionConfiguration = new btDefaultCollisionConfiguration();
	btCollisionDispatcher* dispatcher = new btCollisionDispatcher(collisionConfiguration);
	btSequentialImpulseConstraintSolver* solver = new btSequentialImpulseConstraintSolver;
	world = new btDiscreteDynamicsWorld(dispatcher, broadphase, solver, collisionConfiguration);
	world->setGravity(btVector3(0,-10,0));
	this->enableDebugView();
}

int Simulation::loadWorld()
{
	btCollisionShape* groundShape = new btStaticPlaneShape(btVector3(0,1,0),1);
	btCollisionShape* fallShape = new btSphereShape(-1);

	btDefaultMotionState* groundMotionState =
		new btDefaultMotionState(btTransform(btQuaternion(0,0,0,1),btVector3(0,-1,0)));
	btRigidBody::btRigidBodyConstructionInfo
		groundRigidBodyCI(0,groundMotionState,groundShape,btVector3(0,0,0));
	btRigidBody* groundRigidBody = new btRigidBody(groundRigidBodyCI);
	btDefaultMotionState* fallMotionState =
		new btDefaultMotionState(btTransform(btQuaternion(0,0,0,1),btVector3(0,50,0)));
	world->addRigidBody(groundRigidBody);

	btScalar mass = 1;
	btVector3 fallInertia(0,0,0);
	fallShape->calculateLocalInertia(mass,fallInertia);
	btRigidBody::btRigidBodyConstructionInfo fallRigidBodyCI(mass,fallMotionState,fallShape,fallInertia);
	btRigidBody* fallRigidBody = new btRigidBody(fallRigidBodyCI);
	world->addRigidBody(fallRigidBody);
	return 0;
}

void Simulation::step(double seconds)
{
	world->stepSimulation((btScalar)seconds, 10);
	world->debugDrawWorld();
}

void Simulation::enableDebugView()
{
	GLDebugDrawer *debugView = new GLDebugDrawer();
	world->setDebugDrawer(debugView);
}
