
#include <iostream>
#include <algorithm>
#include <functional>
#include <queue>
#include <time.h>

#include "physics.h"

using namespace Physics;


// Powerup sizes - they're spheres with these radiuses:
#define FLOATING_GOLD_POWERUP_SIZE 0.4
#define GOLD_CHEST_POWERUP_SIZE 0.35
#define NORMAL_POWERUP_SIZE 0.3

// Please don't modify these if you don't have an idea what they're doing. They're pretty easy to mess up.

// How far does the bullet "see" each frame, also the speed of the bullet.
// Changing this can result in bullets flying through karts and walls that they should hit
//PLEASE DON'T CHANGE ULESS KNOW WHAT YOU'RE DOING.
#define STEP_PER_FRAME 0.6
#define ROCKET_STEP_PER_FRAME 0.6

// Theses are pretty streight forward. Just modify them to play around with the bullet mechanism!
// How long the bullet "lives" before being destroyed, even if misses.
#define BULLET_TTL 1.f 

// ROCKET STUFF
#define ROCKET_TTL 5.f
#define HALF_ROCKET_HIGHT 0.2
#define HALF_ROCKET_WIDTH 0.2

// How much the forward vector gets randomized when shooting
#define SHOOTING_RANDOMNESS 5
// How long in seconds does the player have a cooldown between shots. AI cooldwon is in enemyAI.
#define PLAYER_SHOOTING_COOLDOWN 0.1

// NOTE: Angle is measured in the dot product, not degrees
#define MIN_ANGLE_SHOOTING 0.9
#define MAX_DIST_SHOOTING 7

// Pulse constants
#define DIST_FOR_PULSE 3
#define PULSE_FORCE 5000
#define Y_OFFSET_PULSE 0.2f
#define PULSE_CAP 8000

// Array for kart spawn locations.
#define NUM_KART_SPAWN_LOCATIONS 8

//const btVector3 kartSpawnLocations[] = { btVector3(12.4, 1.1, 12.4), btVector3(-12.4, 1.1, 12.4), btVector3(12.4, 1.1, -12.4), 
//	btVector3(-12.4, 1.1,-12.4), btVector3(16,2.1,16), btVector3(-16,2.1,16), btVector3(-16,2.1,-16), btVector3(16,2.1,-16) };

const btVector3 kartSpawnLocations[] = { btVector3(17, 2.10, 8), btVector3(-17, 2.1, 8), btVector3(17, 2.1, -8), 
	btVector3(-17, 2.1, -8), btVector3(8, 2.1, -17), btVector3(-8, 2.1, 17), btVector3(-8, 2.1, -17), btVector3(8, 2.1, 17) };

int kartSpawnCounter;

btVector3 toBtVector(Vector3 *in)
{
	return btVector3(in->x, in->y, in->z);
}

Vector3 fromBtVector(btVector3 *in)
{
	return Vector3(in->getX(), in->getY(), in->getZ());
}

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

	m_world->setGravity(btVector3(0,-8,0));

	mb.request(Events::EventType::Input);
	mb.request(Events::EventType::Reset);
	mb.request(Events::EventType::KartCreated);
	mb.request(Events::EventType::KartDestroyed);
	mb.request(Events::EventType::ArenaMeshCreated);
	mb.request(Events::EventType::PowerupPlacement);
	mb.request(Events::EventType::PowerupDestroyed);
	mb.request(Events::EventType::PowerupActivated);
	mb.request(Events::EventType::Shoot);
	mb.request(Events::EventType::TogglePauseGame);
	mb.request(Events::EventType::RoundEnd);

	m_arena = NULL;
	m_triangleInfoMap = NULL;

	gamePaused = false;

	// Set Initial seed for spawn locations
	srand(time(NULL));
	kartSpawnCounter = rand() % NUM_KART_SPAWN_LOCATIONS;
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
	for (int j = 0; j < m_collisionShapes.size(); j++) 
	{
		btCollisionShape* shape = m_collisionShapes[j];
		delete shape;
	}

	// Arena
	delete m_arena;

	// Karts
	for (auto kart : m_karts) 
	{
		delete kart.second->raycaster;
		delete kart.second->vehicle;
		delete kart.second;
	}

	// Powerups
	for (auto powerup : m_powerups) 
	{
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
		if (!rigidBody) 
		{
			continue;
		}

		//// Clamp kart rotate to 60% of sideways
		//// Prevents flips
		//btTransform tr = rigidBody->getCenterOfMassTransform();
		//btQuaternion rot = tr.getRotation();
		//btVector3 up = rigidBody->getCenterOfMassTransform().getBasis().getColumn(1);
		//up.normalize();
		//btVector3 vUp = btVector3(0, 1, 0);
		//btScalar angle = MAX(up.dot(vUp), 0.1);

		//// Slow flips on way up
		//btScalar ang_damp = MIN(1.2 - (angle * angle * angle), 1);
		//btScalar lin_damp = rigidBody->getLinearDamping();
		//btVector3 ang_vol = rigidBody->getAngularVelocity();
		//if (ang_vol.getY() < 0)
		//	ang_damp = 0.1;
		//rigidBody->setDamping(lin_damp, ang_damp);

		//// Last resort force unflip
		//if (angle <= 0.2) {
		//	btQuaternion qUp = btQuaternion(0, 1, 0);
		//	rot = rot.slerp(qUp, timestep / (angle * angle));
		//	tr.setRotation(rot);
		//	rigidBody->setCenterOfMassTransform(tr);
		//}
	}

	// Process Rigid Body Collisions
	int numManifolds = world->getDispatcher()->getNumManifolds();
	for (int i = 0; i < numManifolds; i++) 
	{
		actOnCollision(world->getDispatcher()->getManifoldByIndexInternal(i));
	}

	// Process Ghost Object Collisions
	for (auto id_powerup_pair : m_powerups)
	{
		auto powerup = id_powerup_pair.second;
		btAlignedObjectArray<btCollisionObject*>& overlappingObjects =
													powerup->powerup_body->getOverlappingPairs();

		const int numObjects = overlappingObjects.size();
		for (int i = 0; i < numObjects; i++) 
		{
			btCollisionObject *colObj=overlappingObjects[i];
			auto local = static_cast<phy_obj *>(colObj->getUserPointer());
			if (local != NULL) 
			{
				actOnCollision(NULL, powerup, local);
			}
		}
	}
}


void Simulation::actOnCollision(btPersistentManifold *manifold, phy_obj *A, phy_obj *B)
{
	if (manifold != NULL) 
	{
		const btCollisionObject* obA = static_cast<const btCollisionObject*>(manifold->getBody0());
		const btCollisionObject* obB = static_cast<const btCollisionObject*>(manifold->getBody1());
	
		A = (phy_obj *)obA->getUserPointer();
		B = (phy_obj *)obB->getUserPointer();
	}

	if (A == NULL || B == NULL) {
		return;
	}

	if ( (A->is_kart && A->is_arena) || (B->is_kart && B->is_arena))
			return ;
	// ^ HACK TO FIX BUG. For some reason, at times a phy_obj comes here with all 3 fileds is_kart, is_arena and is_powerup as set to true.
	// Both me and kyle would get these happen every once in a while - no idea what is the trigger for them, couldn't find what was causing it.

	if (A->is_kart)
	{
		auto kart_entity = GETENTITY(A->kart_id, CarEntity);
		if (kart_entity->isExploding)
			return;
	}

	if (B->is_kart)
	{
		auto kart_entity = GETENTITY(B->kart_id, CarEntity);
		if (kart_entity->isExploding)
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

				btVector3 pos = A->is_kart ? pt.getPositionWorldOnA() :
				                             pt.getPositionWorldOnB();
				report.pos.x = pos.getX();
				report.pos.y = pos.getY();
				report.pos.z = pos.getZ();
			}
		}
	}
	if (A->is_powerup) {
		report.pos = A->powerup_pos;
	} else if (B->is_powerup) {
		report.pos = B->powerup_pos;
	}

	if (report.impact > 10.0 && A->is_kart && B->is_kart) 
	{
		auto id = MIN(A->kart_id, B->kart_id);
		auto id_alt = MAX(A->kart_id, B->kart_id);
		report.kart_id = id;
		report.kart_id_alt = id_alt;
		report.type = KART_TO_KART;

	} else if (report.impact >= 0.0 && 
			((A->is_kart && B->is_powerup ) ||
			(A->is_powerup && B->is_kart ))) 
	{
		report.kart_id = A->is_kart ? A->kart_id : B->kart_id;
		report.powerup_type = A->is_powerup ? A->powerup_type : B->powerup_type;
		report.powerup_id = A->is_powerup ? A->powerup_id : B->powerup_id;
		report.floating_index =  A->is_powerup ? A->floating_gold : B->floating_gold;
		report.type = KART_TO_POWERUP;

	} else if (report.impact > 1000.0 && 
				((A->is_kart && B->is_arena) ||
				(B->is_kart && A->is_arena)) ) 
	{ 
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
	partId0 = partId0;
	index0 = index0;
	btAdjustInternalEdgeContacts(cp,colObj1Wrap,colObj0Wrap, partId1,index1);
	return true;
}

extern ContactAddedCallback gContactAddedCallback;

#define CAR_WIDTH (0.11f)
#define CAR_LENGTH (0.16f)
#define CAR_HEIGHT (0.11f)
#define CAR_MASS (800.0f)

#define CON1 (CAR_WIDTH - 0.02)
#define CON2 (CAR_LENGTH - 0.05)

#define LINEAR_DAMPING 0.4
#define ANGULAR_DAMPING 0.4

int Simulation::createKart(entity_id kart_id)
{
	auto kart_local = new phy_obj();
	kart_local->is_kart = true;
	kart_local->kart_id = kart_id;
	m_karts[kart_id] = kart_local;

	//float wheelFriction = 5;
	float wheelFriction = 15;
	float suspensionStiffness = 14;
	float suspensionCompression = 0.1 * 2.0 * btSqrt(suspensionStiffness);
	float suspensionDamping = suspensionCompression + 0.2;

	// Prevents car flipping due to sharp turns
	float rollInfluence = 0.000f;
	btScalar suspensionRestLength(0.1f);  // Suspension Interval = rest +/- travel * 0.01
	float suspensionTravelcm = 10;

	btRaycastVehicle::btVehicleTuning tuning;
	btCollisionShape* chassisShape = new btBoxShape(btVector3(CAR_WIDTH, CAR_HEIGHT, CAR_LENGTH));
	btCompoundShape* compound = new btCompoundShape();
	//m_collisionShapes.push_back(chassisShape);
	//m_collisionShapes.push_back(compound);

	// Start of car stuff
	btTransform localTrans;
	localTrans.setIdentity();
	localTrans.setOrigin(btVector3(0,0.05f,0));
	compound->addChildShape(localTrans, chassisShape);

	btTransform tr;
	tr.setIdentity();
	//tr.setOrigin(btVector3(0,1.05,0));		
	
	// This sets where the car initially spawns with a rotation to look towards the center of the map
	tr.setOrigin(kartSpawnLocations[kartSpawnCounter % NUM_KART_SPAWN_LOCATIONS]);
	btScalar distX = kartSpawnLocations[kartSpawnCounter % NUM_KART_SPAWN_LOCATIONS].x() * -1;
	btScalar distY = kartSpawnLocations[kartSpawnCounter % NUM_KART_SPAWN_LOCATIONS].z() * -1;
	btScalar radians = atan2(distX, distY);
	btQuaternion rotation = btQuaternion(btVector3(0,1,0), radians);
	tr.setRotation(rotation);

	btRigidBody *carChassis = addRigidBody(CAR_MASS, tr, compound);
	m_kart_bodies[kart_id] = carChassis;
	carChassis->setActivationState(DISABLE_DEACTIVATION);
	carChassis->setUserPointer(kart_local);

	// Air resistance
	// 1 = 100% of speed lost per second
	carChassis->setDamping(LINEAR_DAMPING, ANGULAR_DAMPING);

	// Makes us bounce off walls
	carChassis->setRestitution(0.9);
	carChassis->setFriction(0.1);

	btVehicleRaycaster *vehicleRayCaster = new btDefaultVehicleRaycaster(m_world);

	auto kart = new btRaycastVehicle(tuning, m_kart_bodies[kart_id], vehicleRayCaster);
	kart->getRigidBody()->setMotionState(new btDefaultMotionState(tr));

	kart->setCoordinateSystem(0,1,0);
	m_world->addVehicle(kart);
	m_karts[kart_id]->vehicle = kart;
	m_karts[kart_id]->raycaster = vehicleRayCaster;

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

	auto car1_ms = m_karts[kart_id]->vehicle->getRigidBody()->getMotionState();
	btTransform car1;
	car1_ms->getWorldTransform(car1);

	btVector3 pos = car1.getOrigin();
	kart_entity->Pos.x = (Real)pos.getX();
	kart_entity->Pos.y = (Real)pos.getY();
	kart_entity->Pos.z = (Real)pos.getZ();

	// Keep track of this so kart resets back to its initial spawn location
	kart_entity->respawnLocation = kartSpawnLocations[kartSpawnCounter % NUM_KART_SPAWN_LOCATIONS];
	kartSpawnCounter++;

	btQuaternion rot = car1.getRotation();
	kart_entity->Orient.x = (Real)rot.getX();
	kart_entity->Orient.y = (Real)rot.getY();
	kart_entity->Orient.z = (Real)rot.getZ();
	kart_entity->Orient.w = (Real)-rot.getW();

	//Set initial camera value
	kart_entity->camera.fFOV = 70;
	kart_entity->camera.vFocus.Zero();
	kart_entity->camera.vPos.Zero();
	kart_entity->camera.orient_old.Zero();

	// save forward vector	
	kart_entity->Up = Vector3(0,1,0);

	kart_entity->forDirection = (kart->getForwardVector()).rotate(btVector3(0,1,0),DEGTORAD(-90));

	return 1;
}

int Simulation::loadWorld()
{
	// Create car	
	for ( Events::Event *event : (mb.checkMail()) )
	{
		switch ( event->type )
		{
		case Events::KartCreated:
			{
				auto kart_id = ((Events::KartCreatedEvent *)event)->kart_id;
				createKart(kart_id);
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
				//m_collisionShapes.push_back(arenaShape);

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
	auto kart_entity = GETENTITY(id, CarEntity);

	// Make sure the id is in the map incase we got an old reset event
	if(m_karts.find(id) == m_karts.end())
		return;

	btRaycastVehicle *kart_body = m_karts[id]->vehicle;
	btTransform trans;
	//trans.setOrigin( btVector3( 0, 3, 0 ) );
	trans.setOrigin( kart_entity->respawnLocation + btVector3(0,2,0) );
	btScalar distX = kart_entity->respawnLocation.x() * -1;
	btScalar distY = kart_entity->respawnLocation.y() * -1;
	btScalar radians = atan2(distX, distY);
	btQuaternion rotation = btQuaternion(btVector3(0,1,0), radians);
	trans.setRotation(rotation);

	//trans.setRotation( btQuaternion( 0, 0, 0, 1 ) );
	kart_body->getRigidBody()->setWorldTransform( trans );
	kart_body->getRigidBody()->setLinearVelocity(btVector3(0,0,0));

	
	kart_entity->camera.fFOV = 70;
	kart_entity->camera.vFocus.Zero();
	kart_entity->camera.vPos.Zero();
	kart_entity->camera.orient_old.Zero();

	//DEBUGOUT("Kart #%d Reset\n", id);
}

void Simulation::removePowerup(powerup_id_t id)
{
	auto powerup = m_powerups[id];
	m_world->removeCollisionObject(powerup->powerup_body);
	delete m_powerups[id];
	m_powerups.erase(id);
}

void Simulation::handle_bullets(double time)
{
	std::vector<Events::Event *> events_out;

	auto bullet_iter = list_of_bullets.begin();

	while (bullet_iter != list_of_bullets.end())
	{		
		auto bullet_obj = bullet_iter->second;
		auto pos = &bullet_obj->position;
		auto oldPos = *pos;

		*pos += bullet_obj->direction;
			
		// Reduce ttl
		bullet_obj->time_to_live -= time;

		btCollisionWorld::ClosestRayResultCallback RayCallback(oldPos, *pos);
		m_world->rayTest(oldPos, *pos, RayCallback);
			

		// if bullet hit somethings, check what
		if (RayCallback.hasHit())
		{	
			auto user_obj = (phy_obj *)RayCallback.m_collisionObject->getUserPointer();

			bool hit_ground = user_obj->is_arena;
			bool hit_kart = user_obj->is_kart;

			// If it was a kart, proceed
			if (hit_kart)
			{
				// The kart being hit
				entity_id kart_id= user_obj->kart_id;
				auto kart_entity = GETENTITY( kart_id , CarEntity );

				// Find which kart was hit. If it was the shooting kart, ignore
				if (kart_id != bullet_obj->kart_id && !kart_entity->isExploding) 
				{
					auto hit_event = NEWEVENT(KartHitByBullet);
					hit_event->kart_id = kart_id;
					hit_event->source_kart_id = bullet_obj->kart_id;
					events_out.push_back(hit_event);

					auto id = bullet_obj->bullet_id;
					delete list_of_bullets[id];
					++bullet_iter;
					list_of_bullets.erase(id);
				}
			}
			else if (hit_ground)
			{
				auto id = bullet_obj->bullet_id;
				delete list_of_bullets[id];
				++bullet_iter;
				list_of_bullets.erase(id);
			}
		} 

		else if (bullet_obj->time_to_live < 0) 
		{
			// Kill bullet on TTL < 0
			auto id = bullet_obj->bullet_id;
			delete list_of_bullets[id];
			++bullet_iter;
			list_of_bullets.erase(id);
		} 

		else 
		{
			++bullet_iter;
		}
	}

	// Send out hit events
	mb.sendMail(events_out);
}

void Simulation::handle_rockets(double time)
{
	// go over all the rockets
	for(auto rocket_pair : list_of_rockets)
	{		
		auto rocket_obj = rocket_pair.second;
		auto to_move = (rocket_obj->direction * ROCKET_STEP_PER_FRAME);

		auto oldPos = rocket_obj->position;
		rocket_obj->position += to_move ;
		auto pos = rocket_obj->position;

		// Reduce ttl
		rocket_obj->time_to_live -= time;

		btCollisionWorld::ClosestRayResultCallback RayCallback(oldPos, pos);
		m_world->rayTest(oldPos, pos, RayCallback);
			
		// if rocket hit somethings, check what
		if (RayCallback.hasHit())
		{	
			auto user_obj = (phy_obj *)RayCallback.m_collisionObject->getUserPointer();

			bool hit_ground = user_obj->is_arena;
			bool hit_kart = user_obj->is_kart;

			// If it was a kart, proceed
			if (hit_kart)
			{
				// The kart being hit
				entity_id kart_id= user_obj->kart_id;
				auto kart_entity = GETENTITY( kart_id , CarEntity );

				// Find which kart was hit. If it was the shooting kart, ignore
				if (kart_id != rocket_obj->kart_id && !kart_entity->isExploding) 
				{
					// Rocket hit a kart
					sendRocketEvent(kart_id, rocket_obj->kart_id, &rocket_obj->position);

					// Remove rocket
					rockets_to_remove.push_back(rocket_obj->rocket_id);	
					break;
				}
			}
			else if ( hit_ground )
			{
				// Rocket hit a ground
				sendRocketEvent(-1, rocket_obj->kart_id, &rocket_obj->position);

				// Remove rocket
				rockets_to_remove.push_back(rocket_obj->rocket_id);	
				break;
			}
		} 
		else if ( rocket_obj->time_to_live < 0 )
		{
			// Rocket hit a ground
			sendRocketEvent(-2, rocket_obj->kart_id, &rocket_obj->position);

			// Remove rocket
			rockets_to_remove.push_back(rocket_obj->rocket_id);	
			break;
		}
	}

	for (auto id : rockets_to_remove)
	{
		// Remove rocket
		delete list_of_rockets[id];
		list_of_rockets.erase(id);
	}

	rockets_to_remove.clear();
}

void Simulation::sendRocketEvent(entity_id kart_hit_id, entity_id shooting_kart_id , btVector3 * hit_pos)
{
	// Send out hit events
	std::vector<Events::Event *> events_out;

	auto hit_event = NEWEVENT(RocketHit);

	hit_event->shooting_kart_id = shooting_kart_id; // if -1 hit ground, if TTL expired
	hit_event->kart_hit_id = kart_hit_id;
	hit_event->hit_pos = *hit_pos;

	//DEBUGOUT("Rocket shot by %d has hit %d\n", shooting_kart_id, kart_hit_id)

	// Send event
	events_out.push_back(hit_event);
	mb.sendMail(events_out);
}

void Simulation::step(double seconds)
{
//#define STEER_MAX_ANGLE (35)
#define STEER_MAX_ANGLE (17)
#define ENGINE_MAX_FORCE (3000)
#define BOOST_FACTOR (2)
#define BRAKE_MAX_FORCE (2500)
#define E_BRAKE_FORCE (300)
#define MAX_SPEED (30.0)


	// So that bullets don't collide with the kart that shoots
	handle_bullets(seconds);
	handle_rockets(seconds);

	// Vector to hold out going mail events
	std::vector<Events::Event *> events_out;

	for ( Events::Event *event : (mb.checkMail()) )
	{
		switch ( event->type )
		{
		case Events::KartCreated:
		{
			auto kart_id = ((Events::KartCreatedEvent *)event)->kart_id;
			createKart(kart_id);
			break;
		}
		case Events::Shoot:
		{
			auto kart_id = ((Events::ShootEvent *)event)->kart_id;
			fireBullet(kart_id);
		}
		break;
		case Events::EventType::Input:
		{			
			Events::InputEvent *input = (Events::InputEvent *)event;
			entity_id kart_id = input->kart_id;

			Entities::CarEntity *kart_ent = GETENTITY(kart_id, CarEntity);
			btRaycastVehicle *kart = m_karts.at(kart_id)->vehicle;

			preventKartFlipping(kart, &toBtVector(&(kart_ent->Up)));

			// Check if kart is currently blowing up and if so give it empty input
			if(kart_ent->isExploding)
			{
				kart->setSteeringValue(0, 0);
				kart->setSteeringValue(0, 1);
				kart->applyEngineForce(0, 0);
				kart->applyEngineForce(0, 1);
				
				break;
			}

			//DEBUGOUT("id: %d, r: %f, a: %d\n", kart_id, input->rightTrigger, input->aPressed);
			if (m_karts.count(kart_id) == 0)
			{
				DEBUGOUT("Warning: Invalid kart_id on input event\n");
				break;
			}			

			Real speed = kart->getCurrentSpeedKmHour();

			Real fTurnPower = 1 - ( 2.0f / PI ) * ACOS( MAX( MIN( input->leftThumbStickRL, 1 ), -1 ) );
			fTurnPower *= fTurnPower < 0.0f ? -fTurnPower : fTurnPower;
			fTurnPower *= MIN((1.0 - (speed / (Real)MAX_SPEED)/2), 0.5);

			Real steering = DEGTORAD(STEER_MAX_ANGLE) * fTurnPower;

			//if (steering > 0.4 || steering < -0.4)
			//	DEBUGOUT("s: %f, (): %f, ()_p: %f\n", speed, steering, fTurnPower);

			Real engineForce = ENGINE_MAX_FORCE * input->rightTrigger - BRAKE_MAX_FORCE * input->leftTrigger - speed * 2;
		
			auto car1 = kart->getRigidBody()->getMotionState();
			btTransform trans;
			car1->getWorldTransform(trans);

			btVector3 orig = trans.getOrigin();
			if( input->yPressed )
			{
				orig.setY( orig.getY() + 0.01f );
				trans.setOrigin( orig );
				trans.setRotation( btQuaternion( 0, 0, 0, 1 ) );
				kart->getRigidBody()->setWorldTransform( trans );
			}

			Real breakingForce = 0.0;

			// Check if kart is grounded for handbrake event
			if( input->bPressed )
			{
				btVector3 downRay = orig - btVector3(0,20,0);
				btCollisionWorld::ClosestRayResultCallback RayCallback(orig, downRay);

				m_world->rayTest(orig, downRay, RayCallback);

				if(RayCallback.hasHit())
				{
					btVector3 hitEnd = RayCallback.m_hitPointWorld;	// Point in world coord where ray hit
					btScalar height = orig.getY() - hitEnd.getY();	// Height kart is off ground
				
					if(height < 0.1f)
					{
						breakingForce = E_BRAKE_FORCE;
						auto event = NEWEVENT(KartHandbrake);
						event->kart_id = kart_id;
						event->speed = speed;
						event->pos = fromBtVector(&orig);
						events_out.push_back(event);
					}
				}
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

			kart_ent->shoot_timer -= seconds; // Every turn reduce the cooldown remained before can shoot again

			// Generate bullets for player shots
			if (input->aPressed && !gamePaused && kart_ent->shoot_timer <= 0) {
				kart_ent->shoot_timer = PLAYER_SHOOTING_COOLDOWN;
				fireBullet(kart_id);
			}

			solveBulletFiring(kart_id, MIN_ANGLE_SHOOTING, MAX_DIST_SHOOTING);
		}
		break;
		case Events::EventType::KartDestroyed:
		{
			auto kart_event = (Events::KartDestroyedEvent *)event;
			auto id = kart_event->kart_id;
			auto kart = m_karts[id];
			m_world->removeCollisionObject(kart->vehicle->getRigidBody());

			delete m_karts[id];
			m_karts.erase(id);
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
			powerup->floating_gold = powerup_event->floating_index;
			
			btSphereShape * sphere;
			// Made powerups a bit bigger
			if (powerup->powerup_type == Entities::FloatingGoldPowerup)
				sphere = new btSphereShape( FLOATING_GOLD_POWERUP_SIZE );
			else if (powerup->powerup_type == Entities::FloatingGoldPowerup)
				sphere = new btSphereShape( GOLD_CHEST_POWERUP_SIZE );
			else
				sphere = new btSphereShape( NORMAL_POWERUP_SIZE );
			//m_collisionShapes.push_back(sphere);

			btGhostObject *body = new btGhostObject();
			body->setCollisionShape(sphere);

			btTransform tr;
			tr.setIdentity();
			auto pos = powerup->powerup_pos = powerup_event->pos;
			btVector3 powerup_pos = toBtVector(&pos);
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
			removePowerup(powerup_event->powerup_id);
		}
		break;
		case Events::EventType::PowerupActivated:
		{
			Events::PowerupActivatedEvent *powUsed = (Events::PowerupActivatedEvent *)event;
			switch (powUsed->powerup_type)
			{
			case Entities::SpeedPowerup:
				{
					btRaycastVehicle *kart = m_karts[powUsed->kart_id]->vehicle;
					if(kart != NULL)
					{
						auto kart_entity = GETENTITY(powUsed->kart_id, CarEntity);
						if(kart_entity != NULL)
						{
							btRigidBody *kartBody = kart->getRigidBody();
							btScalar boostForce = ENGINE_MAX_FORCE * BOOST_FACTOR;
							btVector3 boost = kart_entity->forDirection * boostForce;

							kartBody->applyImpulse(boost, btVector3(0,0,0));
						}
					}
				}
				break;
				case Entities::PulsePowerup:
				{
					do_pulse_powerup(powUsed->kart_id);
				}
				break;
				case Entities::RocketPowerup:
				{
					fireRocket(powUsed->kart_id);
				}
				break;
			default:
				break;
			}
		}
		break;
		case Events::EventType::Reset:
		{
			Events::ResetEvent *reset_event = (Events::ResetEvent *)event;
			entity_id kart_id = reset_event->kart_id;
			resetKart(kart_id);
		}
		break;
		case Events::EventType::TogglePauseGame:
		{
			gamePaused = !gamePaused;
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
	if(!gamePaused)
	{
		m_world->stepSimulation( (btScalar)seconds, 10, 0.0166666f );
	}

	// Update Kart Entities
	for (auto id_kart_pair : m_karts) 
	{
		UpdateGameState(seconds, id_kart_pair.first);
	}

	// Issue Collision Events
	std::map<powerup_id_t, bool> takenPowerups;
	for (auto id_report_pair : m_col_reports) 
	{
		auto report = id_report_pair.second;

		switch (report.type) {
			case KART_TO_POWERUP:
			{
				if (takenPowerups[report.powerup_id])
					break;

				auto event = NEWEVENT(PowerupPickup);
				event->pos = report.pos;
				event->kart_id = report.kart_id;
				event->powerup_type = report.powerup_type;
				event->powerup_id = report.powerup_id;
				event->floating_index = report.floating_index;
				events_out.push_back(event);

				takenPowerups[report.powerup_id] = true;
				removePowerup(report.powerup_id);
			}
			break;
			case KART_TO_KART:
			{
				auto event = NEWEVENT(KartColideKart);
				event->pos = report.pos;
				event->kart_id = report.kart_id;
				event->kart_id_alt = report.kart_id_alt;
				event->force = report.impact;
				events_out.push_back(event);
			}
			break;
			case KART_TO_ARENA:
			{
				auto event = NEWEVENT(KartColideArena);
				event->pos = report.pos;
				event->kart_id = report.kart_id;
				event->force = report.impact;
				events_out.push_back(event);
			}
			default:
			break;
		}
	}
	m_col_reports.clear();

	// Send the bullets list pointer to rendering. 
	auto bullet_list_event = NEWEVENT(BulletList);
	bullet_list_event->list_of_bullets = &list_of_bullets;
	bullet_list_event->list_of_rockets = &list_of_rockets;
	events_out.push_back(bullet_list_event);

	mb.sendMail(events_out);

	//DEBUGOUT("Powerups: %d Kart Bodies: %d\n CollisionShapes: %d", m_powerups.size(), m_kart_bodies.size(), m_collisionShapes.size());
}

// Updates the car placement in the world state
void Simulation::UpdateGameState(double seconds, entity_id kart_id)
{
	Entities::CarEntity *kart = GETENTITY(kart_id, CarEntity);

	// -- Kart position ------------------------
	auto car1 = m_karts[kart_id]->vehicle->getRigidBody()->getMotionState();
	btTransform trans;
	car1->getWorldTransform(trans);

	//Vector3 vPosOld = state->Karts[kart_index].vPos;
	Quaternion qOriOld = kart->Orient;
	Quaternion qOriOldInv = qOriOld;
	qOriOldInv.Invert();

	btVector3 pos = trans.getOrigin();
	kart->Pos = fromBtVector(&pos);

	btQuaternion rot = trans.getRotation();
	kart->Orient.x = (Real)rot.getX();
	kart->Orient.y = (Real)rot.getY();
	kart->Orient.z = (Real)rot.getZ();
	kart->Orient.w = (Real)-rot.getW();

	// Get tire locations
	for( Int32 i = 0; i < 4; i++ )
	{
		btQuaternion kart_ori = trans.getRotation();
		Quaternion q = Quaternion(kart_ori.getX(), kart_ori.getY(), kart_ori.getZ(), kart_ori.getW());

		Matrix matOri = Matrix::GetRotateQuaternion( q );
		Vector3 x_axis = Vector3( 1, 0, 0 ).Transform( matOri );
		Vector3 z_axis = Vector3( 0, 0, 1 ).Transform( matOri );

		//CAR_LENGTH
		//CAR_WIDTH

		Vector3 offset_x = CON1 * x_axis;
		Vector3 offset_y;// = -0.02 * y_axis; <- Wheels render through floor if not 0.
		offset_y.Zero();
		Vector3 offset_z = CON2 * z_axis;

		switch (i)
		{
			// Front wheels
			case 0:
				kart->tirePos[i] = kart->Pos - offset_x + offset_y + offset_z;
				break;
			case 1:
				kart->tirePos[i] = kart->Pos + offset_x + offset_y + offset_z;
				break;

			// Back wheels	
			case 2:
				kart->tirePos[i] = kart->Pos - offset_x + offset_y - offset_z;
				break;
			case 3:
				kart->tirePos[i] = kart->Pos + offset_x + offset_y - offset_z;
				break;
		}
		
		auto wtrans = m_karts[kart_id]->vehicle->getWheelTransformWS( i );
		btQuaternion rot = wtrans.getRotation();
		kart->tireOrient[i].x = (Real)rot.getX();
		kart->tireOrient[i].y = (Real)rot.getY();
		kart->tireOrient[i].z = (Real)rot.getZ();
		kart->tireOrient[i].w = (Real)-rot.getW();
	}

	// Save forward vector
	btVector3 Up = toBtVector(&kart->Up);

	kart->forDirection = (m_karts[kart_id]->vehicle->getForwardVector()).rotate(Up,DEGTORAD(-90));

	// Update the karts height above ground and what point is bellow it for rendering shadows
	// Casts ray 20 units directly down from karts position, which is the size of our arena so 
	// we should not go higher than that.
	btVector3 downRay = pos - btVector3(0,20,0);
	btCollisionWorld::ClosestRayResultCallback RayCallback(pos, downRay);

	m_world->rayTest(pos, downRay, RayCallback);

	if(RayCallback.hasHit())
	{
		btVector3 hitEnd = RayCallback.m_hitPointWorld;	// Point in world coord where ray hit
		kart->heightOffGround = pos.getY() - hitEnd.getY();	// Height kart is off ground
		kart->groundHit = fromBtVector(&hitEnd);
		
		kart->groundNormal = fromBtVector(&Up);
	}

	// Camera
	// Performed for Ai & Player

	// @Kyle: Update this code to cast ray from kart to camera to see if wall is in way (outter walls cause issues)
	// put ray cast point slightly above the kart to not collide with smaller walls
	if (kart->isExploding)
	{
		btTransform trans;
		//trans.setOrigin( btVector3( 0, 3, 0 ) );
		trans.setOrigin( btVector3(0,100,0) );
		//trans.setRotation( btQuaternion( 0, 0, 0, 1 ) );
		m_karts[kart_id]->vehicle->getRigidBody()->setWorldTransform( trans );
	}
	else
	{
		Quaternion qOriNew = kart->Orient;
		Quaternion qOriMod = qOriNew;
		qOriMod.w = -qOriMod.w;
		Matrix matOri = Matrix::GetRotateQuaternion( qOriMod );

		Vector3 vUp = kart->Up = Vector3( 0, 1, 0 ).Transform( matOri );

		Vector3 vCamOfs = Vector3( 0, 1.0f, -1.5f ).Transform( matOri );
		vCamOfs.y = 1.0f;

		kart->camera.vFocus = kart->Pos + Vector3( 0, 0.5f, 0 );

		seconds = Clamp((Real)seconds, (Real)0.0f, (Real)0.10f);		// This is incase frame rate really drops.
		Real fLerpAmt = seconds * 5.0f;
		Clamp(fLerpAmt, 0.1f, 0.0f);
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
		Real fAdjFOV = Clamp(fAdjSpeed, (Real)0.f, (Real)MAX_SPEED) / (Real)MAX_SPEED;
		fAdjFOV = (Real)(Int32)( fAdjFOV * 30.0f );

		kart->camera.fFOV = Lerp( kart->camera.fFOV, 90.0f - fAdjFOV, fLerpAmt * 0.1f );


		//DEBUGOUT( "%f\n", state->Camera.fFOV );
		m_karts[kart_id]->lastspeed = fSpeed;
		kart->Speed = fSpeed;
	}

}

void Simulation::fireBullet(entity_id kart_id)
{
	if (m_karts.count(kart_id) == 0) 
	{
		DEBUGOUT("Error: Invalid kart_id %d on fireBullet call\n", kart_id);
		return;
	}

	btRaycastVehicle *kart = m_karts.at(kart_id)->vehicle;
	auto new_bullet = new Simulation::bullet();

	new_bullet->kart_id = kart_id;
	auto kart_ent = GETENTITY(kart_id, CarEntity);
	btVector3 Up = btVector3(kart_ent->Up.x,kart_ent->Up.y,kart_ent->Up.z) ;

	auto direction = (kart->getForwardVector()).rotate( Up ,DEGTORAD(-90));

	// Bake in speed
	direction *= STEP_PER_FRAME;

	// Perturb angle
	btScalar x = direction.getX();
	btScalar y = direction.getY();
	btScalar z = direction.getZ();
	double spread_max = 0.04;
	float spread = (float)((((double)(rand() % 200) / 100.0) - 1.0) * spread_max);
	x += spread;
	z += spread;

	// Perturb firing pos
	auto car1 = m_karts[kart_id]->vehicle->getRigidBody()->getMotionState();
	btTransform trans;
	car1->getWorldTransform(trans);
	btVector3 pos = trans.getOrigin();
	pos += direction * (double)(rand() % 200) / 300.0;

	new_bullet->position = pos;
	new_bullet->direction = btVector3(x,y,z);
	new_bullet->time_to_live = BULLET_TTL;
	list_of_bullets[new_bullet->bullet_id] = new_bullet;
}

void Simulation::fireRocket(entity_id kart_id)
{
	if (m_karts.count(kart_id) == 0) 
	{
		DEBUGOUT("Error: Invalid kart_id %d on fireBullet call\n", kart_id);
		return;
	}

	btRaycastVehicle *kart = m_karts.at(kart_id)->vehicle;
	auto new_rocket = new Simulation::rocket();

	new_rocket->kart_id = kart_id;
	auto kart_ent = GETENTITY(kart_id, CarEntity);
	btVector3 Up = btVector3(kart_ent->Up.x,kart_ent->Up.y,kart_ent->Up.z) ;

	auto direction = (kart->getForwardVector()).rotate( Up ,DEGTORAD(-90));
	direction.normalize();

	// Perturb angle
	btScalar x = direction.getX();
	btScalar y = direction.getY();
	btScalar z = direction.getZ();

	// Perturb firing pos
	auto car1 = m_karts[kart_id]->vehicle->getRigidBody()->getMotionState();
	btTransform trans;
	car1->getWorldTransform(trans);
	btVector3 pos = trans.getOrigin();

	new_rocket->position = pos;

	btVector3 sideways = Up.cross(direction);

	// up side
	new_rocket->positions[0] = pos + (HALF_ROCKET_HIGHT * Up) ;
	// down side
	new_rocket->positions[1] = pos - (HALF_ROCKET_HIGHT * Up) ;
	// left_side
	new_rocket->positions[2] = pos + (HALF_ROCKET_HIGHT * sideways) ;
	// right_side
	new_rocket->positions[3] = pos - (HALF_ROCKET_HIGHT * sideways) ;


	new_rocket->direction = btVector3(x,y,z);
	new_rocket->time_to_live = ROCKET_TTL;
	list_of_rockets[new_rocket->rocket_id] = new_rocket;
}


void Simulation::solveBulletFiring(entity_id firing_kart_id, btScalar min_angle, btScalar max_dist)
{
	std::vector<Events::Event *> events_out;

	auto kart1 = m_karts[firing_kart_id];
	auto kart1_pos = kart1->vehicle->getChassisWorldTransform().getOrigin();

	auto kart = GETENTITY(firing_kart_id, CarEntity);
	btVector3 Up = toBtVector(&kart->Up);

	auto kart1_forward = (kart1->vehicle->getForwardVector()).rotate(Up,DEGTORAD(-90));
	kart1_forward.normalize();
	kart1_pos += (kart1_forward / 3);

	// Find closest karts in cone of firing
	std::priority_queue<btScalar, std::vector<btScalar>, std::greater<btScalar>>possible_dists;
	std::map<btScalar, std::pair<entity_id, btVector3>>dists_to_karts;
	for (auto kart2_pair : m_karts) 
	{
		auto kart2 = kart2_pair.second;
		if (kart2->kart_id == firing_kart_id)
			continue;

		auto kart2_pos = kart2->vehicle->getChassisWorldTransform().getOrigin();
		auto kart1_to_kart2 = kart2_pos - kart1_pos;

		// Cull by Distance
		btScalar dist = kart1_to_kart2.length();
		if (dist >= max_dist)
			continue;

		// Cull by Angle
		btScalar angle = kart1_forward.dot(kart1_to_kart2.normalized());
		if (angle < min_angle)
			continue;

		possible_dists.push(dist);
		dists_to_karts[dist] = std::make_pair(kart2->kart_id, kart2_pos);
	}

	// Find closests kart not blocked by wall
	while (!possible_dists.empty()) 
	{
		auto dist = possible_dists.top();
		possible_dists.pop();
		auto kart_pair = dists_to_karts[dist];

		// Ray cast & test
		btCollisionWorld::ClosestRayResultCallback raycast_test(kart1_pos, kart_pair.second);
		m_world->rayTest(kart1_pos, kart_pair.second, raycast_test);

		if (!raycast_test.hasHit())
			continue;
		
		auto hit_obj = (struct phy_obj *)raycast_test.m_collisionObject->getUserPointer();
		if (hit_obj == NULL || !(hit_obj->is_kart))
			continue;

		entity_id kart_hit_id = hit_obj->kart_id;

		Events::Event *e = makeRerportEvent(firing_kart_id, kart_hit_id);
		events_out.push_back(e);
		break;
	}

	// Send out all the events of possible shooting for all karts
	mb.sendMail(events_out);
}

// Construct an event for reporting a possible target.
Events::Event* Simulation::makeRerportEvent(entity_id kart_shooting , entity_id kart_shot)
{
	auto event = NEWEVENT(ShootReport);
	
	event->shooting_kart_id = kart_shooting;
	event->kart_being_hit_id = kart_shot;


	return event;
}

void Simulation::do_pulse_powerup(entity_id using_kart_id)
{
	std::vector<entity_id> near_karts;

	auto using_kart = GETENTITY(using_kart_id, CarEntity);
	auto using_pos = using_kart->Pos;

	for (auto kart_pair : m_karts)
	{
		auto affected_kart = kart_pair.second;
		auto affected_kart_id = affected_kart->kart_id;

		if (affected_kart_id != using_kart_id)
		{
			auto current_kart = GETENTITY(affected_kart_id, CarEntity);
			auto pos = current_kart->Pos;

			btScalar delta_x =(using_pos.x - pos.x);
			btScalar delta_z =(using_pos.z - pos.z);

			float karts_dist = sqrt(delta_x*delta_x + delta_z*delta_z);
			bool close_enough = (karts_dist < DIST_FOR_PULSE);

			if (close_enough)
			{
				//DEBUGOUT("Kart id %d is close enough!\n", affected_kart_id)
				//DEBUGOUT("distance to it was %f\n", karts_dist)

				near_karts.push_back(affected_kart_id);

				btRigidBody *kart_affected_body = m_karts[affected_kart_id]->vehicle->getRigidBody();

				btVector3 pulse_dir = toBtVector( &current_kart->Pos) - toBtVector ( &using_kart->Pos) ;

				pulse_dir.setY(pulse_dir.getY() + Y_OFFSET_PULSE);

				// Just use as direction
				pulse_dir.normalize();

				float pulse_factor = MAX(DIST_FOR_PULSE - karts_dist, 1);

				//DEBUGOUT("Dirty pulse factor was: %f", DIST_FOR_PULSE - karts_dist)
				//DEBUGOUT("Pulse factor was %f\n", pulse_factor)
				
				float boost_force = MIN(PULSE_FORCE * pulse_factor, PULSE_CAP);

				btVector3 boost = pulse_dir * boost_force;

				//DEBUGOUT("boost force was %f\n", boost_force)

				kart_affected_body->applyImpulse(boost, btVector3(0,0,0));
		
			}
		}
	}

	return;
}

#define MIN_WHEEL_GAP 0.15f
#define CORRECTION_IMPULSE 40

void Simulation::preventKartFlipping(btRaycastVehicle *kart, btVector3 *up)
{
	btVector3 negUp;
	negUp.setX(up->getX() * -1 * CORRECTION_IMPULSE);
	negUp.setY(up->getY() * -1 * CORRECTION_IMPULSE);
	negUp.setZ(up->getZ() * -1 * CORRECTION_IMPULSE);

	// Get the location of the 4 wheels
	for(int i = 0; i < 4; i++)
	{
		btVector3 wheelPos = kart->getWheelTransformWS(i).getOrigin();
		for(int j = i + 1; j < 4; j++)
		{
			btVector3 nextWheelPos = kart->getWheelTransformWS(j).getOrigin();
			if(abs(wheelPos.getY() - nextWheelPos.getY()) > MIN_WHEEL_GAP)
			{				
				btVector3 wheelRelativePosition;
				// Find the highest wheel and apply the force at that wheels position.
				if(wheelPos.getY() > nextWheelPos.getY())
				{
					wheelRelativePosition = wheelPos - kart->getChassisWorldTransform().getOrigin();
				}
				else
				{
					wheelRelativePosition = nextWheelPos - kart->getChassisWorldTransform().getOrigin();
				}

				kart->getRigidBody()->applyImpulse( negUp, wheelRelativePosition);
			}
		}
	}
}
