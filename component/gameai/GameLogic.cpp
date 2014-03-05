#include <stdlib.h>
#include <math.h>
#include <time.h>

#include <chrono>
#include <thread>

#include "GameLogic.h"

#define NUM_OF_PICKUP_POINTS 5

// Is there a pickup on a certain spot
bool is_point_occupied[NUM_OF_PICKUP_POINTS];

GameLogic::GameLogic()
{
	state = GetMutState();
	m_mb = new Events::Mailbox();	
	add_pickup_points();
	reset_pickup_event();

	for (int i=0; i<MAX_POWERUPS; i++)
		respawn_pickup(i);

	// seed random
	srand((unsigned int)time(NULL));
}

void GameLogic::add_pickup_points()
{
	Vector3 v1 = Vector3(0.f, 0.f, 5.5f);
	Vector3 v2 = Vector3(1.f, 0.f, 5.5f);
	Vector3 v3 = Vector3(-1.f, 0.f, 5.5f);
	Vector3 v4 = Vector3(3.22f, 0.f, 7.59f);
	Vector3 v5 = Vector3(-3.22f, 0.f, 7.59f);

	pickup_locations.insert( std::pair <int, Vector3> (0, v1));
	pickup_locations.insert( std::pair <int, Vector3> (1, v2));
	pickup_locations.insert( std::pair <int, Vector3> (2, v3));
	pickup_locations.insert( std::pair <int, Vector3> (3, v4));
	pickup_locations.insert( std::pair <int, Vector3> (4, v5));
}

void GameLogic::modify_pickup_event(int kart_index, int power_up_index)
{
	m_pPickUpEvent->picker_kart_index[power_up_index] = kart_index;
}

void GameLogic::reset_pickup_event()
{
	m_pPickUpEvent = NEWEVENT(PowerupPickup);
	for (int powerUpIndex = 0; powerUpIndex<MAX_POWERUPS; powerUpIndex++)
	{
		modify_pickup_event(powerUpIndex,-1);
	}
}

Real elapsed_time;
void GameLogic::update(Real delta_time)
{
	elapsed_time = delta_time;

	//if (state->Powerups[i].bEnabled);
		//state->Powerups[i].vPos = Vector3(0,0,5.5f);

	for (int i=0 ; i<MAX_POWERUPS; i++)
	{
		GameLogic::handle_powerup(i);
	}
}

#define PICKUP_DIST 0.5f
#define TIME_TO_RESPAWN_PICKUP 5

Real powerUpDownTimer[NUM_KARTS];

void GameLogic::handle_powerup(int power_up_index)
{
	Vector3 power_up_location = state->Powerups[power_up_index].vPos;
	
	bool any_pickup_picked = false;
	for (int kart_index =0; kart_index<NUM_KARTS; kart_index++)
	{
		Vector3 kart_loc = state->Karts[kart_index].vPos;
	
		if ((get_distance(power_up_location, kart_loc) < PICKUP_DIST) && (state->Powerups[power_up_index].bEnabled))
		{
			any_pickup_picked = true;
			state->Powerups[power_up_index].bEnabled = false;
			//DEBUGOUT("PICKED UP!");

			is_point_occupied[state->Powerups[power_up_index].pointId] = false;

			GameLogic::modify_pickup_event(kart_index, power_up_index);
		}
	}

	if(any_pickup_picked)
	{
		send_power_up_picked_event();
	}

	// If the powerup is down, increase it's timer.
	if (!state->Powerups[power_up_index].bEnabled)
	{
		powerUpDownTimer[power_up_index] += elapsed_time;

		if (powerUpDownTimer[power_up_index] > TIME_TO_RESPAWN_PICKUP)
		{
			respawn_pickup(power_up_index);
			powerUpDownTimer[power_up_index] = 0;
		}
	}
}

// get the distance between two points
double GameLogic::get_distance(Vector3 a, Vector3 b)
{
	return sqrtf( pow( ((float)a.x - (float)b.x) , 2 ) + pow(((float)a.z - (float)b.z),2) );
}

void GameLogic::respawn_pickup(int power_up_index)
{
	int rand = (std::rand() % pickup_locations.size());

	// Don't let the pickup respawn at the same place...
	while (is_point_occupied[rand])
		rand = (std::rand() % pickup_locations.size());

	is_point_occupied[rand] = true;

	//DEBUGOUT("Pickup Loc id: %d, Posiion : %f, %f \n", rand, pickup_locations.at(rand).x, pickup_locations.at(rand).z);

	//DEBUGOUT("%d" , power_up_index);
	state->Powerups[power_up_index].bEnabled = true;
	state->Powerups[power_up_index].vPos = pickup_locations.at(rand);
	state->Powerups[power_up_index].pointId = rand;
}

void GameLogic::send_power_up_picked_event()
{
	std::vector<Events::Event *> inputEvents;
	inputEvents.push_back(m_pPickUpEvent);
	m_mb->sendMail(inputEvents);

	reset_pickup_event();
}
