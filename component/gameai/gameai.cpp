#include "gameai.h"

GameAi::GameAi()
{
	m_mb.request( Events::EventType::Quit );
}

GameAi::~GameAi()
{

}

int GameAi::planFrame()
{
	const std::vector<Events::Event*> events = m_mb.checkMail();
	for (Events::Event *event : events) {
		switch( event->type ) {
		case Events::EventType::Quit:
			return 0;
			break;
		default:
			break;
		}
	}
	m_mb.emptyMail();
}
