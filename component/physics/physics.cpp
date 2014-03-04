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

	m_world->setGravity(btVector3(0,-6,0));

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

#define CON1 (CAR_WIDTH)
#define CON2 (CAR_LENGTH)
	
	StateData *state = GetMutState();
	for (int kart_index=0; kart_index<NUM_KARTS; kart_index++)
	{

		state->Karts[kart_index].gVehicleSteering = 0.0f;
		state->Karts[kart_index].gEngineForce = 0.0f;
		state->Karts[kart_index].gBrakingForce = 0.0f;

		state->Karts[kart_index].wheelFriction = 5;
		state->Karts[kart_index].suspensionStiffness = 10;
		state->Karts[kart_index].suspensionDamping = 0.5f;
		state->Karts[kart_index].suspensionCompression = 0.3f;
		state->Karts[kart_index].rollInfluence = 0.015f; // Keep low to prevent car flipping
		btScalar suspensionRestLength(0.1f);// Suspension Interval = rest +/- travel * 0.01
		state->Karts[kart_index].suspensionTravelcm = 20;

		btCollisionShape* chassisShape = new btBoxShape(btVector3(CAR_WIDTH, CAR_WIDTH, CAR_LENGTH));
		btCompoundShape* compound = new btCompoundShape();
		m_collisionShapes.push_back(chassisShape);
		m_collisionShapes.push_back(compound);

		// Start of car stuff

		btTransform localTrans;
		localTrans.setIdentity();
		localTrans.setOrigin(btVector3(0,0.05f, 0));
		compound->addChildShape(localTrans, chassisShape);

		btTransform tr;
		tr.setIdentity();
		tr.setOrigin(btVector3(0,3,0));		// This sets where the car initially spawns
		
		m_carChassis[kart_index] = addRigidBody(CAR_MASS, tr, compound);

		m_vehicleRayCaster[kart_index] = new btDefaultVehicleRaycaster(m_world);

		m_vehicle[kart_index] = new btRaycastVehicle(m_tuning[kart_index],m_carChassis[kart_index], m_vehicleRayCaster[kart_index]);
		m_carChassis[kart_index]->setActivationState(DISABLE_DEACTIVATION);
		m_vehicle[kart_index]->setCoordinateSystem(0,1,0);
		m_world->addVehicle(m_vehicle[kart_index]);

		float connectionHeight = 0.10f;
		btVector3 wheelDirectionCS0(0,-1,0);
		btVector3 wheelAxleCS(-1,0,0);

		float	wheelRadius = 0.15f;
	
		// Setup front 2 wheels
		bool isFrontWheel=true;
		btVector3 connectionPointCS0(CON1,connectionHeight,CON2);
		m_vehicle[kart_index]->addWheel(connectionPointCS0,wheelDirectionCS0,wheelAxleCS,suspensionRestLength,wheelRadius,m_tuning[kart_index],isFrontWheel);

		connectionPointCS0 = btVector3(-CON1,connectionHeight,CON2);
		m_vehicle[kart_index]->addWheel(connectionPointCS0,wheelDirectionCS0,wheelAxleCS,suspensionRestLength,wheelRadius,m_tuning[kart_index],isFrontWheel);

		// Setup rear  2 wheels
		isFrontWheel = false;

		connectionPointCS0 = btVector3(-CON1,connectionHeight,-CON2);
		m_vehicle[kart_index]->addWheel(connectionPointCS0,wheelDirectionCS0,wheelAxleCS,suspensionRestLength,wheelRadius,m_tuning[kart_index],isFrontWheel);

		connectionPointCS0 = btVector3(CON1,connectionHeight,-CON2);
		m_vehicle[kart_index]->addWheel(connectionPointCS0,wheelDirectionCS0,wheelAxleCS,suspensionRestLength,wheelRadius,m_tuning[kart_index],isFrontWheel);	

		for (int i=0; i<m_vehicle[kart_index]->getNumWheels() ;i++)
		{
			btWheelInfo& wheel = m_vehicle[kart_index]->getWheelInfo(i);
		
			wheel.m_maxSuspensionTravelCm = state->Karts[kart_index].suspensionTravelcm;
			wheel.m_suspensionStiffness = state->Karts[kart_index].suspensionStiffness;
			wheel.m_wheelsDampingRelaxation = state->Karts[kart_index].suspensionDamping;
			wheel.m_wheelsDampingCompression = state->Karts[kart_index].suspensionCompression;
			wheel.m_frictionSlip = state->Karts[kart_index].wheelFriction;
			wheel.m_rollInfluence = state->Karts[kart_index].rollInfluence;

		}

		// Add map
		// Credit to http://bulletphysics.org/Bullet/phpBB3/viewtopic.php?t=6662
		// for solution to wheels bouncing off triangle edges
		gContactAddedCallback = CustomMaterialCombinerCallback;
		
		btBvhTriangleMeshShape *arenaShape = new btBvhTriangleMeshShape(state->bttmArena, true, true);

		m_arena = addRigidBody(0.0, btTransform(btQuaternion(0,0,0,1),btVector3(0,0,0)), arenaShape);
		m_arena->setCollisionFlags(m_arena->getCollisionFlags() | btCollisionObject::CF_CUSTOM_MATERIAL_CALLBACK);
		btTriangleInfoMap* triangleInfoMap = new btTriangleInfoMap();
		btGenerateInternalEdgeInfo(arenaShape, triangleInfoMap);

		for (int i=0; i<m_vehicle[kart_index]->getNumWheels() ;i++)
		{
			//synchronize the wheels with the (interpolated) chassis worldtransform
			m_vehicle[kart_index]->updateWheelTransform(i,true);
		}


		btTransform car1 = m_vehicle[kart_index]->getChassisWorldTransform();
		btVector3 pos = car1.getOrigin();
		state->Karts[kart_index].vPos.x = (Real)pos.getX();
		state->Karts[kart_index].vPos.y = (Real)pos.getY();
		state->Karts[kart_index].vPos.z = (Real)pos.getZ();

		btQuaternion rot = car1.getRotation();
		state->Karts[kart_index].qOrient.x = (Real)rot.getX();
		state->Karts[kart_index].qOrient.y = (Real)rot.getY();
		state->Karts[kart_index].qOrient.z = (Real)rot.getZ();
		state->Karts[kart_index].qOrient.w = (Real)-rot.getW();

		// save forward vector	
		state->Karts[kart_index].forDirection = (m_vehicle[kart_index]->getForwardVector()).rotate(btVector3(0,1,0),DEGTORAD(-90));
	}

	return 0;
}

void Simulation::step(double seconds)
{
#define STEER_MAX_ANGLE (25)
#define ENGINE_MAX_FORCE (2000)
#define BRAKE_MAX_FORCE (1500)
#define E_BRAKE_FORCE (200)
#define MAX_SPEED (20.0)
	StateData *state = GetMutState();

	for ( Events::Event *event : (mb.checkMail()) )
	{
		switch ( event->type )
		{
		case Events::EventType::Input:
		{
			Events::InputEvent *input = (Events::InputEvent *)event;

			int kart_index = input->kart_index;

			Real speed = m_vehicle[kart_index]->getCurrentSpeedKmHour();

			Real fTurnPower = 1 - ( 2.0f / PI ) * ACOS( MAX( MIN( input->leftThumbStickRL, 1 ), -1 ) );
			fTurnPower *= fTurnPower < 0.0f ? -fTurnPower : fTurnPower;
			fTurnPower *= MIN((1.0 - (speed / MAX_SPEED)/3), 0.5);

			state->Karts[kart_index].gVehicleSteering = DEGTORAD(STEER_MAX_ANGLE) * fTurnPower;

			state->Karts[kart_index].gBrakingForce = input->bPressed ? E_BRAKE_FORCE : 0;

			btVector3 forward = m_vehicle[kart_index]->getForwardVector();

			// Add checking for speed to this to limit turning angle at high speeds @Kyle
			state->Karts[kart_index].gEngineForce = ENGINE_MAX_FORCE * input->rightTrigger - BRAKE_MAX_FORCE * input->leftTrigger - m_vehicle[kart_index]->getCurrentSpeedKmHour() * 2;
			//gEngineForce = ENGINE_MAX_FORCE * input->rightTrigger - BRAKE_MAX_FORCE * input->leftTrigger;

			if( GetState().key_map['r'] )
			{
				btTransform trans;
				trans.setOrigin( btVector3( 0, 1, 0 ) );
				trans.setRotation( btQuaternion( 0, 0, 0, 1 ) );
				m_vehicle[kart_index]->getRigidBody()->setWorldTransform( trans );
				m_vehicle[kart_index]->getRigidBody()->setLinearVelocity(btVector3(0,0,0));
			}

			if( input->yPressed )
			{
				btTransform trans = m_vehicle[kart_index]->getRigidBody()->getWorldTransform();
				btVector3 orig = trans.getOrigin();
				orig.setY( orig.getY() + 0.01f );
				trans.setOrigin( orig );
				trans.setRotation( btQuaternion( 0, 0, 0, 1 ) );
				m_vehicle[kart_index]->getRigidBody()->setWorldTransform( trans );
			}

//			DEBUGOUT("Bforce: %lf, Eforce: %lf, Steer: %f\n", gBrakingForce, gEngineForce, gVehicleSteering);
//			DEBUGOUT("Speed: %f\n", (float)ABS( m_vehicle->getCurrentSpeedKmHour() ) );
		}
		case Events::EventType::Reset:
		{

		}
		default:
			break;
		}
	}
	mb.emptyMail();

	for (int kart_index = 0; kart_index<NUM_KARTS; kart_index++)
	{
		// Max Speed checking
		if( ABS( m_vehicle[kart_index]->getCurrentSpeedKmHour() ) > MAX_SPEED )
			state->Karts[kart_index].gEngineForce = 0;

		// Apply steering to front wheels
		m_vehicle[kart_index]->setSteeringValue(state->Karts[kart_index].gVehicleSteering, 0);
		m_vehicle[kart_index]->setSteeringValue(state->Karts[kart_index].gVehicleSteering, 1);

		// Apply braking and engine force to rear wheels
		m_vehicle[kart_index]->applyEngineForce(state->Karts[kart_index].gEngineForce, 0);
		m_vehicle[kart_index]->applyEngineForce(state->Karts[kart_index].gEngineForce, 1);
		m_vehicle[kart_index]->setBrake(state->Karts[kart_index].gBrakingForce, 2);
		m_vehicle[kart_index]->setBrake(state->Karts[kart_index].gBrakingForce, 3);

	//	m_world->stepSimulation((btScalar)seconds, 10, 0.016666f / 2.0f);
		m_world->stepSimulation( (btScalar)seconds, 2, 0.0166666f * 0.5f );
	//	m_world->stepSimulation( (btScalar)seconds, 100, 0.0166666f * 0.5f );

		if( m_vehicle[kart_index]->getRigidBody()->getWorldTransform().getOrigin().y() < -10.0f )
		{
			btTransform trans;
			trans.setOrigin( btVector3( 0, 1, 0 ) );
			trans.setRotation( btQuaternion( 0, 0, 0, 1 ) );
			m_vehicle[kart_index]->getRigidBody()->setWorldTransform( trans );
			m_vehicle[kart_index]->getRigidBody()->setLinearVelocity(btVector3(0,0,0));
		}

		UpdateGameState(seconds, kart_index);
	}
}

// Updates the car placement in the world state
void Simulation::UpdateGameState(double seconds, int kart_index)
{
	// -- Kart position ------------------------
	StateData *state = GetMutState();
	btTransform car1 = m_vehicle[kart_index]->getChassisWorldTransform();

	Vector3 vPosOld = state->Karts[kart_index].vPos;
	Quaternion qOriOld = state->Karts[kart_index].qOrient;
	Quaternion qOriOldInv = qOriOld;
	qOriOldInv.Invert();

	btVector3 pos = car1.getOrigin();
	state->Karts[kart_index].vPos.x = (Real)pos.getX();
	state->Karts[kart_index].vPos.y = (Real)pos.getY();
	state->Karts[kart_index].vPos.z = (Real)pos.getZ();

	btQuaternion rot = car1.getRotation();
	state->Karts[kart_index].qOrient.x = (Real)rot.getX();
	state->Karts[kart_index].qOrient.y = (Real)rot.getY();
	state->Karts[kart_index].qOrient.z = (Real)rot.getZ();
	state->Karts[kart_index].qOrient.w = (Real)-rot.getW();

	// save forward vector
	state->Karts[kart_index].forDirection = (m_vehicle[kart_index]->getForwardVector()).rotate(btVector3(0,1,0),DEGTORAD(-90));

	if (kart_index == PLAYER_KART)
	{
		Vector3 vPosNew = state->Karts[kart_index].vPos;
		Quaternion qOriNew = state->Karts[kart_index].qOrient;

		Vector3 vPosChange = vPosNew - vPosOld;
		Real fMoveAmt = vPosChange.Length();

		Quaternion qOriMod = qOriNew;
		qOriMod.w = -qOriMod.w;
		Matrix matOri = Matrix::GetRotateQuaternion( qOriMod );

		Vector3 vUp = Vector3( 0, 1, 0 ).Transform( matOri );

		state->Karts[kart_index].vUp = vUp;

		Vector3 vCamOfs = Vector3( 0, 1.0f, -1.5f ).Transform( matOri );
		vCamOfs.y = 1.0f;

		state->Camera.vFocus = state->Karts[kart_index].vPos + Vector3( 0, 0.5f, 0 );

		Real fLerpAmt = seconds * 5.0f;

		static Vector3 vLastofs = Vector3( 0, 1.0f, -1.5f );
		if( vUp.y > 0.5f )
		{
			state->Camera.vPos = Vector3::Lerp( state->Camera.vPos, vCamOfs + state->Camera.vFocus, fLerpAmt );
			vLastofs = vCamOfs;
		}
		else
		{
			state->Camera.vPos = Vector3::Lerp( state->Camera.vPos, vLastofs + state->Camera.vFocus, fLerpAmt );
		}

		static Real fLastSpeed = 0;
		Real fSpeed = ABS( m_vehicle[kart_index]->getCurrentSpeedKmHour() );
		Real fAdjSpeed = Lerp( fLastSpeed, fSpeed, fLerpAmt );
		Real fAdjFOV = fAdjSpeed / (Real)MAX_SPEED;
		fAdjFOV = (Real)(Int32)( fAdjFOV * 30.0f );

		state->Camera.fFOV = Lerp( state->Camera.fFOV, 90.0f - fAdjFOV, fLerpAmt * 0.1f );


		//DEBUGOUT( "%f\n", state->Camera.fFOV );
		fLastSpeed = fSpeed;
		state->Karts[kart_index].vSpeed = fSpeed;
	}
//	DEBUGOUT( "%f %f %f %f\n", fSpeed, fAdjSpeed, fLerpAmt, fAdjFOV );

}

void Simulation::enableDebugView()
{
}
