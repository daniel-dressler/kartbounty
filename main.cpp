// Platform Specific Code
#include "Standard.h"

// Sub-System Components
#include "component/gameai/gameLogic.h"
#include "component/gameai/gameai.h"
#include "component/rendering/rendering.h"
#include "component/physics/physics.h"
#include "component/input/input.h"
#include "component/state/state.h"
#include "component/audio/audio.h"


int main( int argc, char** argv )
{
	// -- Platfor Specific Init -----------------------------------------------
	INIT_PLATFORM();

	// -- Init components -----------------------------------------------------
	// Rendering
	if( !InitRendering() )
		return 1;

	// Physics
	Physics::Simulation *simulation = new Physics::Simulation();
	simulation->loadWorld();

	// Input
	Input *input = new Input();
	// Audio
	Audio *audio = new Audio();

	// GameAi
	GameAi *gameai = new GameAi();
	// Logic
	GameLogic *logic = new GameLogic();

	// -- Main Loop -----------------------------------------------------------
	while (gameai->planFrame())
	{
		Real elapsed_time = gameai->getElapsedTime();
		logic->update(elapsed_time);
		input->HandleEvents();
		gameai->update(elapsed_time);
		simulation->step(elapsed_time);
		audio->Update(elapsed_time);
		UpdateRendering(elapsed_time);
		Render();
	}
	
	// -- Cleanup & Exit ------------------------------------------------------
	delete input;
	delete simulation;
	delete gameai;
	delete audio;
	ShutdownRendering();

	SDL_Quit();
	return 0;
}
