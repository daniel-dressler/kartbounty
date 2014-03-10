#include "enemyai.h"
#include "util/Sphere.h"
#include "util/Square.h"

#define LIMIT_FOR_STUCK 0.004f
#define REVERSE_TRESHOLD 6
#define RESET_TRESHOLD REVERSE_TRESHOLD*4

// temp
int current_state;
std::vector<Vector3> path;
std::vector<Square> obs_sqr;


EnemyAi::EnemyAi()
{
	// seed random
	srand(time(NULL));
	m_mb.request( Events::EventType::KartDestroyed );
	m_mb.request( Events::EventType::AiKart );

	// Possible points for the car to wander about.
	init_graph();

	current_state = 0;
}

EnemyAi::~EnemyAi()
{

}

void EnemyAi::setup()
{
	EnemyAi::init_obs_sqr();
}

Vector3 EnemyAi::think_of_target(struct ai_kart *kart)
{
	kart->TimeStartedTarget = 0;
	return kart->target_to_move = get_target_roaming();
}

Vector3 EnemyAi::get_target_roaming()
{
	int rand = (std::rand() % path.size());
	if (rand == current_state)
		rand = (std::rand() % path.size());

	Vector3 answer = path.at(rand);
	current_state = rand;

	return answer;
}

void EnemyAi::update(Real elapsed_time)
{
	std::vector<Events::Event *> inputEvents;

	// Send new event
	for (Events::Event *event : m_mb.checkMail()) {
		switch( event->type ) {
		case Events::EventType::AiKart:
		{
			auto kart_id = ((Events::AiKartEvent *)event)->kart_id;
			struct ai_kart *kart;
			if (m_karts.count(kart_id) == 0) {
				kart = m_karts[kart_id] = new ai_kart;
				kart->kart_id = kart_id;
				think_of_target(kart);
			} else {
				kart = m_karts[kart_id];
			}
			inputEvents.push_back(move_kart(kart, elapsed_time));
		}
			break;

		case Events::EventType::KartDestroyed:
		{
			auto kart_id = ((Events::KartCreatedEvent *)event)->kart_id;
			// We only need delete the kart
			// If it was ai controlled
			if (m_karts.count(kart_id) > 0) {
				m_karts.erase(kart_id);
			}
		}
			break;

		default:
			break;
		}
	}
	m_mb.emptyMail();

	m_mb.sendMail(inputEvents);
}

bool UseAI = 1;
bool Prev = 0;

Events::InputEvent *EnemyAi::move_kart(struct ai_kart *kart_local, Real elapsed_time)
{
	// increment it's timer by time
	kart_local->TimeStartedTarget += elapsed_time;

	//DEBUGOUT("Stuck value %f for %d\n", stuck_counter[index], index)
	//DEBUGOUT("Time : %f\n", state->Karts[index].TimeStartedTarget)

	auto kart_entity = GETENTITY(kart_local->kart_id, CarEntity);
	Vector3 target_3 = kart_local->target_to_move;
	Vector2 target = Vector2(target_3.x, target_3.z);
	Vector3 pos = kart_entity->Pos;
	Vector3 oldPos = kart_local->lastPos;

	// Calculate the updated distance and the difference in angle.
	btScalar diff_in_angles = getAngle(target, pos, kart_entity->forDirection) / PI;
	btScalar distance_to_target = sqrtf( pow((pos.x - target.x) , 2) 
											+ pow((pos.z - target.y),2) );

	if (distance_to_target < 1.0f) 
		think_of_target(kart_local);

	// check if car stuck, increase stuck timer if stuck.
	bool stuck = (abs(oldPos.x - pos.x) < LIMIT_FOR_STUCK && 
						abs(oldPos.z - pos.z) < LIMIT_FOR_STUCK);
	if (stuck) {
		kart_local->time_stuck += elapsed_time;
	} else {
		kart_local->time_stuck = 0;
	}
	
	// Generate input for car.
	auto directions = drive(diff_in_angles, distance_to_target, kart_local);

	// Check if the kart is stuck for too long, if so, reset.
	if (kart_local->time_stuck >= RESET_TRESHOLD)
	{
		kart_local->time_stuck = 0;
		directions->reset_requested = true;
	}
	
	
	// DEBUGOUT("angle: %f, dist: %f\n", RADTODEG(diff_in_angles), distance_to_target);

	// update old pos
	kart_local->lastPos = pos;
	return directions;
}

btScalar EnemyAi::getAngle(Vector2 target, Vector3 pos, btVector3 forward)
{
	//Position location = Position(state->Karts[index].vPos.x, state->Karts[index].vPos.z);
	btVector3 toLocation = btVector3( target.x - pos.x, 0.f , target.y - pos.z);
	toLocation.safeNormalize();
	//DEBUGOUT("target: %f %f\n",target.x , target.y)

	forward.setY(0.f);
	btVector3 zAxis = forward.cross(toLocation);
	btScalar angle = forward.angle(toLocation);

	if (zAxis.getY() < 0)
		angle = -angle;

	return angle;
}

Events::InputEvent *EnemyAi::drive(btScalar diff_ang, btScalar dist, struct ai_kart *kart_local)
{
		// Make a new event.
		auto directions = NEWEVENT(Input);
		directions->kart_id = kart_local->kart_id;
		btScalar turn_value = diff_ang;

		// Change the previous event to suit situation.
		if (kart_local->time_stuck >= REVERSE_TRESHOLD)
		{
			//DEBUGOUT("DRIVE BACKWARDS!\n");
			directions->leftTrigger = 1;
			directions->rightTrigger = 0;
	
			if (diff_ang > 0)
				turn_value = -1.f;
			else
				turn_value = 1.f;

			kart_local->time_stuck -= 0.0075f;
		}
		else
		{
			//DEBUGOUT("DRIVE FORWARD!!\n");
			
			directions->rightTrigger = 0.7;
			directions->leftTrigger = 0;

			if (abs(diff_ang) < 0.7f  && abs(diff_ang) > 0.05f )
			{
				if (diff_ang > 0)
					turn_value = diff_ang + 0.7f;
				else
					turn_value = -diff_ang - 0.7f;
			}
			else
			{
				if (diff_ang > 0)
					turn_value = diff_ang;
				else
					turn_value = -diff_ang;
			}
		}

		float avoidance_angle = avoid_obs_sqr(kart_local);
		directions->leftThumbStickRL = avoidance_angle == 0 ? turn_value : avoidance_angle;

		/* @Eric: How would these get out of bounds?
		if (m_pCurrentInput[index]->leftThumbStickRL > 1)
			m_pCurrentInput[index]->leftThumbStickRL = 1;

		if (m_pCurrentInput[index]->leftThumbStickRL < -1)
			m_pCurrentInput[index]->leftThumbStickRL = -1;
		*/

		return directions;
}

void EnemyAi::init_graph()
{
	path.push_back(Vector3(15.5f, 0.f, 15.5f));
	path.push_back(Vector3(-15.5f, 0.f, 15.5f));
	path.push_back(Vector3(15.5f, 0.f, -15.5f));
	path.push_back(Vector3(-15.5f, 0.f, -15.5f));
	path.push_back(Vector3(0.f, 0.f, 0.f));
	path.push_back(Vector3(11.5f, 0.f, 0.f));
	path.push_back(Vector3(-11.5f, 0.f, 0.f));
	path.push_back(Vector3(0.f, 0.f, 11.5f));
	path.push_back(Vector3(0.f, 0.f, -11.5f));
}

// This maps the obsticles to square obsticles.
void EnemyAi::init_obs_sqr()
{
	Vector3 center;
	Vector3 top_left;
	Vector3 bot_right;
	/// ============= First quarter of the map X > 0 , Z > 0================
	// 21-20-22-23 - left part
	center = Vector3(4.f, 0, 14.f); top_left = Vector3(1.28f, 0, 15.544f); bot_right = Vector3(3.87f, 0, 12.695f);
	Square s0 = Square(center, top_left, bot_right);

	// 21-20-22-23 - right part
	center = Vector3(4.f, 0, 14.f); top_left = Vector3(3.88f, 0, 15.544f); bot_right = Vector3(6.07f, 0, 12.695f);
	Square s1 = Square(center, top_left, bot_right);


	// 14-13-15-12 suqare
	center = Vector3(11.f,0,14.f);	top_left = Vector3(9.571f,0,15.462f);  bot_right = Vector3(12.77f,0,12.695f);
	Square s2 = Square(center, top_left, bot_right);
	// 12-16-11-17 square
	center = Vector3(14.f, 0, 12.f); top_left = Vector3(12.77f, 0, 12.69f); bot_right = Vector3(15.37f, 0, 9.65f);
	Square s3 = Square(center, top_left, bot_right);
	// 12 square corner inside (center slightly moved higher on the z)
	center = Vector3(12.77f, 0, 13.00f); top_left = Vector3(11.45f, 0, 11.45f); bot_right = Vector3(11.77f, 0, 13.77f);
	Square s4 = Square(center, top_left, bot_right);

	// 19-18 - top half
	center = Vector3(14.f, 0, 0.f); top_left = Vector3(12.77f, 0, 5.5f); bot_right = Vector3(15.37f, 0, 2.f);
	Square s5 = Square(center, top_left, bot_right);
	center = Vector3(14.f, 0, 0.f); top_left = Vector3(12.77f, 0, 2.f); bot_right = Vector3(15.37f, 0, 0.f);
	Square s6 = Square(center, top_left, bot_right);

	// 7-8-5-6 suqare
	center = Vector3(6.f,0,9.f);	top_left = Vector3(4.571f,0,10.462f);  bot_right = Vector3(7.77f,0,7.695f);
	Square s7 = Square(center, top_left, bot_right);
	// 5-9-10-4 square
	center = Vector3(9.f, 0, 7.f); top_left = Vector3(7.77f, 0, 7.69f); bot_right = Vector3(10.37f, 0, 4.65f);
	Square s8 = Square(center, top_left, bot_right);
	// 5 square corner inside (center slightly moved higher on the z)
	center = Vector3(7.77f, 0.F, 8.00f); top_left = Vector3(6.45f, 0, 6.45f); bot_right = Vector3(6.77f, 0, 8.77f);
	Square s9 = Square(center, top_left, bot_right);

	// 1-2-3 - center offseted to be 1
	center = Vector3(2.71f, 0, 2.63f); top_left = Vector3(2.71f, 0,5.46f); bot_right = Vector3(5.58f, 0, 2.63f);
	Square s10 = Square(center, top_left, bot_right);

	// q1 no flip , X > 0, Z > 0
	obs_sqr.push_back(s0);
	obs_sqr.push_back(s1);
	obs_sqr.push_back(s2);
	obs_sqr.push_back(s3);
	obs_sqr.push_back(s4);
	obs_sqr.push_back(s5);
	obs_sqr.push_back(s6);
	obs_sqr.push_back(s7);
	obs_sqr.push_back(s8);
	obs_sqr.push_back(s9);
	obs_sqr.push_back(s10);

	// q2 flip on the Z axis. X < 0, Z > 0
	obs_sqr.push_back(s0.flip_z_axis());
	obs_sqr.push_back(s1.flip_z_axis());
	obs_sqr.push_back(s2.flip_z_axis());
	obs_sqr.push_back(s3.flip_z_axis());
	obs_sqr.push_back(s4.flip_z_axis());
	obs_sqr.push_back(s5.flip_z_axis());
	obs_sqr.push_back(s6.flip_z_axis());
	obs_sqr.push_back(s7.flip_z_axis());
	obs_sqr.push_back(s8.flip_z_axis());
	obs_sqr.push_back(s9.flip_z_axis());
	obs_sqr.push_back(s10.flip_z_axis());

	// q3 flip on the Z axis and the X axis. X < 0, Z < 0
	obs_sqr.push_back(s0.flip_z_axis().flip_x_axis());
	obs_sqr.push_back(s1.flip_z_axis().flip_x_axis());
	obs_sqr.push_back(s2.flip_z_axis().flip_x_axis());
	obs_sqr.push_back(s3.flip_z_axis().flip_x_axis());
	obs_sqr.push_back(s4.flip_z_axis().flip_x_axis());
	obs_sqr.push_back(s5.flip_z_axis().flip_x_axis());
	obs_sqr.push_back(s6.flip_z_axis().flip_x_axis());
	obs_sqr.push_back(s7.flip_z_axis().flip_x_axis());
	obs_sqr.push_back(s8.flip_z_axis().flip_x_axis());
	obs_sqr.push_back(s9.flip_z_axis().flip_x_axis());
	obs_sqr.push_back(s10.flip_z_axis().flip_x_axis());

	// q4 flip on the  X axis. X > 0, Z < 0
	obs_sqr.push_back(s0.flip_x_axis());
	obs_sqr.push_back(s1.flip_x_axis());
	obs_sqr.push_back(s2.flip_x_axis());
	obs_sqr.push_back(s3.flip_x_axis());
	obs_sqr.push_back(s4.flip_x_axis());
	obs_sqr.push_back(s5.flip_x_axis());
	obs_sqr.push_back(s6.flip_x_axis());
	obs_sqr.push_back(s7.flip_x_axis());
	obs_sqr.push_back(s8.flip_x_axis());
	obs_sqr.push_back(s9.flip_x_axis());
	obs_sqr.push_back(s10.flip_x_axis());

	// TODO need to add 4 corner squares for 2nd inner loop.
	// TODO need to add 4 outer walls.
}

#define LENGTH_OF_RAY 3.75
#define LENGTH_OF_RAY_FORWARD 3.5
#define SENSOR_ANGLE 10

#define IN_FRONT_ANGLE 140
#define FAR_AWAY 6

float EnemyAi::avoid_obs_sqr(struct ai_kart *kart_local)
{
	auto kart_entity = GETENTITY(kart_local->kart_id, CarEntity);
	btVector3 forward = kart_entity->forDirection;
	Vector3 pos = kart_entity->Pos;

	std::vector<Square> danger_sqr;
	//std::vector<int> old_index;

	for(Square square : obs_sqr)
	{
		float angle = getAngle(Vector2(square.getCenter().x, square.getCenter().z), pos, forward);
		if ( abs(RADTODEG(angle)) < IN_FRONT_ANGLE )
		{
			btVector3 left_ray = forward.rotate(btVector3(0,1,0), DEGTORAD(-SENSOR_ANGLE));
			btVector3 right_ray = forward.rotate(btVector3(0,1,0), DEGTORAD(SENSOR_ANGLE));
			
			Vector3 direction = Vector3(forward.getX(), 0, forward.getZ());
			direction = direction.Normalize();

			Vector3 left_sensor = Vector3(left_ray.getX(), 0, left_ray.getZ()).Normalize();
			Vector3 right_sensor = Vector3(right_ray.getX(), 0, right_ray.getZ()).Normalize();

			int intersections_forward = square.LineIntersectsSquare(pos, direction, LENGTH_OF_RAY_FORWARD);
			int intersections_left = square.LineIntersectsSquare(pos, left_sensor, LENGTH_OF_RAY);
			int intersections_right = square.LineIntersectsSquare(pos, right_sensor, LENGTH_OF_RAY);

			if (intersections_forward + intersections_left + intersections_right > 0)
			{
				danger_sqr.push_back(square);
				//old_index.push_back(i);

				/*
				if (intersections_forward > 0)
					DEBUGOUT("Intersects forward with %d %f times!\n" , i, intersections_forward);

				if (intersections_left > 0)
					DEBUGOUT("Intersects left with %d %f times!\n" , i, intersections_left);

				if (intersections_right > 0)
					DEBUGOUT("Intersects right with %d %f times!\n" , i, intersections_right);
				*/
			}
		}
		
	}

	int most_threat = -1;
	float most_threat_dist = 10000000.f;

	for (uint32_t i = 0; i<danger_sqr.size(); i++)
	{
		Square square = danger_sqr.at(i);
		Vector3 center = square.getCenter();

		float dist_from_center = get_distance(pos, center);

		if (dist_from_center < most_threat_dist)
		{
			most_threat = i;
			most_threat_dist = dist_from_center;
		}
	}

	float turn_value = 0;
	if (most_threat != -1) // This means there's a threat that needs to be steered!
	{
		Square most_threat_sqr = danger_sqr.at(most_threat);
		Vector3 center_threat = most_threat_sqr.getCenter();
		Vector2 sqr_center = Vector2(center_threat.x, center_threat.z);

		// get angle between car and the most threatening obsticle.
		float steer_correction_angle = getAngle(sqr_center, pos, forward);

		if (steer_correction_angle > 0)
		{
			if (RADTODEG(steer_correction_angle) < 10)
				turn_value = -0.8f;
			else
				turn_value = -1.f;
		}
		else 
		{
			if (RADTODEG(steer_correction_angle) < -10)
				turn_value = 0.8f;
			else
				turn_value = 1.f;
		}
	}
	return turn_value;
}

// get the distance between two points
float EnemyAi::get_distance(Vector3 a, Vector3 b)
{
	return sqrtf( pow( ((float)a.x - (float)b.x) , 2 ) + pow(((float)a.z - (float)b.z),2) );
}


