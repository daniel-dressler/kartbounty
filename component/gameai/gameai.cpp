#include "gameai.h"
#include <chrono>
#include <thread>
#include <math.h>
#include "util/Sphere.h"
#include "util/Square.h"

#define LIMIT_FOR_STUCK 0.004f
#define REVERSE_TRESHOLD 10
#define RESET_TRESHOLD REVERSE_TRESHOLD*4

// temp
int current_state;
std::map<int, Vector3> path;
std::map<int, Sphere> obs;
std::map<int, Square> obs_sqr;

GameAi::GameAi()
{
	state = GetMutState();
	m_mb = new Events::Mailbox();	
	m_mb->request( Events::EventType::Quit );

	// Setup previous input event struct
	for (int i = 0; i<NUM_KARTS; i++)
	{
		m_pPreviousInput[i] = NEWEVENT(Input);
		m_pPreviousInput[i]->rightTrigger = 0;
		m_pPreviousInput[i]->leftTrigger = 0;
		m_pPreviousInput[i]->leftThumbStickRL = 0;
		m_pPreviousInput[i]->rightThumbStickRL = 0;
		m_pPreviousInput[i]->aPressed = false;
		m_pPreviousInput[i]->bPressed = false;
		m_pPreviousInput[i]->xPressed = false;
		m_pPreviousInput[i]->yPressed = false;
		m_pPreviousInput[i]->kart_index = 0;
	}

	// Scripted path while there's no "brain". HACK. (will be replaced soon)
	init_graph();
	GameAi::init_obs_sqr();
	current_state = 0;
}

GameAi::~GameAi()
{

}

int GameAi::planFrame()
{
	const std::vector<Events::Event*> events = m_mb->checkMail();
	for (Events::Event *event : events) {
		switch( event->type ) {
		case Events::EventType::Quit:
			return 0;
			break;
		default:
			break;
		}
	}
	m_mb->emptyMail();

	// Yield until next frame
	Real timeForLastFrame = frame_timer.CalcSeconds();
	Real sleepPeriod = 0.016 - timeForLastFrame;
	if (sleepPeriod > 0.0) {
		std::chrono::milliseconds timespan( (int)( sleepPeriod * 1000.0f ) );
		std::this_thread::sleep_for(timespan);
	}

	// Report FPS
	static int32_t frames = 0;
	frames++;
	static Real timeAtLastFrame = 0;
	if ( (int32_t)timeAtLastFrame != (int32_t)fps_timer.CalcSeconds()) {
		//DEBUGOUT( "FPS: %d\n", frames);
		frames = 0;
	}
	timeAtLastFrame = fps_timer.CalcSeconds();

	return 1;
}

void GameAi::update(Real elapsed_time)
{
	// =========== AI movement ===============================

	if( state->key_map['z'] ) // Print kart position on 'z' key.
	{
		btScalar x_pos = state->Karts[0].vPos.x;
		btScalar y_pos = state->Karts[0].vPos.y;
		btScalar z_pos = state->Karts[0].vPos.z;

		DEBUGOUT("Pos: %f, %f, %f\n", x_pos, y_pos, z_pos);
	}

	move_all();
}

bool UseAI = 0;
bool Prev = 0;

void GameAi::move_all()
{
	int index = 0;
	//for (int index=0; i<NUM_KARTS; i++)

	bool ButtonPressed = state->key_map['m'];

	if( ButtonPressed != Prev && ButtonPressed )
		UseAI = !UseAI;

	Prev = ButtonPressed;

	avoid_obs_sqr(0,false);

	if (UseAI)
		GameAi::move_kart(index);
}

float stuck_counter = 0;
bool stuck = 0;
bool resetBool = 0;

void GameAi::move_kart(int index)
{
	Vector3 target_3 = path.at(current_state);

	Vector2 target = Vector2(target_3.x, target_3.z);
	// Calculate the updated distance and the difference in angle.

	if (state->key_map['r'] && stuck)
	{
		current_state = 0;
		stuck_counter = 0;
		stuck = false;
		if (resetBool)
			state->key_map['r'] = false;
	}

	btScalar diff_in_angles = getAngle(target, index) / PI;
	btScalar distance_to_target = sqrtf( pow((state->Karts[index].vPos.x - target.x) , 2) 
											+ pow((state->Karts[index].vPos.z - target.y),2) );

	if (distance_to_target < 1.0f) // Advance to the next target of the script. HACK
	{
		current_state++;
		if (current_state > path.size()-1)
			current_state = (current_state % path.size()) + 6;
	}

	// check if car stuck, increase stuck timer if stuck.
	stuck = (abs(state->Karts[index].vOldPos.x - state->Karts[index].vPos.x) < LIMIT_FOR_STUCK && 
						abs(state->Karts[index].vOldPos.z - state->Karts[index].vPos.z) < LIMIT_FOR_STUCK);
	if (stuck)
	{
		stuck_counter += 0.1f;
	}
	
	// Generate input for car.
	drive(diff_in_angles, distance_to_target, index);

	// Check if the kart is stuck for too long, if so, reset.
	if (stuck_counter >= RESET_TRESHOLD)
	{
		GetMutState()->key_map['r'] = true;
		resetBool = true;
	}
	
	//DEBUGOUT("current stuck: %f\n", stuck_counter);
	// DEBUGOUT("angle: %f, dist: %f\n", RADTODEG(diff_in_angles), distance_to_target);

	// update old pos
	state->Karts[index].vOldPos = state->Karts[index].vPos;

}

btScalar GameAi::getAngle(Vector2 target, int index)
{
	//Position location = Position(state->Karts[index].vPos.x, state->Karts[index].vPos.z);
	btVector3 toLocation = btVector3( target.x - state->Karts[index].vPos.x, 0.f , target.y - state->Karts[index].vPos.z);
	toLocation.safeNormalize();
	//DEBUGOUT("target: %f %f\n",target.x , target.y)
	btVector3 forward = (state->Karts[index].forDirection);
	forward.setY(0.f);
	
	btVector3 zAxis = forward.cross(toLocation);
	btScalar angle = forward.angle(toLocation);

	if (zAxis.getY() < 0)
		angle = -angle;

	return angle;
}

void GameAi::drive(btScalar diff_ang, btScalar dist, int index)
{
		// Make a new event.
		m_pCurrentInput[index] = NEWEVENT(Input);

		// Copy previouse event.
		memcpy(m_pCurrentInput[index], m_pPreviousInput[index], sizeof(Events::InputEvent));

		// Change the previous event to suit situation.

		if (stuck_counter >= REVERSE_TRESHOLD)
		{
			//DEBUGOUT("DRIVE BACKWARDS!\n");
			m_pCurrentInput[index]->leftTrigger = 1;
			m_pCurrentInput[index]->rightTrigger = 0;
			btScalar turn_value = diff_ang ;
	
			if (diff_ang > 0)
				turn_value = -1.f;
			else
				turn_value = 1.f;

			m_pCurrentInput[index]->leftThumbStickRL = turn_value;

			stuck_counter -= 0.005f;
		}
		else
		{
			//DEBUGOUT("DRIVE FORWARD!!\n");
			
			m_pCurrentInput[index]->rightTrigger = 0.8;
			m_pCurrentInput[index]->leftTrigger = 0;

			btScalar turn_value = diff_ang ;
	
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

			m_pCurrentInput[index]->leftThumbStickRL = turn_value;

			if (!stuck)
			{
				stuck_counter = 0.f;
			}
		}

		avoid_obs_sqr(index,true);

		if (m_pCurrentInput[index]->leftThumbStickRL > 1)
			m_pCurrentInput[index]->leftThumbStickRL = 1;

		if (m_pCurrentInput[index]->leftThumbStickRL < -1)
			m_pCurrentInput[index]->leftThumbStickRL = -1;

		// Send new event
		std::vector<Events::Event *> inputEvents;
		inputEvents.push_back(m_pCurrentInput[index]);
		m_mb->sendMail(inputEvents);
	
		// Update previous input event
		memcpy(m_pPreviousInput[index], m_pCurrentInput[index], sizeof(Events::InputEvent));

		//DEBUGOUT("speed: %f, turning: %f\n", m_pCurrentInput->rightTrigger, m_pCurrentInput->leftThumbStickRL);

		// Now mailbox owns the object
		m_pCurrentInput[index] = NULL;
}

Real GameAi::getElapsedTime()
{
	Real period = frame_timer.CalcSeconds();
	frame_timer.ResetClock();
	return period;
}

void GameAi::init_graph()
{
	// Init nodes;

	/* good round path.
	Vector3 n0 = Vector3(0.f, 0.f, 13.f);
	Vector3 n1 = Vector3(0.f, 0.f, 14.f);
	Vector3 n2 = Vector3(1.f, 0.f, 15.f);
	Vector3 n3 = Vector3(4.f, 0.f, 16.f);

	Vector3 n4 = Vector3(5.f,  0.f, 16.8f);
	Vector3 n5 = Vector3(13.5f,  0.f, 16.8f);
	Vector3 n6 = Vector3(17.f,  0.f, 14.0f);

	Vector3 n7 = Vector3(17.f,  0.f, -14.f);
	Vector3 n8 = Vector3(14.f,  0.f, -17.f);
	Vector3 n9 = Vector3(-14.f,  0.f, -17.f);

	Vector3 n10 = Vector3(-17.f,  0.f, -14.f);
	Vector3 n11 = Vector3(-17.f,  0.f, 14.f);
	Vector3 n12 = Vector3(-14.f,  0.f, 17.f);
	*/

	Vector3 n0 = Vector3(0.f, 0.f, 15.f);
	Vector3 n1 = Vector3(0.f, 0.f, 15.f);
	Vector3 n2 = Vector3(1.f, 0.f, 15.f);
	Vector3 n3 = Vector3(4.f, 0.f, 16.f);
	Vector3 n4 = Vector3(5.f,  0.f, 16.8f);
	Vector3 n5 = Vector3(13.5f,  0.f, 16.8f);
	Vector3 n6 = Vector3(17.f,  0.f, 14.0f);
	Vector3 n7 = Vector3(17.f,  0.f, -14.f);
	Vector3 n8 = Vector3(14.f,  0.f, -17.f);
	Vector3 n9 = Vector3(-14.f,  0.f, -17.f);
	Vector3 n10 = Vector3(-17.f,  0.f, -14.f);
	Vector3 n11 = Vector3(-17.f,  0.f, 14.f);
	Vector3 n12 = Vector3(-14.f,  0.f, 17.f);


	path.insert(std::pair<int,Vector3>(0,n0));
	path.insert(std::pair<int,Vector3>(1,n1));
	path.insert(std::pair<int,Vector3>(2,n2));
	path.insert(std::pair<int,Vector3>(3,n3));
	path.insert(std::pair<int,Vector3>(4,n4));
	path.insert(std::pair<int,Vector3>(5,n5));
	path.insert(std::pair<int,Vector3>(6,n6));
	path.insert(std::pair<int,Vector3>(7,n7));
	path.insert(std::pair<int,Vector3>(8,n8));
	path.insert(std::pair<int,Vector3>(9,n9));
	path.insert(std::pair<int,Vector3>(10,n10));
	path.insert(std::pair<int,Vector3>(11,n11));
	path.insert(std::pair<int,Vector3>(12,n12));
}

void GameAi::init_obs_sqr()
{
	// ORDER OF CORNERS MATTERS!!!!

	// Square (Vector3 centerValue, Vector3 top-left, Vector3 top-right, Vector3 bot-right, Vector3 bot-left) 
	Square sqr_1 = Square(Vector3(0,0,0), Vector3(-1,0,1), Vector3(1,0,1), Vector3(1,0,-1), Vector3(-1,0,-1)); 
	obs_sqr.insert(std::pair<int,Square>(0,sqr_1));
}

#define LENGTH_OF_RAY 3
#define SENSOR_ANGLE 5
#define IN_FRONT_ANGLE 45

void GameAi::avoid_obs_sqr(int index, bool send)
{
	std::map<int, Square> danger_sqr;
	std::vector<int> old_index;

	

	for(int i = 0; i<obs_sqr.size(); i++)
	{
		Square square = obs_sqr.at(i);

		float angle = GameAi::getAngle(Vector2(square.getCenter().x, square.getCenter().z), index);

		if (abs(angle) < IN_FRONT_ANGLE )
		{
			btVector3 left_ray = state->Karts[index].forDirection.rotate(btVector3(0,1,0), DEGTORAD(-SENSOR_ANGLE));
			btVector3 right_ray = state->Karts[index].forDirection.rotate(btVector3(0,1,0), DEGTORAD(SENSOR_ANGLE));
			
			Vector3 direction = Vector3(state->Karts[index].forDirection.getX(), 0, state->Karts[index].forDirection.getZ());
			direction = direction.Normalize();

			Vector3 left_sensor = Vector3(left_ray.getX(), 0, left_ray.getZ()).Normalize();
			Vector3 right_sensor = Vector3(right_ray.getX(), 0, right_ray.getZ()).Normalize();

			int intersections_forward = square.LineIntersectsSquare(state->Karts[index].vPos, direction, LENGTH_OF_RAY);
			int intersections_left = square.LineIntersectsSquare(state->Karts[index].vPos, left_sensor, LENGTH_OF_RAY);
			int intersections_right = square.LineIntersectsSquare(state->Karts[index].vPos, right_sensor, LENGTH_OF_RAY);

			if (intersections_forward + intersections_left + intersections_right > 0)
			{
				danger_sqr.insert(std::pair<int,Square>(i, square));
				old_index.push_back(i);
				if (intersections_forward > 0)
					DEBUGOUT("Intersects forward with %d %f times!\n" , i, intersections_forward);

				if (intersections_left > 0)
					DEBUGOUT("Intersects left with %d %f times!\n" , i, intersections_left);

				if (intersections_right > 0)
					DEBUGOUT("Intersects right with %d %f times!\n" , i, intersections_right);
			}
		}
		
	}

	int most_threat = -1;
	float most_threat_dist = 10000000.f;

	for (int i = 0; i<danger_sqr.size(); i++)
	{
		Square square = danger_sqr.at(i);
		Vector3 center = square.getCenter();

		float dist_from_center = GameAi::get_distance(state->Karts[index].vPos, center);

		if (dist_from_center < most_threat_dist)
		{
			most_threat = i;
			most_threat_dist = dist_from_center;
		}
	}

	if (most_threat != -1) // This means there's a threat that needs to be steered!
	{
		Square most_threat_sqr = danger_sqr.at(most_threat);
		Vector3 center_threat = most_threat_sqr.getCenter();
		Vector2 sphere_center = Vector2(center_threat.x, center_threat.z);

		// get angle between car and the most threatening obsticle.
		float steer_correction_angle = getAngle(sphere_center, index);

		float turn_value = 0;
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

		if (send)
			m_pCurrentInput[index]->leftThumbStickRL = turn_value;

		//float dist_to_center = abs(GameAi::get_distance(state->Karts[index].vPos, obs_sqr.at(old_index[most_threat]).getCenter()));
		DEBUGOUT("Threat index: %d", old_index[most_threat])
		DEBUGOUT("CORRECTION ANGLE WAS: %f, Turn value was: %f\n", RADTODEG(steer_correction_angle), turn_value);
	}
}

// get the distance between two points
float GameAi::get_distance(Vector3 a, Vector3 b)
{
	return sqrtf( pow( ((float)a.x - (float)b.x) , 2 ) + pow(((float)a.z - (float)b.z),2) );
}
/*
SPHERE STUFF - Not needed ?  (Still testing...)


// This is somewhat based by the line-sphere intersection article on wikipedia.
Vector3 GameAi::findIntersection(Sphere s, Vector3 rayDirection, Vector3 rayOrigin)
{
	Vector3 d = rayDirection.Normalize()/rayDirection.Length();
	Vector3 f = rayOrigin - s.getSphereCenter();
	float r = s.getSphereRadius();

	//DEBUGOUT("Dir: %f, %f, %f\n", d.x, d.y, d.z);
	//DEBUGOUT("Origin: %f, %f, %f\n", origin.getX(), dir.getY(), dir.getZ());

	float a = Vector3::Dot(d,d);
	float b = 2*(Vector3::Dot(f,d));
	float c = Vector3::Dot(f,f) - r*r;

	btScalar discriminant = b*b-4*a*c;
	bool hit = 0;
	if (discriminant < 0)
	{
		hit = 0; // Vector totally missed the circle!
	}
	else // There was at least one collision!
	{
		hit = 1;
	}
	
	Vector3 result = Vector3(0,1000,0);
	if (hit)
	{
		float new_d1 = -Vector3::Dot(d,f) + sqrt(discriminant);
		float new_d2 = -Vector3::Dot(d,f) - sqrt(discriminant);
		
		Vector3 S1 = (d * new_d1 + rayOrigin)/2;
		Vector3 S2 = (d * new_d2 + rayOrigin)/2;

		//DEBUGOUT("First intersection: %f, %f, %f\n", S1.x, S1.y, S1.z);
		//DEBUGOUT("Second intersection: %f, %f, %f\n", S2.x, S2.y, S2.z);

		float dist1 = get_distance(rayOrigin, S1);
		float dist2 = get_distance(rayOrigin, S2);

		if (dist1 < dist2)
		{
			result = S1;
			//DEBUGOUT("S1 CLOSER!")
		}
		else
		{
			result = S2;
			//DEBUGOUT("S2 CLOSER!")
		}
	}

	return result;
} 

#define CORNER_RAD 1.f
void GameAi::init_obs()
{
	// 1-2-3 obs
	Sphere s1 = Sphere(Vector3(2.71f, 1.f, 2.633f), CORNER_RAD); //0
	Sphere s2 = Sphere(Vector3(2.71f, 1.f, 5.462f), CORNER_RAD); //1
	Sphere s3 = Sphere(Vector3(5.578f, 1.f, 2.633f), CORNER_RAD); //2
	
	// 5-6-7-8 obs
	Sphere s1 = Sphere(Vector3(5.262f, 0.f, 8.2f), CORNER_RAD);
	Sphere s1_2 = Sphere(Vector3(5.262f, 0.5f, 8.7f), CORNER_RAD);
	Sphere s2 = Sphere(Vector3(5.233f, 1.f, 9.521f), CORNER_RAD);

	// 5 obs (corner)
	Sphere s3 = Sphere(Vector3(7.607f, 0.f, 7.607f), CORNER_RAD);

	// 4-10-5-9 obs
	Sphere s4 = Sphere(Vector3(8.533f, 0.f, 5.462f), CORNER_RAD);
	Sphere s5 = Sphere(Vector3(9.521f, 0.f, 5.633f), CORNER_RAD);
	
	// obs 8

	// obs 9

	obs.insert(std::pair<int,Sphere>(0,s1));
	obs.insert(std::pair<int,Sphere>(1,s2));
	//obs.insert(std::pair<int,Sphere>(2,s3));
	
}

// Shoot a cone infront of the car. Check if it intersects any obsticle spheres. If it does, steer away from sphere center.

#define SENSOR_ANGLE 15
#define SPHERE_IN_FRONT_ANGLE 25
#define DIST_CAP_OBS 3

// First part is done to optimize, no need to check against ALL spheres.
// for every sphere-obsticle 
	// if vector to sphear and forawrd vector angle < SPHERE_IN_FRONT_ANGLE 
		//if sphere center is within DIST_CAP_OBS to the kart
			// This sphere is a potential obsticle and needs to be further checked.

// This is the avoidance part. Only takes "threatening" obsticles in account.
// Lines a and b are sensors that are rotated SENSOR_ANGLE from the forward vecotr both to the left and to the right.

// for each threatening obs, find if there's intersection with line a or b.
	// keep track of it if it's distance to target is shorter then current. <- This finds the most immidiate threat.

// when got most threatening sphere, stear to correct in reversed angle from between it and car.

void GameAi::avoid_obs(int index, bool send)
{
	btVector3 forward = state->Karts[index].forDirection;
	std::vector<Sphere> in_front_spheres;
	std::vector<int> old_index;

	// check all the spheres, are they a threat?
	for (int i=0; i<obs.size(); i++)
	{
		Sphere temp_s = obs.at(i);
		Vector2 s_center = Vector2(temp_s.getSphereCenter().x, temp_s.getSphereCenter().z);
		Vector2 toSphereVect = s_center - Vector2(state->Karts[index].vPos.x, state->Karts[index].vPos.z);
	
		float angle = RADTODEG(getAngle(s_center, index));
		// Only check for collisions with that sphere if it is infront and within good distance.
		if (abs(angle) < SPHERE_IN_FRONT_ANGLE )
			if (abs(GameAi::get_distance(state->Karts[index].vPos, temp_s.getSphereCenter())) < DIST_CAP_OBS)
			{
				//DEBUGOUT("Angle to obs %d: %f\n", i,angle);
				in_front_spheres.push_back(temp_s);
				//DEBUGOUT("THREATENING OBG : %d \n" ,i);
				old_index.push_back(i);
			}
	}

	int biggest_threat_index = -1;
	float closest = 10000000.f;

	for(int i=0; i<in_front_spheres.size(); i++)
	{
		Sphere temp_s = in_front_spheres.at(i);

		btVector3 left_sensor = forward.rotate(btVector3(0,1,0), DEGTORAD(-SENSOR_ANGLE));
		btVector3 right_sensor = forward.rotate(btVector3(0,1,0), DEGTORAD(SENSOR_ANGLE));

		Vector3 source = state->Karts[index].vPos;
		Vector3 left_ray = Vector3(left_sensor.getX(), 0, left_sensor.getZ());
		Vector3 left_obs_intersec = findIntersection(temp_s, left_ray, source);

		//DEBUGOUT("LEFT OBS: %f,%f,%f\n", left_obs_intersec.x,left_obs_intersec.y,left_obs_intersec.z);

		Vector3 right_ray = Vector3(right_sensor.getX(), 0, right_sensor.getZ());
		Vector3 right_obs_intersec = findIntersection(temp_s, right_ray, source);

		//DEBUGOUT("RIGHT OBS: %f,%f,%f\n", right_obs_intersec.x, right_obs_intersec.y, right_obs_intersec.z);

		Vector3 forward_ray = Vector3(forward.getX(), 0, forward.getZ());
		Vector3 forward_obs_intersec = findIntersection(temp_s, forward_ray, source);
		//DEBUGOUT("FORWARD OBS: %f,%f,%f\n", forward_obs_intersec.x, forward_obs_intersec.y, forward_obs_intersec.z);

		if (left_obs_intersec.y != 1000)
		{
			
			float obs_dist = GameAi::get_distance(source,left_obs_intersec);
			if ( obs_dist < closest)
			{
				biggest_threat_index = i;
				closest = obs_dist;
				//DEBUGOUT("HIT LEFT OBS!")
			}
		}

		if (right_obs_intersec.y != 1000)
		{
			
			float obs_dist = GameAi::get_distance(source,right_obs_intersec);
			if ( obs_dist < closest)
			{
				
				biggest_threat_index = i;
				closest = obs_dist;
				//DEBUGOUT("HIT RIGHT OBS - WAS CLOSER THAN LEFT!\n")
			}
		}
	}
	
	if (biggest_threat_index != -1) // This means there's a threat that needs to be steered!
	{
		Sphere most_threat_sphere = in_front_spheres.at(biggest_threat_index);
		Vector3 center_threat = most_threat_sphere.getSphereCenter();
		Vector2 sphere_center = Vector2(center_threat.x, center_threat.z);

		// get angle between car and the most threatening obsticle.
		float steer_correction_angle = getAngle(sphere_center, index);

		float turn_value = 0;
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

		if (send)
			m_pCurrentInput[index]->leftThumbStickRL = turn_value;

		float dist_to_center = abs(GameAi::get_distance(state->Karts[index].vPos, obs.at(old_index[biggest_threat_index]).getSphereCenter()));
		DEBUGOUT("Threat index: %d, dist_inter %f, dist_center: %f\n", old_index[biggest_threat_index], closest, dist_to_center)
		DEBUGOUT("CORRECTION ANGLE WAS: %f, Turn value was: %f\n", RADTODEG(steer_correction_angle), turn_value);
	}
}
*/