#pragma once
#include <vector>

#include "../../Standard.h"
#include "../events/events.h"
#include "../state/state.h"

class GameAi 
{
public:
	GameAi();
	~GameAi();
	void setup();
	int planFrame();
	Real getElapsedTime();
	bool gamePaused;

private:
	Events::Mailbox* m_mb;
	Timer frame_timer;
	Timer fps_timer;

	std::vector<entity_id> kart_ids;

	entity_id player1KartId;

	std::vector<Vector3> m_open_points;
	uint32_t active_powerups;
	uint32_t active_tresures;
	powerup_id_t next_powerup_id;

	Vector3 pick_point();
	void open_point(Vector3);
	Events::PowerupPlacementEvent *spawn_powerup(Entities::powerup_t);
	void updateScoreBoard();
	void outputScoreBoard();

	void resetGame();
};
