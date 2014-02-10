// Platform Specific Code
#include "Standard.h"

// Sub-System Components
#include "component/gameai/gameai.h"
#include "component/rendering/rendering.h"
#include "component/physics/physics.h"
#include "component/input/input.h"

int main( int argc, char** argv )
{
	// -- Platfor Specific Init -----------------------------------------------
	INIT_PLATFORM();

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
	Input *input = new Input();

	// -- Main Loop -----------------------------------------------------------
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
