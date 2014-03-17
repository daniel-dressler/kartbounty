#include <iostream>

#include "physics.h"

using namespace Physics;

// Callback cannot be inside class
Simulation *g_physics_subsystem = NULL;
void substepCallback(btDynamicsWorld *world, btScalar timestep)
{
	g_physics_subsystem->substepEnforcer(world, timestep);
}

Simulation::Simulation()
{
	m_broadphase = new btDbvtBroadphase();
	m_collisionConfiguration = new btDefaultCollisionConfiguration();
	m_dispatcher = new btCollisionDispatcher(m_collisionConfiguration);
	m_solver = new btSequentialImpulseConstraintSolver;
	m_world = new btDiscreteDynamicsWorld(m_dispatcher, m_broadphase, m_solver, m_collisionConfiguration);
	m_world->setInternalTickCallback(substepCallback);

	g_physics_subsystem = this;

	m_world->setGravity(btVector3(0,-7,0));

	mb.request(Events::EventType::Input);
	mb.request(Events::EventType::Reset);
	mb.request(Events::EventType::KartCreated);
	mb.request(Events::EventType::KartDestroyed);
	mb.request(Events::EventType::ArenaMeshCreated);
	mb.request(Events::EventType::PowerupPlacement);
	mb.request(Events::EventType::PowerupDestroyed);

	m_arena = NULL;
	m_triangleInfoMap = NULL;
}

Simulation::~Simulation()
{
	// Delete Rigid Bodies
	for (int i = m_world->getNumCollisionObjects()-1; i >= 0; i--) {
		btCollisionObject* obj = m_world->getCollisionObjectArray()[i];
		m_world->removeCollisionObject( obj );
		delete obj;
	}
	
	// Delete Collision Shapes
	for (int j = 0; j < m_collisionShapes.size(); j++) {
		btCollisionShape* shape = m_collisionShapes[j];
		delete shape;
	}

	// Arena
	delete m_arena;

	// Karts
	for (auto kart : m_karts) {
		delete kart.second->raycaster;
		delete kart.second->vehicle;
		delete kart.second;
	}

	// Powerups
	for (auto powerup : m_powerups) {
		delete powerup.second;
	}

	// Infulstructure
	delete m_world;
	delete m_solver;
	delete m_broadphase;
	delete m_dispatcher;
	delete m_collisionConfiguration;
	delete m_triangleInfoMap;
}

void Simulation::substepEnforcer(btDynamicsWorld *world, btScalar timestep)
{
	btCollisionObjectArray objects = m_world->getCollisionObjectArray();
	for (int i = 0; i < objects.size(); i++) {
		btRigidBody *rigidBody = btRigidBody::upcast(objects[i]);
		if (!rigidBody) {
			continue;
		}

		// Clamp kart rotate to 60% of sideways
		// Prevents flips
		btTransform tr = rigidBody->getCenterOfMassTransform();
		btQuaternion rot = tr.getRotation();
		btVector3 up = rigidBody->getCenterOfMassTransform().getBasis().getColumn(1);
		up.normalize();
		btVector3 vUp = btVector3(0, 1, 0);
		btScalar angle = MAX(up.dot(vUp), 0.1);

		// Slow flips
		btScalar ang_damp = MIN(1.2 - (angle * angle * angle), 1);
		btScalar lin_damp = rigidBody->getLinearDamping();
		rigidBody->setDamping(lin_damp, ang_damp);

		// Last resort force unflip
		if (angle <= 0.2) {
			btQuaternion qUp = btQuaternion(0, 1, 0);
			rot = rot.slerp(qUp, timestep / (angle * angle));
			tr.setRotation(rot);
			rigidBody->setCenterOfMassTransform(tr);
		}
	}

	// Process Rigid Body Collisions
	int numManifolds = world->getDispatcher()->getNumManifolds();
	for (int i = 0; i < numManifolds; i++) {
		actOnCollision(world->getDispatcher()->getManifoldByIndexInternal(i));
	}

	// Process Ghost Object Collisions
	for (auto id_powerup_pair : m_powerups) {
		auto powerup = id_powerup_pair.second;
		btAlignedObjectArray<btCollisionObject*>& overlappingObjects =
													powerup->powerup_body->getOverlappingPairs();

		const int numObjects = overlappingObjects.size();
		for (int i = 0; i < numObjects; i++) {
			btCollisionObject *colObj=overlappingObjects[i];
			auto local = static_cast<phy_obj *>(colObj->getUserPointer());
			if (local != NULL) {
				actOnCollision(NULL, powerup, local);
			}
		}
	}
}

void Simulation::actOnCollision(btPersistentManifold *manifold, phy_obj *A, phy_obj *B)
{
	if (manifold != NULL) {
		const btCollisionObject* obA = static_cast<const btCollisionObject*>(manifold->getBody0());
		const btCollisionObject* obB = static_cast<const btCollisionObject*>(manifold->getBody1());
	
		A = (phy_obj *)obA->getUserPointer();
		B = (phy_obj *)obB->getUserPointer();
	}

	if (A == NULL || B == NULL) {
		return;
	}


	struct col_report report = col_report();
	report.type = NULL_TO_NULL;
	report.impact = 0;

	if (manifold != NULL) {
		int numContacts = manifold->getNumContacts();
		for (int j = 0; j < numContacts; j++) {
			btManifoldPoint& pt = manifold->getContactPoint(j);
			if (report.impact < pt.getAppliedImpulse()) {
				report.impact = pt.getAppliedImpulse();

				btVector3 pos = pt.getPositionWorldOnA();
				report.pos.x = pos.getX();
				report.pos.y = pos.getY();
				report.pos.z = pos.getZ();
			}
		}
	}

	if (report.impact > 10.0 && A->is_kart && B->is_kart) {
		auto id = MIN(A->kart_id, B->kart_id);
		auto id_alt = MAX(A->kart_id, B->kart_id);
		report.kart_id = id;
		report.kart_id_alt = id_alt;
		report.type = KART_TO_KART;

	} else if (report.impact >= 0.0 && 
			((A->is_kart && B->is_powerup) ||
			(A->is_powerup && B->is_kart))) {
		report.kart_id = A->is_kart ? A->kart_id : B->kart_id;
		report.powerup_type = A->is_powerup ? A->powerup_type : B->powerup_type;
		report.powerup_id = A->is_powerup ? A->powerup_id : B->powerup_id;
		report.type = KART_TO_POWERUP;

	} else if (report.impact > 1000.0 && 
				((A->is_kart && B->is_arena) ||
				(B->is_kart && A->is_arena)) ) { 
		report.kart_id = A->is_kart ? A->kart_id : B->kart_id;
		report.type = KART_TO_ARENA;
	}

	if (report.type != NULL_TO_NULL) {
		m_col_reports[report.kart_id] = report;
	}
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

	btRigidBody::btRigidBodyConstructionInfo cInfo((btScalar)mass,NULL,shape,localInertia);
	cInfo.m_startWorldTransform = startTransform;
	
	btRigidBody* body = new btRigidBody(cInfo);
	body->setContactProcessingThreshold(0.1);
	body->setCollisionFlags(body->getCollisionFlags() | btCollisionObject::CF_CUSTOM_MATERIAL_CALLBACK);
	
	m_world->addRigidBody(body);

	return body;
}

static bool CustomMaterialCombinerCallback(btManifoldPoint& cp,
		const btCollisionObjectWrapper* colObj0Wrap,int partId0,int index0,
		const btCollisionObjectWrapper* colObj1Wrap,int partId1,int index1)
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
	
	for ( Events::Event *event : (mb.checkMail()) )
	{
		switch ( event->type )
		{
		case Events::KartCreated:
		{
			auto kart_id = ((Events::KartCreatedEvent *)event)->kart_id;
			auto kart_local = new phy_obj();
			kart_local->is_kart = true;
			kart_local->kart_id = kart_id;
			m_karts[kart_id] = kart_local;
			
			float wheelFriction = 3;
			float suspensionStiffness = 10;
			float suspensionDamping = 0.5f;
			float suspensionCompression = 0.3f;
			// Prevents car flipping due to sharp turns
			float rollInfluence = 0.000;
			btScalar suspensionRestLength(0.15f);// Suspension Interval = rest +/- travel * 0.01
			float suspensionTravelcm = 20;
			
			btRaycastVehicle::btVehicleTuning tuning;
			tuning.m_maxSuspensionTravelCm = suspensionRestLength * 1.5;
			tuning.m_frictionSlip = 30;
			tuning.m_maxSuspensionForce = 5;
			tuning.m_suspensionCompression = suspensionCompression;
			tuning.m_suspensionDamping = suspensionDamping;
			tuning.m_suspensionStiffness = suspensionStiffness;

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
			carChassis->setUserPointer(kart_local);

			// Air resistance
			// 1 = 100% of speed lost per second
			carChassis->setDamping(0.4, 0.4);

			// Makes us bounce off walls
			carChassis->setRestitution(0.9);
			carChassis->setFriction(0.1);

			btVehicleRaycaster *vehicleRayCaster = new btDefaultVehicleRaycaster(m_world);

			auto kart = new btRaycastVehicle(tuning, m_kart_bodies[kart_id], vehicleRayCaster);
			kart->setCoordinateSystem(0,1,0);
			m_world->addVehicle(kart);
			m_karts[kart_id]->vehicle = kart;
			m_karts[kart_id]->raycaster = vehicleRayCaster;

#define CON1 (CAR_WIDTH * 1.0)
#define CON2 (CAR_LENGTH * 1.0)
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
			m_collisionShapes.push_back(arenaShape);

			phy_obj *arena = new phy_obj();
			arena->is_arena = true;
			arena->arena = addRigidBody(0.0, btTransform(btQuaternion(0,0,0,1),btVector3(0,0,0)), arenaShape);
			arena->arena->setRestitution(0.2);
			arena->arena->setUserPointer(arena);
			arena->arena->setContactProcessingThreshold(0.0);
			m_arena = arena;

			m_triangleInfoMap = new btTriangleInfoMap();
			btGenerateInternalEdgeInfo(arenaShape, m_triangleInfoMap);
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
	btRaycastVehicle *kart_body = m_karts[id]->vehicle;
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
#define STEER_MAX_ANGLE (35)
#define ENGINE_MAX_FORCE (3000)
#define BRAKE_MAX_FORCE (2500)
#define E_BRAKE_FORCE (2000)
#define MAX_SPEED (30.0)

	for ( Events::Event *event : (mb.checkMail()) )
	{
		switch ( event->type )
		{
		case Events::EventType::Input:
		{
			Events::InputEvent *input = (Events::InputEvent *)event;

			entity_id kart_id = input->kart_id;
			btRaycastVehicle *kart = m_karts.at(kart_id)->vehicle;

			Real speed = kart->getCurrentSpeedKmHour();

			Real fTurnPower = 1 - ( 2.0f / PI ) * ACOS( MAX( MIN( input->leftThumbStickRL, 1 ), -1 ) );
			fTurnPower *= fTurnPower < 0.0f ? -fTurnPower : fTurnPower;
			fTurnPower *= MIN((1.0 - (speed / MAX_SPEED)/2), 0.5);

			Real steering = DEGTORAD(STEER_MAX_ANGLE) * fTurnPower;

			Real breakingForce = input->bPressed ? E_BRAKE_FORCE : 0.0;
			if (steering > 0.4 || steering < -0.4)
				DEBUGOUT("s: %f, (): %f, ()_p: %f\n", speed, steering, fTurnPower);

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
	    	
			if( orig.y() < -1.0f  ||
				input->reset_requested)
			{
				resetKart(kart_id);
			}
	

			// Print Position?
			if (input->print_position) {
				DEBUGOUT("Pos: %f, %f, %f\n", orig.x(), orig.y(), orig.z());
			}
		}
		break;
		case Events::EventType::PowerupPlacement:
		{
			auto powerup_event = (Events::PowerupPlacementEvent *)event;
			auto powerup = new phy_obj();
			auto id = powerup_event->powerup_id;

			powerup->is_powerup = true;
			powerup->powerup_type = powerup_event->powerup_type;
			powerup->powerup_id = id;

			auto sphere = new btSphereShape(0.15);
			m_collisionShapes.push_back(sphere);

			btGhostObject *body = new btGhostObject();
			body->setCollisionShape(sphere);

			btTransform tr;
			tr.setIdentity();
			auto pos = powerup_event->pos;
			btVector3 powerup_pos = btVector3(pos.x, pos.y, pos.z);
			tr.setOrigin(powerup_pos);
			body->setWorldTransform(tr);

			body->setUserPointer(powerup);
			body->setCollisionFlags(body->getCollisionFlags() | btCollisionObject::CF_NO_CONTACT_RESPONSE);
			m_world->addCollisionObject(body,
					btBroadphaseProxy::SensorTrigger,
					btBroadphaseProxy::AllFilter & ~btBroadphaseProxy::SensorTrigger);

			m_powerups[id] = powerup;
			powerup->powerup_body = body;
		}
		break;
		case Events::EventType::PowerupDestroyed:
		{
			auto powerup_event = (Events::PowerupDestroyedEvent *)event;
			auto powerup_id = powerup_event->powerup_id;
			delete m_powerups[powerup_id];
			m_powerups.erase(powerup_id);
		}
		break;
		case Events::EventType::Reset:
		{
			Events::ResetEvent *reset_event = (Events::ResetEvent *)event;
			entity_id kart_id = reset_event->kart_id;
			resetKart(kart_id);
		}
		break;
		default:
			break;
		}
	}
	mb.emptyMail();

	// 3 means max process 16ms * 3 worth of physics
	// this thus clamps simulation at between 20FPS
	// and 60FPS.
	// If we have lower real FPS the car will
	// slow down.
	m_world->stepSimulation( (btScalar)seconds, 3, 0.0166666f );

	// Update Kart Entities
	for (auto id_kart_pair : m_karts) {
		UpdateGameState(seconds, id_kart_pair.first);
	}

	// Issue Collision Events
	std::vector<Events::Event *> events_out;
	for (auto id_report_pair : m_col_reports) {
		auto report = id_report_pair.second;

		switch (report.type) {
			case KART_TO_POWERUP:
			{
				auto event = NEWEVENT(PowerupPickup);
				event->pos = report.pos;
				event->kart_id = report.kart_id;
				event->powerup_type = report.powerup_type;
				event->powerup_id = report.powerup_id;
				events_out.push_back(event);
			}
			break;
			case KART_TO_KART:
			{
				auto event = NEWEVENT(KartColideKart);
				event->pos = report.pos;
				event->kart_id = report.kart_id;
				event->kart_id_alt = report.kart_id_alt;
				events_out.push_back(event);
			}
			break;
			case KART_TO_ARENA:
			{
				auto event = NEWEVENT(KartColideArena);
				event->pos = report.pos;
				event->kart_id = report.kart_id;
				events_out.push_back(event);
			}
			default:
			break;
		}
	}
	m_col_reports.clear();
	mb.sendMail(events_out);
}

// Updates the car placement in the world state
void Simulation::UpdateGameState(double seconds, entity_id kart_id)
{
	Entities::CarEntity *kart = GETENTITY(kart_id, CarEntity);

	// -- Kart position ------------------------
	btTransform car1 = m_karts[kart_id]->vehicle->getChassisWorldTransform();

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
	kart->forDirection = (m_karts[kart_id]->vehicle->getForwardVector()).rotate(btVector3(0,1,0),DEGTORAD(-90));

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

	Vector3 vLastofs = m_karts[kart_id]->lastofs;
	auto cameraPos = kart->camera.vPos;
	auto cameraFocus = kart->camera.vFocus;
	if( vUp.y > 0.5f )
	{
		m_karts[kart_id]->lastofs = vCamOfs;
	}
	kart->camera.vPos = Vector3::Lerp( cameraPos, vLastofs + cameraFocus, fLerpAmt );

	Real fLastSpeed = m_karts[kart_id]->lastspeed;
	Real fSpeed = ABS( m_karts[kart_id]->vehicle->getCurrentSpeedKmHour() );
	Real fAdjSpeed = Lerp( fLastSpeed, fSpeed, fLerpAmt );
	Real fAdjFOV = fAdjSpeed / (Real)MAX_SPEED;
	fAdjFOV = (Real)(Int32)( fAdjFOV * 30.0f );

	kart->camera.fFOV = Lerp( kart->camera.fFOV, 90.0f - fAdjFOV, fLerpAmt * 0.1f );


	//DEBUGOUT( "%f\n", state->Camera.fFOV );
	m_karts[kart_id]->lastspeed = fSpeed;
	kart->Speed = fSpeed;

}

void Simulation::enableDebugView()
{
}
