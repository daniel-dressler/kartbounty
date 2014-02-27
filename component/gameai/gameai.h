#pragma once
#include "../events/events.h"
#include "../../Standard.h"
#include "util/Position.h"
#include "graph/Graph.h"
#include <vector>

class GameAi {
public:
	GameAi();
	~GameAi();
	int planFrame();
	Real getElapsedTime();
	void update();

private:
	Events::Mailbox* m_mb;
	Timer frame_timer;
	Timer fps_timer;

	// temp
	int current_state;
	int path[12];

	// the graph of intersections.
	Graph graph;

	// The event that will be sent to the physics.
	Events::InputEvent *m_pCurrentInput;
	Events::InputEvent *m_pPreviousInput;

	void GameAi::drive(btScalar ang, btScalar dist);
	void GameAi::drive_backwards(btScalar diff_ang, btScalar dist);
	void GameAi::move_kart(int index);
	void GameAi::move_all();

	void GameAi::init_graph();

	btScalar GameAi::getAngle(Position target, int index);
};
