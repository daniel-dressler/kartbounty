#include "Standard.h"
#include "component/rendering/glhelpers.h"
#include <SDL.h>

#include "component/gameai/gameai.h"
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


	// -- Init components -----------------------------------------------------
	// Rendering
	if( !InitRendering() )
		return 1;

	// GameAi
	GameAi *gameai = new GameAi();

	// Physics
	Physics::Simulation *simulation = new Physics::Simulation();
	simulation->loadWorld();

	// Input
	// Player 1 requires joystick
	Input *input = new Input();


	// -- Main Loop -----------------------------------------------------------
	Timer timer;
	while (gameai->planFrame())
	{
		Real elapsed_time = gameai->getElapsedTime();

		input->HandleEvents();
		simulation->step(elapsed_time);
		UpdateRendering(elapsed_time);
		Render();
	}

	
	// -- Cleanup & Exit ------------------------------------------------------
	delete input;
	delete simulation;
	delete gameai;
	ShutdownRendering();

	SDL_Quit();
	return 0;
}
