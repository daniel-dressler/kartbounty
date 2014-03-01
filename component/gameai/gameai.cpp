#include "gameai.h"
#include <chrono>
#include <thread>

#define LIMIT_FOR_STUCK 0.004f
#define REVERSE_TRESHOLD 10
#define RESET_TRESHOLD REVERSE_TRESHOLD*4

// temp
int current_state;
std::map<int, Vector3> path;

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
	}

	// Scripted path while there's no "brain". HACK. (will be replaced soon)
	init_graph();
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
	// Calculate the updated distance and the difference in angle.
	StateData *state = GetMutState();
	
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

// Shoot a cone infront of the car. Check if it intersects any obsticle spheres. If it does, steer away from sphere center.

#define CONE_ANGLE 15

void GameAi::avoid_obs(btScalar diff_ang, btScalar dist, int index)
{
	btVector3 forward = state->Karts[index].forDirection;
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

		avoid_obs(diff_ang, dist, index);

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

	Vector3 n0 = Vector3(0.f, 0.f, 3.f);
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


