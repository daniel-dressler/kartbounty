#include "physics.h"
#include "GLDebugDrawer.h"
#include "../state/state.h"
#include <iostream>
#include <BulletCollision/CollisionDispatch/btInternalEdgeUtility.h>
#include <BulletCollision/CollisionShapes/btTriangleShape.h>
#include <btBulletDynamicsCommon.h>

using namespace Physics;

Simulation::Simulation()
{
	m_broadphase = new btDbvtBroadphase();
	m_collisionConfiguration = new btDefaultCollisionConfiguration();
	m_dispatcher = new btCollisionDispatcher(m_collisionConfiguration);
	m_solver = new btSequentialImpulseConstraintSolver;
	m_world = new btDiscreteDynamicsWorld(m_dispatcher, m_broadphase, m_solver, m_collisionConfiguration);

	m_world->setGravity(btVector3(0,-5,0));

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
// Also:
// https://code.google.com/p/bullet/source/browse/trunk/Demos/VehicleDemo/VehicleDemo.cpp

float	gVehicleSteering = 0.0f;
float	gEngineForce = 0.0f;
float	gBrakingForce = 0.0f;

float	wheelFriction = 5;
float	suspensionStiffness = 10;
float	suspensionDamping = 0.5f;
float	suspensionCompression = 0.3f;
float	rollInfluence = 0.05f; // Keep low to prevent car flipping

btScalar suspensionRestLength(0.1f);// Suspension Interval = rest +/- travel * 0.01
float	suspensionTravelcm = 20;

btRigidBody *Simulation::addRigidBody(double mass, const btTransform& startTransform, btCollisionShape* shape)
{
	btVector3 localInertia(0, 0, 0);
	if (mass != 0.0) {
		shape->calculateLocalInertia((btScalar)mass, localInertia);
	}	

	btDefaultMotionState* myMotionState = new btDefaultMotionState(startTransform);
	
	btRigidBody::btRigidBodyConstructionInfo cInfo((btScalar)mass,myMotionState,shape,localInertia);
	
	btRigidBody* body = new btRigidBody(cInfo);
	body->setContactProcessingThreshold(0.1f);
	
	m_world->addRigidBody(body);

	return body;
}

static bool CustomMaterialCombinerCallback(btManifoldPoint& cp,	const btCollisionObjectWrapper* colObj0Wrap,int partId0,int index0,const btCollisionObjectWrapper* colObj1Wrap,int partId1,int index1)
{
	btAdjustInternalEdgeContacts(cp,colObj1Wrap,colObj0Wrap, partId1,index1);
	return true;
}

extern ContactAddedCallback gContactAddedCallback;

int Simulation::loadWorld()
{
	// Create car
#define CAR_WIDTH (0.11f)
#define CAR_LENGTH (0.15f)
#define CAR_MASS (800.0f)

	btCollisionShape* chassisShape = new btBoxShape(btVector3(CAR_WIDTH, CAR_WIDTH, CAR_LENGTH));
	btCompoundShape* compound = new btCompoundShape();
	m_collisionShapes.push_back(chassisShape);
	m_collisionShapes.push_back(compound);

	btTransform localTrans;
	localTrans.setIdentity();
	localTrans.setOrigin(btVector3(0,0.05f, 0));
	compound->addChildShape(localTrans, chassisShape);

	btTransform tr;
	tr.setIdentity();
	tr.setOrigin(btVector3(0,2,0));		// This sets where the car initially spawns
	m_carChassis = addRigidBody(CAR_MASS, tr, compound);

	m_vehicleRayCaster = new btDefaultVehicleRaycaster(m_world);

	m_vehicle = new btRaycastVehicle(m_tuning,m_carChassis, m_vehicleRayCaster);
	m_carChassis->setActivationState(DISABLE_DEACTIVATION);
	m_vehicle->setCoordinateSystem(0,1,0);
	m_world->addVehicle(m_vehicle);

	float connectionHeight = 0.10f;
	btVector3 wheelDirectionCS0(0,-1,0);
	btVector3 wheelAxleCS(-1,0,0);

#define CON1 (CAR_WIDTH)
#define CON2 (CAR_LENGTH)

	float	wheelRadius = 0.15f;
	
	// Setup front 2 wheels
	bool isFrontWheel=true;
	btVector3 connectionPointCS0(CON1,connectionHeight,CON2);
	m_vehicle->addWheel(connectionPointCS0,wheelDirectionCS0,wheelAxleCS,suspensionRestLength,wheelRadius,m_tuning,isFrontWheel);

	connectionPointCS0 = btVector3(-CON1,connectionHeight,CON2);
	m_vehicle->addWheel(connectionPointCS0,wheelDirectionCS0,wheelAxleCS,suspensionRestLength,wheelRadius,m_tuning,isFrontWheel);

	// Setup rear  2 wheels
	isFrontWheel = false;

	connectionPointCS0 = btVector3(-CON1,connectionHeight,-CON2);
	m_vehicle->addWheel(connectionPointCS0,wheelDirectionCS0,wheelAxleCS,suspensionRestLength,wheelRadius,m_tuning,isFrontWheel);

	connectionPointCS0 = btVector3(CON1,connectionHeight,-CON2);
	m_vehicle->addWheel(connectionPointCS0,wheelDirectionCS0,wheelAxleCS,suspensionRestLength,wheelRadius,m_tuning,isFrontWheel);	

	for (int i=0;i<m_vehicle->getNumWheels();i++)
	{
		btWheelInfo& wheel = m_vehicle->getWheelInfo(i);
		
		wheel.m_maxSuspensionTravelCm = suspensionTravelcm;
		wheel.m_suspensionStiffness = suspensionStiffness;
		wheel.m_wheelsDampingRelaxation = suspensionDamping;
		wheel.m_wheelsDampingCompression = suspensionCompression;
		wheel.m_frictionSlip = wheelFriction;
		wheel.m_rollInfluence = rollInfluence;

		
	}

	// Add map
	// Credit to http://bulletphysics.org/Bullet/phpBB3/viewtopic.php?t=6662
	// for solution to wheels bouncing off triangle edges
	gContactAddedCallback = CustomMaterialCombinerCallback;
	StateData *state = GetMutState();
	btBvhTriangleMeshShape *arenaShape = new btBvhTriangleMeshShape(state->bttmArena, true, true);

	m_arena = addRigidBody(0.0, btTransform(btQuaternion(0,0,0,1),btVector3(0,0,0)), arenaShape);
	m_arena->setCollisionFlags(m_arena->getCollisionFlags() | btCollisionObject::CF_CUSTOM_MATERIAL_CALLBACK);
	btTriangleInfoMap* triangleInfoMap = new btTriangleInfoMap();
	btGenerateInternalEdgeInfo(arenaShape, triangleInfoMap);

	for (int i=0;i<m_vehicle->getNumWheels();i++)
	{
		//synchronize the wheels with the (interpolated) chassis worldtransform
		m_vehicle->updateWheelTransform(i,true);
	}

	return 0;
}

void Simulation::step(double seconds)
{
#define STEER_MAX_ANGLE (25)
#define ENGINE_MAX_FORCE (1500)
#define BRAKE_MAX_FORCE (1000)
#define E_BRAKE_FORCE (200)
#define MAX_SPEED (25)

	for ( Events::Event *event : (mb.checkMail()) )
	{
		switch ( event->type )
		{
		case Events::EventType::Input:
		{
			Events::InputEvent *input = (Events::InputEvent *)event;

			Real fTurnSqr = input->leftThumbStickRL * input->leftThumbStickRL;

			gVehicleSteering = DEGTORAD(STEER_MAX_ANGLE) * ( input->leftThumbStickRL < 0 ? -fTurnSqr : fTurnSqr );

			gBrakingForce = input->bPressed ? E_BRAKE_FORCE : 0;
			gEngineForce = ENGINE_MAX_FORCE * input->rightTrigger - BRAKE_MAX_FORCE * input->leftTrigger;

			if( GetState().key_map['r'] )
			{
				btTransform trans;
				trans.setOrigin( btVector3( 0, 1, 0 ) );
				trans.setRotation( btQuaternion( 0, 0, 0, 1 ) );
				m_vehicle->getRigidBody()->setWorldTransform( trans );
				m_vehicle->getRigidBody()->setLinearVelocity(btVector3(0,0,0));
			}

			if( input->yPressed )
			{
				btTransform trans = m_vehicle->getRigidBody()->getWorldTransform();
				btVector3 orig = trans.getOrigin();
				orig.setY( orig.getY() + 0.01f );
				trans.setOrigin( orig );
				trans.setRotation( btQuaternion( 0, 0, 0, 1 ) );
				m_vehicle->getRigidBody()->setWorldTransform( trans );
			}

//			DEBUGOUT("Bforce: %lf, Eforce: %lf, Steer: %f\n", gBrakingForce, gEngineForce, gVehicleSteering);
//			DEBUGOUT("Speed: %f\n", (float)ABS( m_vehicle->getCurrentSpeedKmHour() ) );
		}
		default:
			break;
		}
	}
	mb.emptyMail();

	if( ABS( m_vehicle->getCurrentSpeedKmHour() ) > MAX_SPEED )
		gEngineForce = 0;

	// Apply steering to front wheels
	m_vehicle->setSteeringValue(gVehicleSteering, 0);
	m_vehicle->setSteeringValue(gVehicleSteering, 1);

	// Apply braking and engine force to rear wheels
	m_vehicle->applyEngineForce(gEngineForce, 0);
	m_vehicle->applyEngineForce(gEngineForce, 1);
	m_vehicle->setBrake(gBrakingForce, 2);
	m_vehicle->setBrake(gBrakingForce, 3);

	m_world->stepSimulation((btScalar)seconds, 10);

	if( m_vehicle->getRigidBody()->getWorldTransform().getOrigin().y() < -1.0f )
	{
		btTransform trans;
		trans.setOrigin( btVector3( 0, 1, 0 ) );
		trans.setRotation( btQuaternion( 0, 0, 0, 1 ) );
		m_vehicle->getRigidBody()->setWorldTransform( trans );
		m_vehicle->getRigidBody()->setLinearVelocity(btVector3(0,0,0));
	}

	UpdateGameState();
}

// Updates the car placement in the world state
void Simulation::UpdateGameState()
{
	StateData *state = GetMutState();
	btTransform car1 = m_vehicle->getChassisWorldTransform();

	btVector3 pos = car1.getOrigin();
	state->Karts[0].vPos.x = (Real)pos.getX();
	state->Karts[0].vPos.y = (Real)pos.getY();
	state->Karts[0].vPos.z = (Real)pos.getZ();

	btQuaternion rot = car1.getRotation();
	state->Karts[0].qOrient.x = (Real)rot.getX();
	state->Karts[0].qOrient.y = (Real)rot.getY();
	state->Karts[0].qOrient.z = (Real)rot.getZ();
	state->Karts[0].qOrient.w = (Real)-rot.getW();
}

void Simulation::enableDebugView()
{
}
