#pragma once
#include "../events/events.h"

class GameAi {
public:
	GameAi();
	~GameAi();
	int planFrame();

private:
	Events::Mailbox m_mb;
};
