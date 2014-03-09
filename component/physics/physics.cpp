#include <iostream>

#include "physics.h"
#include "../state/state.h"

using namespace Physics;

Simulation::Simulation()
{
	m_broadphase = new btDbvtBroadphase();
	m_collisionConfiguration = new btDefaultCollisionConfiguration();
	m_dispatcher = new btCollisionDispatcher(m_collisionConfiguration);
	m_solver = new btSequentialImpulseConstraintSolver;
	m_world = new btDiscreteDynamicsWorld(m_dispatcher, m_broadphase, m_solver, m_collisionConfiguration);

	m_world->setGravity(btVector3(0,-7,0));

	mb.request(Events::EventType::Input);
	mb.request(Events::EventType::Reset);
	mb.request(Events::EventType::KartCreated);
	mb.request(Events::EventType::KartDestroyed);
	mb.request(Events::EventType::ArenaMeshCreated);
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
	for ( Events::Event *event : (mb.checkMail()) )
	{
		switch ( event->type )
		{
		case Events::EventType::KartCreated:
		{
			auto kart_id = ((Events::KartCreatedEvent *)event)->kart_id;
			int kart_index = 0;

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
			
			btRigidBody *carChassis = addRigidBody(CAR_MASS, tr, compound);
			m_kart_bodies[kart_id] = carChassis;
			carChassis->setActivationState(DISABLE_DEACTIVATION);

			m_vehicleRayCaster[kart_index] = new btDefaultVehicleRaycaster(m_world);

			auto kart = new btRaycastVehicle(m_tuning[kart_index],
										m_kart_bodies[kart_id], m_vehicleRayCaster[kart_index]);
			kart->setCoordinateSystem(0,1,0);
			m_world->addVehicle(kart);
			m_karts[kart_id] = kart;

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
			kart->addWheel(connectionPointCS0,wheelDirectionCS0,wheelAxleCS,suspensionRestLength,wheelRadius,m_tuning[kart_index],isFrontWheel);

			connectionPointCS0 = btVector3(CON1,connectionHeight,-CON2);
			kart->addWheel(connectionPointCS0,wheelDirectionCS0,wheelAxleCS,suspensionRestLength,wheelRadius,m_tuning[kart_index],isFrontWheel);	

			for (int i=0; i < kart->getNumWheels(); i++)
			{
				btWheelInfo& wheel = kart->getWheelInfo(i);
			
				wheel.m_maxSuspensionTravelCm = state->Karts[kart_index].suspensionTravelcm;
				wheel.m_suspensionStiffness = state->Karts[kart_index].suspensionStiffness;
				wheel.m_wheelsDampingRelaxation = state->Karts[kart_index].suspensionDamping;
				wheel.m_wheelsDampingCompression = state->Karts[kart_index].suspensionCompression;
				wheel.m_frictionSlip = state->Karts[kart_index].wheelFriction;
				wheel.m_rollInfluence = state->Karts[kart_index].rollInfluence;

			}
			for (int i=0; i < m_vehicle[kart_index]->getNumWheels(); i++)
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
		case Events::EventType::ArenaMeshCreated:
		{
			btTriangleMesh *arena_mesh;// = ((Events::ArenaMeshCreatedEvent *)event)->arena;
			arena_mesh = GetState().bttmArena;
			// Add map
			// Credit to http://bulletphysics.org/Bullet/phpBB3/viewtopic.php?t=6662
			// for solution to wheels bouncing off triangle edges
			gContactAddedCallback = CustomMaterialCombinerCallback;
			
			btBvhTriangleMeshShape *arenaShape = new btBvhTriangleMeshShape( GetMutState()->bttmArena, true, true);

			m_arena = addRigidBody(0.0, btTransform(btQuaternion(0,0,0,1),btVector3(0,0,0)), arenaShape);
			m_arena->setCollisionFlags(m_arena->getCollisionFlags() | btCollisionObject::CF_CUSTOM_MATERIAL_CALLBACK);
			btTriangleInfoMap* triangleInfoMap = new btTriangleInfoMap();
			btGenerateInternalEdgeInfo(arenaShape, triangleInfoMap);
			delete arena_mesh;
		}
		default:
			break;
		}
	}

	return 0;
}

void Simulation::step(double seconds)
{
#define STEER_MAX_ANGLE (25)
#define ENGINE_MAX_FORCE (2000)
#define BRAKE_MAX_FORCE (1500)
#define E_BRAKE_FORCE (200)
#define MAX_SPEED (30.0)

	for ( Events::Event *event : (mb.checkMail()) )
	{
		switch ( event->type )
		{
		case Events::EventType::Input:
		{
			Events::InputEvent *input = (Events::InputEvent *)event;

			int kart_index = input->kart_index;
			int kart_id = input->kart_id;
			auto kart = m_karts[kart_id];

			Real speed = kart->getCurrentSpeedKmHour();

			Real fTurnPower = 1 - ( 2.0f / PI ) * ACOS( MAX( MIN( input->leftThumbStickRL, 1 ), -1 ) );
			fTurnPower *= fTurnPower < 0.0f ? -fTurnPower : fTurnPower;
			fTurnPower *= MIN((1.0 - (speed / MAX_SPEED)/3), 0.5);

			Real steering = DEGTORAD(STEER_MAX_ANGLE) * fTurnPower;

			Real breakingForce = input->bPressed ? E_BRAKE_FORCE : 0;

			// unused
			//btVector3 forward = m_vehicle[kart_index]->getForwardVector();

			// Add checking for speed to this to limit turning angle at high speeds @Kyle
			Real engineForce = ENGINE_MAX_FORCE * input->rightTrigger - BRAKE_MAX_FORCE * input->leftTrigger - m_vehicle[kart_index]->getCurrentSpeedKmHour() * 2;
			//gEngineForce = ENGINE_MAX_FORCE * input->rightTrigger - BRAKE_MAX_FORCE * input->leftTrigger;
			
			if( input->yPressed )
			{
				btTransform trans = kart->getRigidBody()->getWorldTransform();
				btVector3 orig = trans.getOrigin();
				orig.setY( orig.getY() + 0.01f );
				trans.setOrigin( orig );
				trans.setRotation( btQuaternion( 0, 0, 0, 1 ) );
				kart->getRigidBody()->setWorldTransform( trans );
			}

			// Max Speed checking
			if( ABS( kart->getCurrentSpeedKmHour() ) > MAX_SPEED )
				engineForce = 0;
	
			// Apply steering to front wheels
			kart->setSteeringValue(steering, 0);
			kart->setSteeringValue(steering, 1);
	
			// Apply braking and engine force to rear wheels
			kart->applyEngineForce(engineForce, 0);
			kart->applyEngineForce(engineForce, 1);
			kart->setBrake(breakingForce, 2);
			kart->setBrake(breakingForce, 3);
	    	
			if( kart->getRigidBody()->getWorldTransform().getOrigin().y() < -10.0f )
			{
				btTransform trans;
				trans.setOrigin( btVector3( 0, 1, 0 ) );
				trans.setRotation( btQuaternion( 0, 0, 0, 1 ) );
				kart->getRigidBody()->setWorldTransform( trans );
				kart->getRigidBody()->setLinearVelocity(btVector3(0,0,0));
			}
	
			UpdateGameState(seconds, kart_id);
			break;
		}
		case Events::EventType::Reset:
		{
			Events::ResetEvent *reset_event = (Events::ResetEvent *)event;
			int kart_index = reset_event->kart_to_reset;

			btTransform trans;
			trans.setOrigin( btVector3( 0, 1, 0 ) );
			trans.setRotation( btQuaternion( 0, 0, 0, 1 ) );
			m_vehicle[kart_index]->getRigidBody()->setWorldTransform( trans );
			m_vehicle[kart_index]->getRigidBody()->setLinearVelocity(btVector3(0,0,0));
			break;
		}
		default:
			break;
		}
	}
	mb.emptyMail();



	for (int kart_index = 0; kart_index<NUM_KARTS; kart_index++)
	{
	}

	m_world->stepSimulation( (btScalar)seconds, 3, 0.0166666f );
}

// Updates the car placement in the world state
void Simulation::UpdateGameState(double seconds, int kart_id)
{
	Entities::CarEntity *kart = GETENTITY(kart_id, CarEntity);

	// -- Kart position ------------------------
	btTransform car1 = m_karts[kart_id]->getChassisWorldTransform();

	//Vector3 vPosOld = state->Karts[kart_index].vPos;
	Quaternion qOriOld = kart->Orient;
	Quaternion qOriOldInv = qOriOld;
	qOriOldInv.Invert();

	btVector3 pos = car1.getOrigin();
	kart->Pos.x = (Real)pos.getX();
	kart->Pos.y = (Real)pos.getY();
	kart->Pos.z = (Real)pos.getZ();

	btQuaternion rot = car1.getRotation();
	kart->Orient.x = (Real)rot.getX();
	kart->Orient.y = (Real)rot.getY();
	kart->Orient.z = (Real)rot.getZ();
	kart->Orient.w = (Real)-rot.getW();

	// save forward vector
	kart->ForDirection = (m_karts[kart_id]->getForwardVector()).rotate(btVector3(0,1,0),DEGTORAD(-90));

	// Camera
	// Performed for Ai & Player
	Quaternion qOriNew = kart->Orient;
	Quaternion qOriMod = qOriNew;
	qOriMod.w = -qOriMod.w;
	Matrix matOri = Matrix::GetRotateQuaternion( qOriMod );

	Vector3 vUp = kart->Up = Vector3( 0, 1, 0 ).Transform( matOri );

	Vector3 vCamOfs = Vector3( 0, 1.0f, -1.5f ).Transform( matOri );
	vCamOfs.y = 1.0f;

	kart->CameraFocus = kart->Pos + Vector3( 0, 0.5f, 0 );

	Real fLerpAmt = seconds * 5.0f;

	static Vector3 vLastofs = Vector3( 0, 1.0f, -1.5f );
	if( vUp.y > 0.5f )
	{
		kart->Pos = Vector3::Lerp( kart->CameraPos, vCamOfs + kart->CameraFocus, fLerpAmt );
		vLastofs = vCamOfs;
	}
	else
	{
		kart->Pos = Vector3::Lerp( kart->CameraPos, vLastofs + kart->CameraFocus, fLerpAmt );
	}

	static Real fLastSpeed = 0;
	Real fSpeed = ABS( m_karts[kart_id]->getCurrentSpeedKmHour() );
	Real fAdjSpeed = Lerp( fLastSpeed, fSpeed, fLerpAmt );
	Real fAdjFOV = fAdjSpeed / (Real)MAX_SPEED;
	fAdjFOV = (Real)(Int32)( fAdjFOV * 30.0f );

	kart->CameraFOV = Lerp( kart->CameraFOV, 90.0f - fAdjFOV, fLerpAmt * 0.1f );


	//DEBUGOUT( "%f\n", state->Camera.fFOV );
	fLastSpeed = fSpeed;
	kart->Speed = fSpeed;

}

void Simulation::enableDebugView()
{
}
