#pragma once
#include "../events/events.h"
#include "../../Standard.h"

class GameAi {
public:
	GameAi();
	~GameAi();
	int planFrame();
	Real getElapsedTime();

private:
	Events::Mailbox m_mb;
	Timer frame_timer;
	Timer fps_timer;
};
