// Platform Specific Code
#include "Standard.h"

// Sub-System Components
#include "component/entities/entities.h"
#include "component/gameai/gameai.h"
#include "component/enemyai/enemyai.h"
#include "component/rendering/rendering.h"
#include "component/physics/physics.h"
#include "component/input/input.h"
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
	Audio *audio = new Audio();  

	GameAi *gameai = new GameAi();
	EnemyAi *enemyai = new EnemyAi();

	// Act on events
	gameai->setup();
	renderer->setup();
	simulation->loadWorld();
	input->setup();
	enemyai->setup();
	audio->setup();


	// -- Main Loop -----------------------------------------------------------
	while (gameai->planFrame())
	{
		Real elapsed_time = gameai->getElapsedTime();

		input->HandleEvents();
		enemyai->update(elapsed_time);

		simulation->step(elapsed_time);

		audio->update(elapsed_time);
		renderer->render(elapsed_time);
	}

	// -- Cleanup & Exit ------------------------------------------------------
	delete enemyai;
	delete input;
	delete simulation;
	delete gameai;
	delete audio;
	delete renderer;
	shutdown_inventory();

	return 0;
}
