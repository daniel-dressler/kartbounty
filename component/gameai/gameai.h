#pragma once
#include <vector>

#include "../../Standard.h"
#include "../events/events.h"
#include "../state/state.h"

class GameAi 
{
public:
	GameAi();
	~GameAi();
	void setup();
	int planFrame();
	Real getElapsedTime();

private:
	Events::Mailbox* m_mb;
	Timer frame_timer;
	Timer fps_timer;

	std::vector<entity_id> kart_ids;
};
