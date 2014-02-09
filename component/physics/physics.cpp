#include "physics.h"
#include "GLDebugDrawer.h"
#include "../state/state.h"
#include <iostream>

using namespace Physics;

Simulation::Simulation()
{
	m_broadphase = new btDbvtBroadphase();
	m_collisionConfiguration = new btDefaultCollisionConfiguration();
	m_dispatcher = new btCollisionDispatcher(m_collisionConfiguration);
	m_solver = new btSequentialImpulseConstraintSolver;
	m_world = new btDiscreteDynamicsWorld(m_dispatcher, m_broadphase, m_solver, m_collisionConfiguration);

	m_world->setGravity(btVector3(1,-10,0));

	mb.request(Events::EventType::Input);
}

Simulation::~Simulation()
{
	/*
	delete m_world;
	delete m_solver;
	delete m_broadphase;
	delete m_dispatcher;
	delete m_collisionConfiguration;
	*/
}

// Tuning
// Credit to:
// http://bullet.googlecode.com/svn-history/r2704/trunk/Demos/ForkLiftDemo/ForkLiftDemo.cpp
float	gEngineForce = 0.f;

float	defaultBreakingForce = 10.f;
float	gBreakingForce = 100.f;

float	maxEngineForce = 1000.f;//this should be engine/velocity dependent
float	maxBreakingForce = 100.f;

float	gVehicleSteering = 0.f;
float	steeringIncrement = 0.04f;
float	steeringClamp = 0.3f;
float	wheelRadius = 0.5f;
float	wheelWidth = 0.4f;
float	wheelFriction = 10;//BT_LARGE_FLOAT;
float	suspensionStiffness = 20.f;
float	suspensionDamping = 2.3f;
float	suspensionCompression = 4.4f;
float	rollInfluence = 0.1f;//1.0f;
btScalar suspensionRestLength(0.6);
#define CUBE_HALF_EXTENTS 1

btRigidBody *Simulation::addRigidBody(double mass, const btTransform& startTransform, btCollisionShape* shape)
{
	btVector3 localInertia(0, 0, 0);
	if (mass != 0.0) {
		shape->calculateLocalInertia(mass, localInertia);
	}
	

	btDefaultMotionState* myMotionState = new btDefaultMotionState(startTransform);
	
	btRigidBody::btRigidBodyConstructionInfo cInfo(mass,myMotionState,shape,localInertia);
	
	btRigidBody* body = new btRigidBody(cInfo);
	body->setContactProcessingThreshold(0.01);
	
	m_world->addRigidBody(body);

	return body;
}

int Simulation::loadWorld()
{
	btCollisionShape* groundShape = new btStaticPlaneShape(btVector3(0,1,0),1);
	addRigidBody(0, btTransform(btQuaternion(0,0,0,1),btVector3(0,-1,0)), groundShape);

	// Create car
	btCollisionShape* chassisShape = new btBoxShape(btVector3(0.3f,0.3f, 0.5f));
	btCompoundShape* compound = new btCompoundShape();
	m_collisionShapes.push_back(chassisShape);
	m_collisionShapes.push_back(compound);

	btTransform localTrans; // shift gravity to center of car
	localTrans.setIdentity();;
	localTrans.setOrigin(btVector3(0,1,0));
	compound->addChildShape(localTrans,chassisShape);

	btTransform tr;
	tr.setIdentity();
	tr.setOrigin(btVector3(0,10,0));
	m_carChassis = addRigidBody(800.0, tr, compound);

	m_vehicleRayCaster = new btDefaultVehicleRaycaster(m_world);
	m_vehicle = new btRaycastVehicle(m_tuning,m_carChassis, m_vehicleRayCaster);
	m_carChassis->setActivationState(DISABLE_DEACTIVATION);
	m_world->addVehicle(m_vehicle);

	float connectionHeight = 1.2f;
	bool isFrontWheel=true;
	btVector3 wheelDirectionCS0(0,-1,0);
	btVector3 wheelAxleCS(-1,0,0);

	btVector3 connectionPointCS0(CUBE_HALF_EXTENTS-(0.3*wheelWidth),connectionHeight,2*CUBE_HALF_EXTENTS-wheelRadius);

	m_vehicle->addWheel(connectionPointCS0,wheelDirectionCS0,wheelAxleCS,suspensionRestLength,wheelRadius,m_tuning,isFrontWheel);
	connectionPointCS0 = btVector3(-CUBE_HALF_EXTENTS+(0.3*wheelWidth),connectionHeight,2*CUBE_HALF_EXTENTS-wheelRadius);

	m_vehicle->addWheel(connectionPointCS0,wheelDirectionCS0,wheelAxleCS,suspensionRestLength,wheelRadius,m_tuning,isFrontWheel);
	connectionPointCS0 = btVector3(-CUBE_HALF_EXTENTS+(0.3*wheelWidth),connectionHeight,-2*CUBE_HALF_EXTENTS+wheelRadius);

	isFrontWheel = false;
	m_vehicle->addWheel(connectionPointCS0,wheelDirectionCS0,wheelAxleCS,suspensionRestLength,wheelRadius,m_tuning,isFrontWheel);

	connectionPointCS0 = btVector3(CUBE_HALF_EXTENTS-(0.3*wheelWidth),connectionHeight,-2*CUBE_HALF_EXTENTS+wheelRadius);
	m_vehicle->addWheel(connectionPointCS0,wheelDirectionCS0,wheelAxleCS,suspensionRestLength,wheelRadius,m_tuning,isFrontWheel);
	
	for (int i=0;i<m_vehicle->getNumWheels();i++)
	{
		btWheelInfo& wheel = m_vehicle->getWheelInfo(i);
		wheel.m_suspensionStiffness = suspensionStiffness;
		wheel.m_wheelsDampingRelaxation = suspensionDamping;
		wheel.m_wheelsDampingCompression = suspensionCompression;
		wheel.m_frictionSlip = wheelFriction;
		wheel.m_rollInfluence = rollInfluence;
	}


	// Add map
	StateData *state = GetMutState();
	btBvhTriangleMeshShape *arenaShape = new btBvhTriangleMeshShape(state->bttmArena, true, true);
	m_arena = addRigidBody(0.0, btTransform(btQuaternion(0,0,0,1),btVector3(0,0,0)), arenaShape);

	m_vehicle->resetSuspension();
	for (int i=0;i<m_vehicle->getNumWheels();i++)
	{
		//synchronize the wheels with the (interpolated) chassis worldtransform
		m_vehicle->updateWheelTransform(i,true);
	}

	return 0;
}

void Simulation::step(double seconds)
{
	// DEBUGOUT("RUNING PHYSICS %lf\n", seconds);
	
	
	for ( Events::Event *event : (mb.checkMail()) )
	{
		// Hack: Sorry
		Events::InputEvent *input = (Events::InputEvent *)event;
		switch ( event->type )
		{
		case Events::EventType::Input:
			gVehicleSteering = input->turn;
			DEBUGOUT("STEER %lf, ", gVehicleSteering);
			gEngineForce += -1000 * input->accelerate;
			DEBUGOUT("FORCE %lf\n", gEngineForce);
			gBreakingForce = 0.f;

			if (input->accelerate > 0.5) {
				m_carChassis->applyImpulse(btVector3(0,200,0),btVector3(0,1,1));
				DEBUGOUT("impuled");
			}
			break;
		default:
			break;
		}
	}
	mb.emptyMail();

	int wheelIndex = 2;
	m_vehicle->applyEngineForce(gEngineForce,wheelIndex);
	m_vehicle->setBrake(gBreakingForce,wheelIndex);
	wheelIndex = 3;
	m_vehicle->applyEngineForce(gEngineForce,wheelIndex);
	m_vehicle->setBrake(gBreakingForce,wheelIndex);


	wheelIndex = 0;
	m_vehicle->setSteeringValue(gVehicleSteering,wheelIndex);
	wheelIndex = 1;
	m_vehicle->setSteeringValue(gVehicleSteering,wheelIndex);

	m_world->stepSimulation((btScalar)seconds, 10);

	StateData *state = GetMutState();
	btTransform car1 = m_vehicle->getChassisWorldTransform();
	btVector3 pos = car1.getOrigin();
	state->Karts[0].vPos.x = pos.getX();
	state->Karts[0].vPos.y = pos.getY();
	state->Karts[0].vPos.z = pos.getZ();
	btQuaternion rot = car1.getRotation();
	btVector3 axis = rot.getAxis();
	Vector3 v;
	v.x = axis.getX();
	v.y = axis.getY();
	v.z = axis.getZ();
	Real angle = rot.getAngle();
	state->Karts[0].qOrient *= Quaternion();
	state->Karts[0].qOrient.RotateAxisAngle(v, angle);


}

void Simulation::enableDebugView()
{
}
