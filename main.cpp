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

	MEMSET( &GetState(), 0, sizeof(StateData) );
	GetState().Karts[0].vPos = Vector3( 0, 0.5f, 0 );
	GetState().Karts[0].qOrient.Identity();

	Events::Mailbox* pMailbox = new Events::Mailbox();
	std::vector<Events::Event*> aryEvents;
	aryEvents.push_back( new Events::Event( Events::EventType::StateUpdate ) );
	pMailbox->sendMail( aryEvents );												// @Daniel: This is breaking it.

	Timer timer;
	int bRunning = 1;
	SDL_Event event;

	// Init components
	Physics::Simulation *simulation = new Physics::Simulation();
	simulation->loadWorld();

	// Init Input control and the player 1 joystick if plugged in.
	Input *input = new Input();
	DEBUGOUT("%i joysticks found.\n", SDL_NumJoysticks() );
	if(SDL_NumJoysticks())
	{
		input->OpenJoysticks();
	}

	while( bRunning )
	{
		static Real fLastTime = 0;
		Real fTime, fElapse;

		// Daniel: Caused infinite loop on linux
		// @Phil: Does windows need multiple pollings?
		// The SDL_PollEvent should retreive
		// the queued input so mutliple checks 
		// of the poll might just complicate our
		// input architecture.
		// do {
			fTime = (Real)timer.CalcSeconds();
			fElapse = fTime - fLastTime;

			input->HandleEvents();

		//} while( fElapse < 0.008f );
		
		static Int32 nFPS = 0;
		nFPS++;
		if( (Int32)fTime != (Int32)fLastTime )
		{
			DEBUGOUT( "FPS: %d\n", nFPS );
			nFPS = 0;
		}
		
		glClearColor( 0, 0, 0, 1 );
		glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );

		// Components
		simulation->step(fElapse);

		UpdateRendering( fElapse );
		Render();

		fLastTime = fTime;
		std::chrono::milliseconds timespan(10); // oswhatever
		std::this_thread::sleep_for(timespan);
	}

	delete pMailbox;
	ShutdownRendering();
	SDL_DestroyWindow( win );
	SDL_Quit();

	return 0;
}
