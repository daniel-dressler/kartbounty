// Platform Specific Code
#include "Standard.h"

// Sub-System Components
#include "component/entities/entities.h"
#include "component/gameai/GameLogic.h"
#include "component/gameai/gameai.h"
#include "component/enemyai/enemyai.h"
#include "component/rendering/rendering.h"
#include "component/physics/physics.h"
#include "component/input/input.h"
#include "component/state/state.h"
#ifdef _WIN32
#include "component/audio/audio.h"
#endif

#define PLAYAUDIO 1

int main( int argc, char** argv )
{
	// -- Platfor Specific Init -----------------------------------------------
	INIT_PLATFORM();

	// -- Init components -----------------------------------------------------
	// Register Events
	Renderer *renderer = new Renderer();
	init_inventory();
	Physics::Simulation *simulation = new Physics::Simulation();
	Input *input = new Input();
#if (PLAYAUDIO)
	Audio *audio = new Audio();  
#endif

	GameAi *gameai = new GameAi();
	EnemyAi *enemyai = new EnemyAi();

	// Act on events
	gameai->setup();
	renderer->setup();
	simulation->loadWorld();
	input->setup();
	enemyai->setup();
#if (PLAYAUDIO)
	audio->setup();
#endif


	// -- Main Loop -----------------------------------------------------------
	while (gameai->planFrame())
	{
		Real elapsed_time = gameai->getElapsedTime();

		input->HandleEvents();
		enemyai->update(elapsed_time);

		simulation->step(elapsed_time);
#if (PLAYAUDIO)
		audio->update(elapsed_time);
#endif
		renderer->update(elapsed_time);
		renderer->render();
	}
	
	// -- Cleanup & Exit ------------------------------------------------------
	delete enemyai;
	delete input;
	delete simulation;
	delete gameai;
#if (PLAYAUDIO)
	delete audio;
#endif
	delete renderer;
	shutdown_inventory();

	return 0;
}
