#include "Standard.h"
#include "component/rendering/glhelpers.h"
#include <SDL.h>

#include <thread>
#include <chrono>

#include "component/rendering/rendering.h"
#include "component/entities/entities.h"
#include "component/events/events.h"
#include "component/physics/physics.h"
#include "component/input/input.h"

int main( int argc, char** argv )
{
#ifdef _WIN32
#ifdef _DEBUG
	_CrtSetDbgFlag( _CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF );
#endif
#endif

	if( !InitRendering() )
		return 1;

	Events::Mailbox* pMailbox = new Events::Mailbox();
	pMailbox->request( Events::EventType::Quit );
	std::vector<Events::Event*> aryEvents;
	aryEvents.push_back( NEWEVENT(StateUpdate) );
	pMailbox->sendMail( aryEvents );
	
	Timer timer;
	int bRunning = 1;

	// -- Init components -----------------------------------------------------
	// Physics
	Physics::Simulation *simulation = new Physics::Simulation();
	simulation->loadWorld();

	// Input
	// Player 1 requires joystick
	Input *input = new Input();


	// -- Main Loop -----------------------------------------------------------
	while( bRunning )
	{
		static Real fLastTime = 0;
		static Real fLastPhysTime = 0;
		Real fTime, fElapse;

		fTime = (Real)timer.CalcSeconds();
		fElapse = fTime - fLastTime;

		input->HandleEvents();

		static Int32 nFPS = 0;
		nFPS++;
		if( (Int32)fTime != (Int32)fLastTime )
		{
			//DEBUGOUT( "FPS: %d\n", nFPS );
			nFPS = 0;
		}

		// Components
		if( fTime - fLastPhysTime > 0.01f )
		{
			simulation->step( fTime - fLastPhysTime );
			fLastPhysTime = fTime;
		}

		GetState().fTime = fTime;
		GetState().fElapse = fElapse;

		glClearColor( 0, 0, 0, 1 );
		glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );


		UpdateRendering( fElapse );
		Render();

		fLastTime = fTime;
		std::chrono::milliseconds timespan(10);
		std::this_thread::sleep_for(timespan);

		const std::vector<Events::Event*> aryEvents = pMailbox->checkMail();
		for( unsigned int i = 0; i < aryEvents.size(); i++ )
		{
			switch( aryEvents[i]->type )
			{
			case Events::EventType::Quit:
				bRunning = 0;
				break;
			default:
				break;
			}
		}
	}

	
	// -- Cleanup & Exit ------------------------------------------------------
	delete input;
	delete simulation;
	delete pMailbox;
	ShutdownRendering();

	SDL_Quit();
	return 0;
}
