#pragma once
#include "../events/events.h"
#include "../../Standard.h"
#include <vector>
#include "util/Sphere.h"

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
	Vector3 think_of_target(int index);
	void GameAi::move_kart(int index);
	void GameAi::drive(btScalar ang, btScalar dist, int index);
	void GameAi::avoid_obs(int index, bool send);
	void avoid_obs_sqr(int index, bool send);
	
	void GameAi::init_graph();
	void GameAi::init_obs();
	void GameAi::init_obs_sqr();
	btScalar GameAi::getAngle(Vector2 target, int index);

	float GameAi::get_distance(Vector3 a, Vector3 b);
	Vector3 findIntersection(Sphere s, Vector3 rayDirection, Vector3 rayOrigin);
};
