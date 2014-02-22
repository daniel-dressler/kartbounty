// Platform Specific Code
#include "Standard.h"

// Sub-System Components
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
	// GameAi
	GameAi *gameai = new GameAi();
	// Physics
	Physics::Simulation *simulation = new Physics::Simulation();
	simulation->loadWorld();
	// Input
	Input *input = new Input();
	// Audio
	Audio *audio = new Audio();

	//Mix_Init( MIX_INIT_FLAC | MIX_INIT_MP3 );

	GetState().Camera.oldVec = btVector3(0.f, 0.f, 0.f);

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
	delete audio;
	ShutdownRendering();

	SDL_Quit();
	return 0;
}
