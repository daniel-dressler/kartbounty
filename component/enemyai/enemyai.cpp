#include <time.h>

#include "enemyai.h"
#include "util/Sphere.h"
#include "util/Square.h"

#define LIMIT_FOR_STUCK 0.1f
#define REVERSE_TRESHOLD 1.5f
#define REVERSE_TIME 0.7f
#define RESET_TRESHOLD 4.f

// temp
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
}

EnemyAi::~EnemyAi()
{

}

void EnemyAi::setup()
{
	EnemyAi::init_obs_sqr();
}

void EnemyAi::think_of_target(struct ai_kart *kart)
{
	kart->target_timer = 0;
	get_target_roaming(kart);
}

void EnemyAi::get_target_roaming(struct ai_kart *kart)
{
	int rand = (std::rand() % path.size());
	if (rand == kart->current_target_index)
		rand = (std::rand() % path.size());

	Vector3 answer = path.at(rand);
	kart->current_target_index = rand;

	kart->target_to_move = answer;
	kart->driving_mode = drivingMode::Roaming;
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
			if (m_karts.count(kart_id) == 0) 
			{
				kart = m_karts[kart_id] = new ai_kart;
				kart->kart_id = kart_id;
				think_of_target(kart);
			} else 
			{
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
			if (m_karts.count(kart_id) > 0) 
			{
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
	kart_local->target_timer -= elapsed_time;

	//DEBUGOUT("Stuck value %f for %d\n", kart_local->time_stuck, kart_local->kart_id)
	//DEBUGOUT("Time : %f\n", kart_local->target_timer)

	auto kart_entity = GETENTITY(kart_local->kart_id, CarEntity);
	Vector3 target_3 = kart_local->target_to_move;
	Vector2 target = Vector2(target_3.x, target_3.z);
	Vector3 pos = kart_entity->Pos;
	Vector3 oldPos = kart_local->lastPos;

	// Calculate the updated distance and the difference in angle.
	btScalar diff_in_angles = getAngle(target, pos, &kart_entity->forDirection) / PI;
	btScalar distance_to_target = sqrtf( pow((pos.x - target.x) , 2) 
											+ pow((pos.z - target.y),2) );

	if (distance_to_target < 1.0f) 
		think_of_target(kart_local);

	// check if car stuck, increase stuck timer if stuck.
	bool stuck = (abs(oldPos.x - pos.x) < LIMIT_FOR_STUCK && 
						abs(oldPos.z - pos.z) < LIMIT_FOR_STUCK);
	if (stuck) 
	{
		kart_local->time_stuck += elapsed_time;
	} 
	else 
	{
		kart_local->time_stuck = 0;
	}
	
	// Generate input for car.
	auto directions = drive(diff_in_angles, distance_to_target, kart_local, elapsed_time);

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

btScalar EnemyAi::getAngle(Vector2 target, Vector3 pos, btVector3 *forward)
{
	//Position location = Position(state->Karts[index].vPos.x, state->Karts[index].vPos.z);
	btVector3 toLocation = btVector3( target.x - pos.x, 0.f , target.y - pos.z);
	toLocation.safeNormalize();
	//DEBUGOUT("target: %f %f\n",target.x , target.y)

	forward->setY(0.f);
	btVector3 zAxis = forward->cross(toLocation);
	btScalar angle = forward->angle(toLocation);

	if (zAxis.getY() < 0)
		angle = -angle;

	return angle;
}

Events::InputEvent *EnemyAi::drive(btScalar diff_ang, btScalar dist, struct ai_kart *kart_local, Real elapsed_time)
{
		// Make a new event.
		auto directions = NEWEVENT(Input);
		directions->kart_id = kart_local->kart_id;
		btScalar turn_value = diff_ang;

		// Change the previous event to suit situation.
		if (kart_local->time_stuck >= REVERSE_TRESHOLD)
		{
			kart_local->driving_mode = drivingMode::Reverse;
			kart_local->target_timer = REVERSE_TIME;
		}

		if (kart_local->driving_mode == drivingMode::Reverse)
		{
			//DEBUGOUT("DRIVE BACKWARDS!\n");
			directions->leftTrigger = 1;
			directions->rightTrigger = 0;
	
			if (diff_ang > 0)
				turn_value = -1.f;
			else
				turn_value = 1.f;

			if (kart_local->target_timer <= 0)
				think_of_target(kart_local);
		}
		else
		{
			//DEBUGOUT("DRIVE FORWARD!!\n");
			
			directions->rightTrigger = .9f;
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
	Vector3 top_right;
	Vector3 bot_right;
	Vector3 bot_left;
	/// ============= First quarter of the map X > 0 , Z > 0================
	// 21-20-22-23 - left part
	center = Vector3(4.f, 0, 14.f); top_left = Vector3(1.28f, 0, 15.544f); bot_right = Vector3(3.87f, 0, 12.695f);
	Square s0 = Square(center, top_left, bot_right);

	// 21-20-22-23 - right part
	center = Vector3(4.f, 0, 14.f); top_left = Vector3(3.88f, 0, 15.544f); bot_right = Vector3(6.77f, 0, 12.695f);
	Square s1 = Square(center, top_left, bot_right);


	// 14-13-15-12 suqare
	center = Vector3(11.f,0,14.f);	top_left = Vector3(9.571f,0,15.462f);  bot_right = Vector3(12.77f,0,12.695f);
	Square s2 = Square(center, top_left, bot_right);
	// 12-16-11-17 square
	center = Vector3(14.f, 0, 12.f); top_left = Vector3(12.77f, 0, 12.69f); bot_right = Vector3(15.37f, 0, 9.65f);
	Square s3 = Square(center, top_left, bot_right);

	// 13-12-11 triangle
	center = Vector3(12.75f, 0, 12.75f); top_left = Vector3(9.6f, 0, 12.76f); top_right = Vector3(12.75f, 0 , 12.75f); bot_right = Vector3(12.75f, 0, 9.6f); bot_left = bot_right;
	Square s4 = Square(center, top_left, top_right, bot_right, bot_left);

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

	// 6-5-4 triangle
	center = Vector3(7.5f, 0.F, 7.5f); top_left = Vector3(4.45f, 0,7.7f); top_right = Vector3(7.5f, 0, 7.5f); bot_right = Vector3(7.6f, 0, 4.77f); bot_left = bot_right;
	Square s9 = Square(center, top_left, top_right, bot_right, bot_left);

	// 1-2-3 - center offseted to be 1
	center = Vector3(3.15f, 0, 3.15f); top_left = Vector3(2.71f, 0,5.46f); top_right = top_left; bot_right = Vector3(5.58f, 0, 2.63f); bot_left = Vector3(2.75f, 0, 2.75);
	Square s10 = Square(center, top_left, top_right, bot_right, bot_left);

	// 15-16-12 triangle
	center = Vector3(12.75f, 0, 12.75f); top_left = Vector3(13.f, 0, 15.56f); top_right = top_left; bot_right = Vector3(15.37f, 0, 13.12f); bot_left = Vector3(12.75f, 0, 12.75f);
	Square s11 = Square(center, top_left, top_right, bot_right, bot_left);

	// 8-9-6 triangle
	center = Vector3(7.5f, 0.F, 7.5f); top_left = Vector3(8.f, 0,10.2f); top_right = top_left; bot_right = Vector3(10.42f, 0, 8.35f); bot_left = Vector3(7.5f, 0.F, 7.5f);
	Square s12 = Square(center, top_left, top_right, bot_right, bot_left);

	// Outer walls - top/bottom
	center = Vector3(3.f, 0, 19.f); top_left = Vector3(1.f, 0, 19.f);bot_right = Vector3(5.f, 0, 19.f);
	Square s13 = Square(center, top_left, bot_right);
	center = Vector3(5.f, 0, 19.f); top_left = Vector3(3.f, 0, 19.f);bot_right = Vector3(7.f, 0, 19.f);
	Square s14 = Square(center, top_left, bot_right);
	center = Vector3(7.f, 0, 19.f); top_left = Vector3(5.f, 0, 19.f);bot_right = Vector3(9.f, 0, 19.f);
	Square s15 = Square(center, top_left, bot_right);
	center = Vector3(9.f, 0, 19.f); top_left = Vector3(7.f, 0, 19.f);bot_right = Vector3(11.f, 0, 19.f);
	Square s16 = Square(center, top_left, bot_right);
	center = Vector3(11.f, 0, 19.f); top_left = Vector3(9.f, 0, 19.f);bot_right = Vector3(13.f, 0, 19.f);
	Square s17 = Square(center, top_left, bot_right);
	center = Vector3(13.f, 0, 19.f); top_left = Vector3(11.f, 0, 19.f);bot_right = Vector3(15.f, 0, 19.f);
	Square s18 = Square(center, top_left, bot_right);

	// Outer walls - left/right
	center = Vector3(19.f, 0, 3.f); top_left = Vector3(19.f, 0, 1.f);bot_right = Vector3(19.f, 0, 5.f);
	Square s19 = Square(center, top_left, bot_right);
	center = Vector3(19.f, 0, 5.f); top_left = Vector3(19.f, 0, 3.f);bot_right = Vector3(19.f, 0, 7.f);
	Square s20 = Square(center, top_left, bot_right);
	center = Vector3(19.f, 0, 7.f); top_left = Vector3(19.f, 0, 5.f);bot_right = Vector3(19.f, 0, 9.f);
	Square s21 = Square(center, top_left, bot_right);
	center = Vector3(19.f, 0, 9.f); top_left = Vector3(19.f, 0, 7.f);bot_right = Vector3(19.f, 0, 11.f);
	Square s22 = Square(center, top_left, bot_right);
	center = Vector3(19.f, 0, 11.f); top_left = Vector3(19.f, 0, 9.f);bot_right = Vector3(19.f, 0, 13.f);
	Square s23 = Square(center, top_left, bot_right);
	center = Vector3(19.f, 0, 13.f); top_left = Vector3(19.f, 0, 11.f);bot_right = Vector3(19.f, 0, 15.f);
	Square s24 = Square(center, top_left, bot_right);

	// Outer wall - corner padding
	center = Vector3(19.f, 0, 15.f); top_left = Vector3(19.f, 0, 12.f);bot_right = Vector3(19.f, 0, 16.f);
	Square s25 = Square(center, top_left, bot_right);
	center = Vector3(15.f, 0, 19.f); top_left = Vector3(12.f, 0, 19.f);bot_right = Vector3(16.f, 0, 19.f);
	Square s26 = Square(center, top_left, bot_right);

	// Corner triangle
	center = Vector3(19.f, 0,19.f); top_left = Vector3(14.9f, 0, 19.f); top_right = center; bot_right = Vector3(19.f, 0, 14.9f); bot_left = bot_right;
	Square s27 = Square(center, top_left, top_right, bot_right, bot_left);

	// 0 squares
	center = Vector3(0.f, 0, 19.f); top_left = Vector3(-2.f, 0, 19.f);bot_right = Vector3(2.f, 0, 19.f);
	Square s28 = Square(center, top_left, bot_right);
	center = Vector3(0.f, 0, -19.f); top_left = Vector3(-2.f, 0, -19.f);bot_right = Vector3(2.f, 0, -19.f);
	Square s29 = Square(center, top_left, bot_right);
	center = Vector3(19.f, 0, 0.f); top_left = Vector3(19.f, 0, 2.f);bot_right = Vector3(19.f, 0, -2.f);
	Square s30 = Square(center, top_left, bot_right);
	center = Vector3(-19.f, 0, 0.f); top_left = Vector3(-19.f, 0, 2.f);bot_right = Vector3(-19.f, 0, -2.f);
	Square s31 = Square(center, top_left, bot_right);

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
	obs_sqr.push_back(s11);
	obs_sqr.push_back(s12);
	obs_sqr.push_back(s13);
	obs_sqr.push_back(s14);
	obs_sqr.push_back(s15);
	obs_sqr.push_back(s16);
	obs_sqr.push_back(s17);
	obs_sqr.push_back(s18);
	obs_sqr.push_back(s19);
	obs_sqr.push_back(s20);
	obs_sqr.push_back(s21);
	obs_sqr.push_back(s22);
	obs_sqr.push_back(s23);
	obs_sqr.push_back(s24);
	obs_sqr.push_back(s25);
	obs_sqr.push_back(s26);
	obs_sqr.push_back(s27);

	// 0's
	obs_sqr.push_back(s28);
	obs_sqr.push_back(s29);
	obs_sqr.push_back(s30);
	obs_sqr.push_back(s31);

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
	obs_sqr.push_back(s11.flip_z_axis());
	obs_sqr.push_back(s12.flip_z_axis());
	obs_sqr.push_back(s13.flip_z_axis());
	obs_sqr.push_back(s14.flip_z_axis());
	obs_sqr.push_back(s15.flip_z_axis());
	obs_sqr.push_back(s16.flip_z_axis());
	obs_sqr.push_back(s17.flip_z_axis());
	obs_sqr.push_back(s18.flip_z_axis());
	obs_sqr.push_back(s19.flip_z_axis());
	obs_sqr.push_back(s20.flip_z_axis());
	obs_sqr.push_back(s21.flip_z_axis());
	obs_sqr.push_back(s22.flip_z_axis());
	obs_sqr.push_back(s23.flip_z_axis());
	obs_sqr.push_back(s24.flip_z_axis());
	obs_sqr.push_back(s25.flip_z_axis());
	obs_sqr.push_back(s26.flip_z_axis());
	obs_sqr.push_back(s27.flip_z_axis());

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
	obs_sqr.push_back(s11.flip_z_axis().flip_x_axis());
	obs_sqr.push_back(s12.flip_z_axis().flip_x_axis());
	obs_sqr.push_back(s13.flip_z_axis().flip_x_axis());
	obs_sqr.push_back(s14.flip_z_axis().flip_x_axis());
	obs_sqr.push_back(s15.flip_z_axis().flip_x_axis());
	obs_sqr.push_back(s16.flip_z_axis().flip_x_axis());
	obs_sqr.push_back(s17.flip_z_axis().flip_x_axis());
	obs_sqr.push_back(s18.flip_z_axis().flip_x_axis());
	obs_sqr.push_back(s19.flip_z_axis().flip_x_axis());
	obs_sqr.push_back(s20.flip_z_axis().flip_x_axis());
	obs_sqr.push_back(s21.flip_z_axis().flip_x_axis());
	obs_sqr.push_back(s22.flip_z_axis().flip_x_axis());
	obs_sqr.push_back(s23.flip_z_axis().flip_x_axis());
	obs_sqr.push_back(s24.flip_z_axis().flip_x_axis());
	obs_sqr.push_back(s25.flip_z_axis().flip_x_axis());
	obs_sqr.push_back(s26.flip_z_axis().flip_x_axis());
	obs_sqr.push_back(s27.flip_z_axis().flip_x_axis());

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
	obs_sqr.push_back(s11.flip_x_axis());
	obs_sqr.push_back(s12.flip_x_axis());
	obs_sqr.push_back(s13.flip_x_axis());
	obs_sqr.push_back(s14.flip_x_axis());
	obs_sqr.push_back(s15.flip_x_axis());
	obs_sqr.push_back(s16.flip_x_axis());
	obs_sqr.push_back(s17.flip_x_axis());
	obs_sqr.push_back(s18.flip_x_axis());
	obs_sqr.push_back(s19.flip_x_axis());
	obs_sqr.push_back(s20.flip_x_axis());
	obs_sqr.push_back(s21.flip_x_axis());
	obs_sqr.push_back(s22.flip_x_axis());
	obs_sqr.push_back(s23.flip_x_axis());
	obs_sqr.push_back(s24.flip_x_axis());
	obs_sqr.push_back(s25.flip_x_axis());
	obs_sqr.push_back(s26.flip_x_axis());
	obs_sqr.push_back(s27.flip_x_axis());
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
		float angle = getAngle(Vector2(square.getCenter().x, square.getCenter().z), pos, &forward);
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
		float steer_correction_angle = getAngle(sqr_center, pos, &forward);

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


