#pragma once
#include <map>

#include "../enemyai/graph/Graph.h"
#include "../state/state.h"
#include "../events/events.h"

class GameLogic 
{
	public:
		GameLogic();
		void update(Real elapsed_time);

	private:
		Events::Mailbox *m_mb;	
		Events::PowerupPickupEvent *m_pPickUpEvent;
		StateData *state;
		std::map<int, Vector3> pickup_locations;

		void add_pickup_points();
		void modify_pickup_event(int kart_index, int power_up_index);
		void reset_pickup_event();
		void handle_powerup(int i);
		double get_distance(Vector3 a, Vector3 b);
		void respawn_pickup(int power_up_index);
		void send_power_up_picked_event();
};
