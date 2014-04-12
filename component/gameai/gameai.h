#pragma once
#include <vector>

#include "../../Standard.h"
#include "../events/events.h"

class GameAi 
{
public:
	GameAi();
	~GameAi();
	void setup();
	int planFrame();
	Real getElapsedTime();
	bool gamePaused;

	struct local_powerup_to_spawn
	{
		entity_id powerup_id;
		Entities::powerup_t powerup_type;
		float timer_to_respawn;
		Vector3 pos;
	};

private:
	Events::Mailbox* m_mb;
	Timer frame_timer;
	Timer fps_timer;

	float inputPauseTimer;

	struct explodingKart {
		entity_id kart_id;
		float timer;
	};

	std::vector<explodingKart *> m_exploding_karts;
	
	enum GameStates
	{
		StartMenu = 1,
		RoundStart = 2,
		RoundInProgress = 4,
		RoundEnd = 8
	} currentState;

	Real roundStartCountdownTimer;

	std::map<entity_id, GameAi::local_powerup_to_spawn *> m_to_spawn_vec;

	std::vector<entity_id> kart_ids;
	std::vector<entity_id> ai_kart_ids;
	std::vector<entity_id> player_kart_ids;

	std::vector<Vector3> m_open_points;
	uint32_t active_powerups;
	uint32_t active_tresures;
	powerup_id_t next_powerup_id;
	powerup_id_t m_gold_case_id;

	Vector3 pick_point();
	void open_point(Vector3);
	Events::PowerupPlacementEvent *spawn_powerup(Entities::powerup_t p_type, Vector3 pos);
	void updateScoreBoard();
	void outputScoreBoard();

	void add_to_future_respawn(Events::PowerupPickupEvent *);
	void handle_powerups_not_gold(double time_elapsed);
	void init_powerups_not_gold();

	void endRound();
	void newRound(int, int);
	void spawn_a_powerup_not_gold(Vector3 pos, std::vector<Events::Event *> &events_out);
	void updateExplodingKarts();
};
