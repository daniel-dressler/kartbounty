#pragma once
#include "../events/events.h"
#include "../../Standard.h"
#include <vector>

class GameAi 
{
	public:
		GameAi();
		~GameAi();
		int planFrame();
		Real getElapsedTime();
		void update(Real elapsed_time);

	private:
		Events::Mailbox* m_mb;
		Timer frame_timer;
		Timer fps_timer;
		StateData *state;

		// The event that will be sent to the physics.
		Events::InputEvent *m_pCurrentInput[NUM_KARTS];
		Events::InputEvent *m_pPreviousInput[NUM_KARTS];

		void GameAi::move_all();
		void GameAi::move_kart(int index);
		void GameAi::drive(btScalar ang, btScalar dist, int index);
		void GameAi::avoid_obs(btScalar diff_ang, btScalar dist, int index);
	
		void GameAi::init_graph();
		btScalar GameAi::getAngle(Vector2 target, int index);
};
