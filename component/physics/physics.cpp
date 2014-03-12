#include <iostream>

#include "physics.h"

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
	
	for ( Events::Event *event : (mb.checkMail()) )
	{
		switch ( event->type )
		{
		case Events::KartCreated:
		{
			auto kart_id = ((Events::KartCreatedEvent *)event)->kart_id;

			float wheelFriction = 5;
			float suspensionStiffness = 10;
			float suspensionDamping = 0.5f;
			float suspensionCompression = 0.3f;
			float rollInfluence = 0.015f; // Keep low to prevent car flipping
			btScalar suspensionRestLength(0.1f);// Suspension Interval = rest +/- travel * 0.01
			float suspensionTravelcm = 20;

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
			tr.setOrigin(btVector3(0,2,0));		// This sets where the car initially spawns
			
			btRigidBody *carChassis = addRigidBody(CAR_MASS, tr, compound);
			m_kart_bodies[kart_id] = carChassis;
			carChassis->setActivationState(DISABLE_DEACTIVATION);

			btVehicleRaycaster *vehicleRayCaster = new btDefaultVehicleRaycaster(m_world);
			btRaycastVehicle::btVehicleTuning tuning;

			auto kart = new btRaycastVehicle(tuning, m_kart_bodies[kart_id], vehicleRayCaster);
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
			kart->addWheel(connectionPointCS0,wheelDirectionCS0,
					wheelAxleCS,suspensionRestLength,wheelRadius,tuning,isFrontWheel);

			connectionPointCS0 = btVector3(-CON1,connectionHeight,CON2);
			kart->addWheel(connectionPointCS0,wheelDirectionCS0,
					wheelAxleCS,suspensionRestLength,wheelRadius,tuning,isFrontWheel);

			// Setup rear  2 wheels
			isFrontWheel = false;

			connectionPointCS0 = btVector3(-CON1,connectionHeight,-CON2);
			kart->addWheel(connectionPointCS0,wheelDirectionCS0,
					wheelAxleCS,suspensionRestLength,wheelRadius,tuning,isFrontWheel);

			connectionPointCS0 = btVector3(CON1,connectionHeight,-CON2);
			kart->addWheel(connectionPointCS0,wheelDirectionCS0,
					wheelAxleCS,suspensionRestLength,wheelRadius,tuning,isFrontWheel);	

			for (int i=0; i < kart->getNumWheels(); i++)
			{
				btWheelInfo& wheel = kart->getWheelInfo(i);
			
				wheel.m_maxSuspensionTravelCm = suspensionTravelcm;
				wheel.m_suspensionStiffness =suspensionStiffness;
				wheel.m_wheelsDampingRelaxation = suspensionDamping;
				wheel.m_wheelsDampingCompression = suspensionCompression;
				wheel.m_frictionSlip = wheelFriction;
				wheel.m_rollInfluence = rollInfluence;

			}
			for (int i=0; i < kart->getNumWheels(); i++)
			{
				//synchronize the wheels with the (interpolated) chassis worldtransform
				kart->updateWheelTransform(i,true);
			}


			Entities::CarEntity *kart_entity = GETENTITY(kart_id, CarEntity);
			btTransform car1 = kart->getChassisWorldTransform();
			btVector3 pos = car1.getOrigin();
			kart_entity->Pos.x = (Real)pos.getX();
			kart_entity->Pos.y = (Real)pos.getY();
			kart_entity->Pos.z = (Real)pos.getZ();

			btQuaternion rot = car1.getRotation();
			kart_entity->Orient.x = (Real)rot.getX();
			kart_entity->Orient.y = (Real)rot.getY();
			kart_entity->Orient.z = (Real)rot.getZ();
			kart_entity->Orient.w = (Real)-rot.getW();

			// save forward vector	
			kart_entity->forDirection = (kart->getForwardVector()).rotate(btVector3(0,1,0),DEGTORAD(-90));
			break;
		}
		case Events::EventType::ArenaMeshCreated:
		{
			btTriangleMesh *arena_mesh = ((Events::ArenaMeshCreatedEvent *)event)->arena;
			// Add map
			// Credit to http://bulletphysics.org/Bullet/phpBB3/viewtopic.php?t=6662
			// for solution to wheels bouncing off triangle edges
			gContactAddedCallback = CustomMaterialCombinerCallback;
			
			btBvhTriangleMeshShape *arenaShape = new btBvhTriangleMeshShape( arena_mesh, true, true);

			m_arena = addRigidBody(0.0, btTransform(btQuaternion(0,0,0,1),btVector3(0,0,0)), arenaShape);
			m_arena->setCollisionFlags(m_arena->getCollisionFlags() | btCollisionObject::CF_CUSTOM_MATERIAL_CALLBACK);
			btTriangleInfoMap* triangleInfoMap = new btTriangleInfoMap();
			btGenerateInternalEdgeInfo(arenaShape, triangleInfoMap);
			//delete arena_mesh;
			DEBUGOUT("Arena mesh in simulation\n");
			break;
		}
		default:
			printf("ignored event %d\n", event->type);
			break;
		}
	}

	return 0;
}

void Simulation::resetKart(entity_id id)
{
	btRaycastVehicle *kart_body = m_karts[id];
	btTransform trans;
	trans.setOrigin( btVector3( 0, 3, 0 ) );
	trans.setRotation( btQuaternion( 0, 0, 0, 1 ) );
	kart_body->getRigidBody()->setWorldTransform( trans );
	kart_body->getRigidBody()->setLinearVelocity(btVector3(0,0,0));

	auto kart_entity = GETENTITY(id, CarEntity);
	kart_entity->camera.fFOV = 0;
	kart_entity->camera.vFocus.Zero();
	kart_entity->camera.vPos.Zero();
	kart_entity->camera.orient_old.Zero();

	DEBUGOUT("Kart #%lu Reset\n", id);
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

			entity_id kart_id = input->kart_id;
			btRaycastVehicle *kart = m_karts.at(kart_id);

			Real speed = kart->getCurrentSpeedKmHour();

			Real fTurnPower = 1 - ( 2.0f / PI ) * ACOS( MAX( MIN( input->leftThumbStickRL, 1 ), -1 ) );
			fTurnPower *= fTurnPower < 0.0f ? -fTurnPower : fTurnPower;
			fTurnPower *= MIN((1.0 - (speed / MAX_SPEED)/3), 0.5);

			Real steering = DEGTORAD(STEER_MAX_ANGLE) * fTurnPower;

			Real breakingForce = input->bPressed ? E_BRAKE_FORCE : 0.0;

			// Add checking for speed to this to limit turning angle at high speeds @Kyle
			Real engineForce = ENGINE_MAX_FORCE * input->rightTrigger - BRAKE_MAX_FORCE * input->leftTrigger - speed * 2;
			
			btTransform trans = kart->getRigidBody()->getWorldTransform();
			btVector3 orig = trans.getOrigin();
			if( input->yPressed )
			{
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
	    	
			if( orig.y() < -10.0f  ||
				input->reset_requested)
			{
				resetKart(kart_id);
			}
	
			UpdateGameState(seconds, kart_id);

			// Print Position?
			if (input->print_position) {
				DEBUGOUT("Pos: %f, %f, %f\n", orig.x(), orig.y(), orig.z());
			}

			break;
		}
		case Events::EventType::Reset:
		{
			Events::ResetEvent *reset_event = (Events::ResetEvent *)event;
			entity_id kart_id = reset_event->kart_id;
			resetKart(kart_id);
			break;
		}
		default:
			break;
		}
	}
	mb.emptyMail();

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
	kart->forDirection = (m_karts[kart_id]->getForwardVector()).rotate(btVector3(0,1,0),DEGTORAD(-90));

	// Camera
	// Performed for Ai & Player
	Quaternion qOriNew = kart->Orient;
	Quaternion qOriMod = qOriNew;
	qOriMod.w = -qOriMod.w;
	Matrix matOri = Matrix::GetRotateQuaternion( qOriMod );

	Vector3 vUp = kart->Up = Vector3( 0, 1, 0 ).Transform( matOri );

	Vector3 vCamOfs = Vector3( 0, 1.0f, -1.5f ).Transform( matOri );
	vCamOfs.y = 1.0f;

	kart->camera.vFocus = kart->Pos + Vector3( 0, 0.5f, 0 );

	Real fLerpAmt = seconds * 5.0f;

	static Vector3 vLastofs = Vector3( 0, 1.0f, -1.5f );
	auto cameraPos = kart->camera.vPos;
	auto cameraFocus = kart->camera.vFocus;
	if( vUp.y > 0.5f )
	{
		vLastofs = vCamOfs;
	}
	kart->Pos = Vector3::Lerp( cameraPos, vLastofs + cameraFocus, fLerpAmt );

	static Real fLastSpeed = 0;
	Real fSpeed = ABS( m_karts[kart_id]->getCurrentSpeedKmHour() );
	Real fAdjSpeed = Lerp( fLastSpeed, fSpeed, fLerpAmt );
	Real fAdjFOV = fAdjSpeed / (Real)MAX_SPEED;
	fAdjFOV = (Real)(Int32)( fAdjFOV * 30.0f );

	kart->camera.fFOV = Lerp( kart->camera.fFOV, 90.0f - fAdjFOV, fLerpAmt * 0.1f );


	//DEBUGOUT( "%f\n", state->Camera.fFOV );
	fLastSpeed = fSpeed;
	kart->Speed = fSpeed;

}

void Simulation::enableDebugView()
{
}
