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

	SDL_Init( SDL_INIT_EVERYTHING );
//	SDL_GL_SetAttribute( SDL_GL_CONTEXT_MAJOR_VERSION, 3 );		// This breaks shit...
//	SDL_GL_SetAttribute( SDL_GL_CONTEXT_MINOR_VERSION, 3 );
	SDL_GL_SetAttribute( SDL_GL_DEPTH_SIZE, 16 );
	SDL_GL_SetAttribute( SDL_GL_DOUBLEBUFFER, 1 );
	SDL_GL_SetAttribute( SDL_GL_MULTISAMPLEBUFFERS, 1 );
	SDL_GL_SetAttribute( SDL_GL_MULTISAMPLESAMPLES, 4 );

	SDL_Window *win = SDL_CreateWindow( GAMENAME,
			SDL_WINDOWPOS_UNDEFINED,
			SDL_WINDOWPOS_UNDEFINED,
			1280, 720, SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE
//			| SDL_WINDOW_FULLSCREEN
			);
	if( !win )
		return 1;

	SDL_GLContext glcontext = SDL_GL_CreateContext( win );
	if( !glcontext )
		return 1;

	SDL_GL_MakeCurrent( win, glcontext );

	if( !InitRendering( win ) )
		return 1;

	GetState().Karts[0].vPos = Vector3( 0, 0.5f, 0 );
	GetState().Karts[0].qOrient.Identity();

	GetState().Camera.vFocus = Vector3( 0, 1, 0 );
	GetState().Camera.vPos = GetState().Camera.vPos + Vector3( 0, 5, 5 );
	
	Events::Mailbox* pMailbox = new Events::Mailbox();
	pMailbox->request( Events::EventType::Quit );
	std::vector<Events::Event*> aryEvents;
	aryEvents.push_back( NEWEVENT(StateUpdate) );
	pMailbox->sendMail( aryEvents );
	
	Timer timer;
	int bRunning = 1;

	// Init components
	Physics::Simulation *simulation = new Physics::Simulation();
	simulation->loadWorld();

	// Init Input control and the player 1 joystick if plugged in.
	Input *input = new Input();

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
		//GetState().Karts[0].qOrient = Quaternion::GetRotateAxisAngle( Vector3( 0, 1, 0 ), fTime );

		glClearColor( 0, 0, 0, 1 );
		glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );


		UpdateRendering( fElapse );
		Render();

		fLastTime = fTime;
		std::chrono::milliseconds timespan(10); // oswhatever
		std::this_thread::sleep_for(timespan);

		const std::vector<Events::Event*> aryEvents = pMailbox->checkMail();
		for( unsigned int i = 0; i < aryEvents.size(); i++ )
		{
			switch( aryEvents[i]->type )
			{
			case Events::EventType::Quit:
				bRunning = 0;
				break;
			}
		}
	}
	
	delete pMailbox;
	ShutdownRendering();
	SDL_DestroyWindow( win );
	SDL_Quit();

	return 0;
}
