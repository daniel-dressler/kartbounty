#include <chrono>
#include <thread>

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
		DEBUGOUT( "FPS: %d\n", frames);
		frames = 0;
	}
	timeAtLastFrame = fps_timer.CalcSeconds();

	return 1;
}

Real GameAi::getElapsedTime()
{
	Real period = frame_timer.CalcSeconds();
	frame_timer.ResetClock();
	return period;
}
