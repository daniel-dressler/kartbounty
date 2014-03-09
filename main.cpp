// Platform Specific Code
#include "Standard.h"

// Sub-System Components
#include "component/entities/entities.h"
#include "component/gameai/GameLogic.h"
#include "component/gameai/gameai.h"
#include "component/rendering/rendering.h"
#include "component/physics/physics.h"
#include "component/input/input.h"
#include "component/state/state.h"
#ifdef _WIN32
#include "component/audio/audio.h"
#endif


int main( int argc, char** argv )
{
	// -- Platfor Specific Init -----------------------------------------------
	INIT_PLATFORM();

	// -- Init components -----------------------------------------------------
	// Register Events
	init_inventory();
	Physics::Simulation *simulation = new Physics::Simulation();
	if( !InitRendering() )
		return 1;
	Input *input = new Input();
	Audio *audio = new Audio();
	GameAi *gameai = new GameAi();
	GameLogic *logic = new GameLogic();

	// Act on events
	gameai->setup();
	simulation->loadWorld();
	input->setup();


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
	shutdown_inventory();

	return 0;
}
