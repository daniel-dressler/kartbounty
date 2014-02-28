#include <chrono>
#include <thread>

#include "gameai.h"
#include "../state/state.h"

#define LIMIT_FOR_STUCK 0.004f
#define REVERSE_TRESHOLD 10
#define RESET_TRESHOLD REVERSE_TRESHOLD*4

GameAi::GameAi()
{
	m_mb = new Events::Mailbox();	
	m_mb->request( Events::EventType::Quit );

	// Setup previous input event struct
	m_pPreviousInput = NEWEVENT(Input);
	m_pPreviousInput->rightTrigger = 0;
	m_pPreviousInput->leftTrigger = 0;
	m_pPreviousInput->leftThumbStickRL = 0;
	m_pPreviousInput->rightThumbStickRL = 0;
	m_pPreviousInput->aPressed = false;
	m_pPreviousInput->bPressed = false;
	m_pPreviousInput->xPressed = false;
	m_pPreviousInput->yPressed = false;

	// Scripted path while there's no "brain". HACK. (will be replaced soon)
	init_graph();

	path[0] = 1;
	path[1] = 2;
	path[2] = 3;
	path[3] = 4;
	path[4] = 5;
	path[5] = 6;
	path[6] = 7;
	path[7] = 8;
	path[8] = 9;
	path[9] = 10;
	path[10] = 11;
	path[11] = 12;

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

void GameAi::update()
{
	// =========== AI movement ===============================

	if( GetState().key_map['z'] ) // Print kart position on 'z' key.
	{
		StateData *state = GetMutState();
		btScalar x_pos = state->Karts[0].vPos.x;
		btScalar y_pos = state->Karts[0].vPos.z;

		DEBUGOUT("Pos: %f, %f\n", x_pos, y_pos);
	}

	// think_what_to_do for all carts

	// Find needed information from state

	// if is close enough, go directly
	// else use the graph to construct a path
	
	// if path is empty, go to the direct point
	// else get next point in path, go there
	move_all();
}

bool UseAI = 0;
bool Prev = 0;

void GameAi::move_all()
{
	int index = 0;
	//for (int index=0; i<NUM_KARTS; i++)
	StateData *state = GetMutState();

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
	int target_node_id = path[current_state];
	//DEBUGOUT("curr_state %d , node_id %d\n", current_state, target_node_id);
	Node temp_node = graph.getNode(target_node_id);

	Position target = Position (temp_node.getPosX(), temp_node.getPosY()) ;

	// Calculate the updated distance and the difference in angle.
	StateData *state = GetMutState();

	if (state->key_map['r'] && stuck)
	{
		current_state = 0;
		stuck_counter = 0;
		stuck = false;
		if (resetBool)
			state->key_map['r'] = false;
	}

	btScalar diff_in_angles = getAngle(target, index) / PI;
	btScalar distance_to_target = sqrtf( abs( pow( (state->Karts[index].vPos.x - target.posX) , 2 ) + pow((state->Karts[index].vPos.z - target.posY),2) ) );

	if (distance_to_target < 1.0f) // Advance to the next target of the script. HACK
	{
		current_state++;
		if (current_state > 11)
			current_state = (current_state % 12) + 5;
	}

	stuck = (abs(state->Karts[index].vOldPos.x - state->Karts[index].vPos.x) < LIMIT_FOR_STUCK && 
						abs(state->Karts[index].vOldPos.z - state->Karts[index].vPos.z) < LIMIT_FOR_STUCK);
	// Supply this info to the driving input generator if the car is not stuck!
	if (stuck)
	{
		stuck_counter += 0.1f;
	}
	
	drive(diff_in_angles, distance_to_target);

	if (stuck_counter >= RESET_TRESHOLD)
	{
		GetMutState()->key_map['r'] = true;
		resetBool = true;
	}
	
	DEBUGOUT("current stuck: %f\n", stuck_counter);

	// update old pos
	state->Karts[index].vOldPos = state->Karts[index].vPos;
	DEBUGOUT("angle: %f, dist: %f\n", RADTODEG(diff_in_angles), distance_to_target);
}

btScalar GameAi::getAngle(Position target, int index)
{
	// Calculate the updated distance and the difference in angle.
	StateData *state = GetMutState();
	
	//Position location = Position(state->Karts[index].vPos.x, state->Karts[index].vPos.z);
	
	btVector3 toLocation = btVector3( target.posX - state->Karts[index].vPos.x, 0.f , target.posY - state->Karts[index].vPos.z);
	toLocation.safeNormalize();
	DEBUGOUT("target: %f %f\n",target.posX , target.posY)
	btVector3 forward = (state->Karts[index].forDirection);
	forward.setY(0.f);
	
	btVector3 zAxis = forward.cross(toLocation);
	btScalar angle = forward.angle(toLocation);

	if (zAxis.getY() < 0)
		angle = -angle;

	return angle;
}



void GameAi::drive(btScalar diff_ang, btScalar dist)
{
		// Make a new event.
		m_pCurrentInput = NEWEVENT(Input);

		// Copy previouse event.
		memcpy(m_pCurrentInput, m_pPreviousInput, sizeof(Events::InputEvent));

		// Change the previous event to suit situation.

		if (stuck_counter >= REVERSE_TRESHOLD)
		{
			DEBUGOUT("DRIVE BACKWARDS!\n");
			m_pCurrentInput->leftTrigger = 1;
			m_pCurrentInput->rightTrigger = 0;
			btScalar turn_value = diff_ang ;
	
			if (diff_ang > 0)
				turn_value = -1.f;
			else
				turn_value = 1.f;

			m_pCurrentInput->leftThumbStickRL = turn_value;

			stuck_counter -= 0.005f;
		}
		else
		{
			DEBUGOUT("DRIVE FORWARD!!\n");
			
			m_pCurrentInput->rightTrigger = 0.8;
			m_pCurrentInput->leftTrigger = 0;

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

			m_pCurrentInput->leftThumbStickRL = turn_value;

			if (!stuck)
			{
				stuck_counter = 0.f;
			}

			//DEBUGOUT("turn value: %f\n",turn_value);
		}


		if (m_pCurrentInput->leftThumbStickRL > 1)
			m_pCurrentInput->leftThumbStickRL = 1;

		if (m_pCurrentInput->leftThumbStickRL < -1)
			m_pCurrentInput->leftThumbStickRL = -1;

		// Send new event
		std::vector<Events::Event *> inputEvents;
		inputEvents.push_back(m_pCurrentInput);
		m_mb->sendMail(inputEvents);
	
		// Update previous input event
		memcpy(m_pPreviousInput, m_pCurrentInput, sizeof(Events::InputEvent));

		//DEBUGOUT("speed: %f, turning: %f\n", m_pCurrentInput->rightTrigger, m_pCurrentInput->leftThumbStickRL);

		// Now mailbox owns the object
		m_pCurrentInput = NULL;
}

void GameAi::drive_backwards(btScalar diff_ang, btScalar dist)
{
		// Make a new event.
		m_pCurrentInput = NEWEVENT(Input);

		// Copy previouse event.
		memcpy(m_pCurrentInput, m_pPreviousInput, sizeof(Events::InputEvent));

		// Change the previous event to suit situation.

		

		// Send new event
		std::vector<Events::Event *> inputEvents;
		inputEvents.push_back(m_pCurrentInput);
		m_mb->sendMail(inputEvents);
	
		// Update previous input event
		memcpy(m_pPreviousInput, m_pCurrentInput, sizeof(Events::InputEvent));

		//DEBUGOUT("speed: %f, turning: %f\n", m_pCurrentInput->leftTrigger, m_pCurrentInput->leftThumbStickRL);

		// Now mailbox owns the object
		m_pCurrentInput = NULL;
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
	//Node n0 = Node(0, 0.f, 0.f);

	Node n1 = Node(1, 0.f, 14.f);
	Node n2 = Node(2, 1.f, 15.f);
	Node n3 = Node(3, 4.f, 16.f);

	Node n4 = Node(4, 5.f, 16.8f);
	Node n5 = Node(5, 13.5f, 16.8f);
	Node n6 = Node(6, 17.f, 14.0f);

	Node n7 = Node(7, 17.f, -14.f);
	Node n8 = Node(8, 14.f, -17.f);
	Node n9 = Node(9, -14.f, -17.f);

	Node n10 = Node(10, -17.f, -14.f);
	Node n11 = Node(11, -17.f, 14.f);
	Node n12 = Node(12, -14.f, 17.f);

	graph.addNode(n1);
	graph.addNode(n2);
	graph.addNode(n3);
	graph.addNode(n4);
	graph.addNode(n5);
	graph.addNode(n6);
	graph.addNode(n7);
	graph.addNode(n8);
	graph.addNode(n9);
	graph.addNode(n10);
	graph.addNode(n11);
	graph.addNode(n12);

	graph.addEdge(n1, n2);
	graph.addEdge(n2, n3);
	graph.addEdge(n3, n4);
	graph.addEdge(n4, n5);
	graph.addEdge(n5, n6);
	graph.addEdge(n6, n7);
	graph.addEdge(n7, n8);
	graph.addEdge(n8, n9);
	graph.addEdge(n9, n10);
	graph.addEdge(n10, n11);
	graph.addEdge(n11, n12);
	graph.addEdge(n12, n1);
}


