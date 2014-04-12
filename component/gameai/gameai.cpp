#include <chrono>
#include <thread>
#include <math.h>

#include "gameai.h"
#include "../entities/entities.h"

// Score to win
#define FINAL_SCORE_GOAL 9
// How much health to substruct on bullet hit
#define DAMAGE_FROM_BULLET 1
// Number of karts
#define NUM_KARTS 8
// Big Gold Powerup
#define BIG_GOLD_VALUE 1
// Small Gold Powerup
#define SMALL_GOLD_VALUE 1
// Amount of gold for killing another kart
#define KART_KILL_GOLD_VALUE 2
// How much health does a kart has to start with
#define STARTING_HEALTH 1
#define HEALTH_POWERUP_AMOUNT 5
// timer to spawn powerups that aren't gold, in seconds
#define TIME_TO_SPAWN_POWERUPS 3
// time to stop listening to input events at end of round
#define TIME_TO_IGNORE_INPUT 0.5
// time to wait before resetting kart to show explosion
#define TIME_TO_EXPLODE 1.5

const Vector3 goldSpawnLocations[] = { Vector3(0,1.1, 0), Vector3(10, 1.1, -10), Vector3(-10, 1.1, 10), Vector3(17,2.1,0), Vector3(-17,2.1,0),
	Vector3(10,1.1,10), Vector3(-10,1.1,-10), Vector3(0,2.1,17), Vector3(0,2.1,-17) };
int goldSpawnCounter;
int goldSpawnPointCount;

GameAi::GameAi()
{
	active_powerups = 0;
	active_tresures = 0;
	next_powerup_id = 0;

	m_mb = new Events::Mailbox();	
	m_mb->request( Events::EventType::Quit );
	m_mb->request( Events::EventType::PowerupPickup );
	m_mb->request( Events::EventType::PowerupActivated );
	m_mb->request( Events::EventType::TogglePauseGame );
	m_mb->request( Events::EventType::KartHitByBullet );
	m_mb->request( Events::EventType::RoundStart );
	m_mb->request( Events::EventType::Input );
	m_mb->request( Events::EventType::StartMenuInput );

	goldSpawnPointCount = sizeof goldSpawnLocations / sizeof Vector3;

	m_open_points.push_back(Vector3(0.0, 0.0, 5.5));

	m_open_points.push_back(Vector3(1.0, 0.0, 5.5));
	m_open_points.push_back(Vector3(-1.0, 0.0, 5.5));
	m_open_points.push_back(Vector3(1.0, 0.0, -5.5));
	m_open_points.push_back(Vector3(-1.0, 0.0, -5.5));

	m_open_points.push_back(Vector3(6.5f, 0.f, 1.f));
	m_open_points.push_back(Vector3(-6.5f, 0.f, -1.f));
	m_open_points.push_back(Vector3(-6.5f, 0.f, 1.f));
	m_open_points.push_back(Vector3(6.5f, 0.f, -1.f));

	m_open_points.push_back(Vector3(3.22f, 0.f, 7.59f));
	m_open_points.push_back(Vector3(-3.22f, 0.f, 7.59f));
	m_open_points.push_back(Vector3(3.22f, 0.f, -7.59f));
	m_open_points.push_back(Vector3(-3.22f, 0.f, -7.59f));

	m_open_points.push_back(Vector3(7.59f, 0.f, 3.22f));
	m_open_points.push_back(Vector3(-7.59f, 0.f, -3.22f));
	m_open_points.push_back(Vector3(-7.59f, 0.f, 3.22f));
	m_open_points.push_back(Vector3(7.59f, 0.f, -3.22f));

	m_open_points.push_back(Vector3(15.5f, 2.f, 15.5f));
	m_open_points.push_back(Vector3(-15.5f, 2.f, 15.5f));
	m_open_points.push_back(Vector3(15.5f, 2.f, -15.5f));
	m_open_points.push_back(Vector3(-15.5f, 2.f, -15.5f));

	m_open_points.push_back(Vector3(6.5f, 0.f, 2.5f));
	m_open_points.push_back(Vector3(-6.5f, 0.f, -2.5f));
	m_open_points.push_back(Vector3(6.5f, 0.f, -2.5f));
	m_open_points.push_back(Vector3(-6.5f, 0.f, 2.5f));	

	m_open_points.push_back(Vector3(2.5f, 0.f, 6.5f));
	m_open_points.push_back(Vector3(-2.5f, 0.f, -6.5f));
	m_open_points.push_back(Vector3(2.5f, 0.f, -6.5f));
	m_open_points.push_back(Vector3(-2.5f, 0.f, 6.5f));

	m_open_points.push_back(Vector3(0.f, 1.f, 0.f));

	m_open_points.push_back(Vector3(1.5f, 1.f, 0.f));
	m_open_points.push_back(Vector3(-1.5f, 1.f, 0.f));
	m_open_points.push_back(Vector3(0.f, 1.f, 1.5f));
	m_open_points.push_back(Vector3(0.f, 1.f, -1.5f));

	m_open_points.push_back(Vector3(1.5f, 1.f, 1.5f));
	m_open_points.push_back(Vector3(-1.5f, 1.f, -1.5f));
	m_open_points.push_back(Vector3(1.5f, 1.f, -1.5f));
	m_open_points.push_back(Vector3(-1.5f, 1.f, 1.5f));

	m_open_points.push_back(Vector3(11.5f, 1.f, 0.f));
	m_open_points.push_back(Vector3(-11.5f, 1.f, 0.f));
	m_open_points.push_back(Vector3(0.f, 1.f, 11.5f));
	m_open_points.push_back(Vector3(0.f, 1.f, -11.5f));

	gamePaused = false;
	currentState = StartMenu;
}

GameAi::~GameAi()
{
}

void GameAi::setup()
{
	// Gameai might not even need a setup
}

void GameAi::add_to_future_respawn(Events::PowerupPickupEvent * event)
{
	auto to_spawn = new local_powerup_to_spawn();

	to_spawn->powerup_id = event->powerup_id;
	to_spawn->powerup_type = event->powerup_type;
	to_spawn->timer_to_respawn = TIME_TO_SPAWN_POWERUPS;
	to_spawn->pos = event->pos;

	m_to_spawn_vec[to_spawn->powerup_id] = to_spawn;
}

void GameAi::init_powerups_not_gold()
{
	std::vector<Vector3> powerup_location;
	std::vector<Events::Event *> events_out;
	powerup_location.push_back(Vector3(12,1,0));
	powerup_location.push_back(Vector3(-12,1,0));
	powerup_location.push_back(Vector3(0,1,12));
	powerup_location.push_back(Vector3(0,1,-12));

	for (Vector3 pos: powerup_location)
	{
		spawn_a_powerup_not_gold(pos, events_out);
	}

	m_mb->sendMail(events_out);
}

void GameAi::handle_powerups_not_gold(double time_elapsed)
{
	std::vector<Events::Event *> events_out;
	std::vector<entity_id> to_delete;

	for (auto to_spawn_pair : m_to_spawn_vec)
	{
		auto to_spawn = to_spawn_pair.second;
		auto pos = to_spawn->pos;
		to_spawn->timer_to_respawn -= time_elapsed;

		if (to_spawn->timer_to_respawn <= 0)
		{
			to_delete.push_back( to_spawn->powerup_id );
			spawn_a_powerup_not_gold(pos, events_out);
		}
	}
	
	// If spawned, remove struct from to_spawn
	for (auto id : to_delete)
	{
		auto current = m_to_spawn_vec[id];
		delete current;
		m_to_spawn_vec.erase(id);
	}

	if (events_out.size() > 0)
		m_mb->sendMail(events_out);
}

int GameAi::planFrame()
{
	const std::vector<Events::Event*> events_in = m_mb->checkMail();
	std::vector<Events::Event *> events_out;
	
	for (Events::Event *event : events_in) {
		switch( event->type ) 
		{
			case Events::EventType::Quit:
			return 0;
			break;

			case Events::EventType::PowerupPickup:
			{
				auto pickup = ((Events::PowerupPickupEvent *)event);
				auto powerup = pickup->powerup_type;
				entity_id kart_id = pickup->kart_id;

				// Score or store pickup
				auto kart = GETENTITY(kart_id, CarEntity);

				switch (powerup) 
				{
					case Entities::GoldCasePowerup:
						// Return point to pool
						open_point(pickup->pos);

						active_tresures--;
						kart->gold += BIG_GOLD_VALUE;

						DEBUGOUT("Kart %d picked up gold!", kart_id)
						break;

					case Entities::GoldCoinPowerup:
						kart->gold += SMALL_GOLD_VALUE;
						break;

					default:
						// Player loses any unused powerups
						kart->powerup_slot = powerup;
						add_to_future_respawn(pickup);
						DEBUGOUT("Kart %d picked up a powerup of type %d!\n", kart_id, powerup)
						break;
				}
			}
			break;

			case Events::EventType::PowerupActivated:
			{
				Events::PowerupActivatedEvent *powUsed = (Events::PowerupActivatedEvent *)event;
				switch (powUsed->powerup_type)
				{
					case Entities::HealthPowerup:
					{
						auto kart_entity = GETENTITY(powUsed->kart_id, CarEntity);
						kart_entity->health += HEALTH_POWERUP_AMOUNT;
					}
					break;

					default:
						break;
				}				
			}
			break;

			case Events::EventType::TogglePauseGame:
			{
				gamePaused = !gamePaused;
			}
			break;

			case Events::EventType::Input:
			{
				auto inputEvent = ((Events::InputEvent *)event);

				if(inputEvent->xPressed)		// Powerup should be activated
				{
					auto kart = GETENTITY(inputEvent->kart_id, CarEntity);
					if(kart != NULL)
					{
						if(kart->powerup_slot != Entities::NullPowerup)
						{
							//Entities::powerup_t p_type = kart->powerup_slot;

							auto pow_event = NEWEVENT(PowerupActivated);
							pow_event->kart_id = inputEvent->kart_id;
							pow_event->pos = kart->Pos;
							pow_event->powerup_type = kart->powerup_slot;

							events_out.push_back(pow_event);

							// Empty the power up slot
							kart->powerup_slot = Entities::NullPowerup;
						}
					}
				}
			}
			break;

			case Events::EventType::StartMenuInput:
			{
				inputPauseTimer -= frame_timer.CalcSeconds();
				if(inputPauseTimer <= 0)
				{
					auto inputEvent = ((Events::StartMenuInputEvent *)event);

					if(inputEvent->bPressed)
					{
						return 0;	// Quit the game
					}

					int numPlayers = 0;
					if (inputEvent->aPressed || inputEvent->onePressed) {
						numPlayers = 1;
					} else if (inputEvent->twoPressed) {
						numPlayers = 2;
					} else if (inputEvent->threePressed) {
						numPlayers = 3;
					} else if (inputEvent->fourPressed) {
						numPlayers = 4;
					}

					if (numPlayers > 0)
					{
						auto startRoundEvent = NEWEVENT( RoundStart );
						events_out.push_back(startRoundEvent);

						// Restart a new game
						endRound();
						newRound(numPlayers, NUM_KARTS - numPlayers);

						// Pause the game to allow audio to play countdown
						// If we are starting a second round the game is already paused so we skip this
						if (currentState == StartMenu)
							events_out.push_back( NEWEVENT( TogglePauseGame ));	

						currentState = RoundStart;
						roundStartCountdownTimer = 2.2;  // This is how long to wait until we unpause in seconds
					}
				}
			}
			break;

			case Events::EventType::KartHitByBullet:
			{
				// Apply damage to kart
				entity_id kart_id = ((Events::KartHitByBulletEvent *)event)->kart_id;
				auto kart = GETENTITY(kart_id, CarEntity);

				if(kart->isExploding)
					break;

				kart->health -= DAMAGE_FROM_BULLET;

				//DEBUGOUT("FROM GAME AI: HIT KART %d, health left: %f\n", kart_id, kart->health);

				// Reset kart if health gone
				// There should be a slight pause before this gets triggered to show the kart exploding and to
				// punish the kart for dying.
				if (kart->health <= 0)
				{
					// Add points to the kart that shot the bullet
					auto shootingKart = GETENTITY(((Events::KartHitByBulletEvent *)event)->source_kart_id, CarEntity);
					shootingKart->gold += KART_KILL_GOLD_VALUE;

					auto explodeEvent = NEWEVENT( Explosion );
					explodeEvent->exploder = kart_id;
					explodeEvent->pos = kart->Pos;

					events_out.push_back(explodeEvent);

					kart->isExploding = true;
					auto localDeadKart = new explodingKart();
					localDeadKart->kart_id = kart_id;
					localDeadKart->timer = TIME_TO_EXPLODE;

					m_exploding_karts.push_back(localDeadKart);	
				}
			}
			break;		

		default:
			break;
		}
	}
	m_mb->emptyMail();
	

	if(currentState == StartMenu)
	{
		auto menuEvent = NEWEVENT(StartMenu);
		events_out.push_back(menuEvent);
	}
	else if (currentState & (RoundStart | RoundInProgress))
	{
		if (currentState & RoundStart)
		{
			// Update countdown timer
			roundStartCountdownTimer -= frame_timer.CalcSeconds();
	
			if(roundStartCountdownTimer < 0)
			{
				auto unpauseEvent = NEWEVENT( TogglePauseGame );
				events_out.push_back(unpauseEvent);
				currentState = RoundInProgress;
			}
		}

		// Update the scoreboard to be sent to rendering
		updateScoreBoard();

		updateExplodingKarts();

		// Direct controllers to karts
		for (auto id : this->player_kart_ids) {
			Events::PlayerKartEvent *kart_event;
			if (currentState & (RoundStart | RoundInProgress)) {
				kart_event = NEWEVENT(PlayerKart);
			} else {
				kart_event = (Events::PlayerKartEvent *)NEWEVENT(AiKart);
			}
			kart_event->kart_id = id;
			events_out.push_back(kart_event);
		}

		for (auto id : this->ai_kart_ids) {
			auto kart_event = NEWEVENT(AiKart);
			kart_event->kart_id = id;
			events_out.push_back(kart_event);
		}
	}
	else if (currentState == RoundEnd)
	{
		auto menuEvent = NEWEVENT(StartMenu);
		events_out.push_back(menuEvent);
	}

	// Spawn Powerups?
	if (!gamePaused)
	{
		if (active_tresures <= 0) 
		{
			active_tresures++;
			auto pos = pick_point();
			events_out.push_back(spawn_powerup(Entities::GoldCasePowerup, pos));
		} 
	}

	// handle powerups
	handle_powerups_not_gold(frame_timer.CalcSeconds());

	// Issue planned events
	m_mb->sendMail(events_out);

	// Yield until next frame
	Real timeForLastFrame = frame_timer.CalcSeconds();
	Real sleepPeriod = 0.016 - timeForLastFrame;
	if (sleepPeriod > 0.0) 
	{
		std::chrono::milliseconds timespan( (int)( sleepPeriod * 1000.0f ) );
		std::this_thread::sleep_for(timespan);
	}

	// Report FPS
	const bool PRINT_FPS = 0;
	static int32_t frames = 0;
	if (PRINT_FPS) 
	{
		frames++;
		static Real timeAtLastFrame = 0;
		if ( (int32_t)timeAtLastFrame != (int32_t)fps_timer.CalcSeconds()) 
		{
			DEBUGOUT( "FPS: %d\n", frames);
			frames = 0;
		}
		timeAtLastFrame = fps_timer.CalcSeconds();
	}

	return 1;
}

Real GameAi::getElapsedTime()
{
	Real period = frame_timer.CalcSeconds();
	frame_timer.ResetClock();
	return period;
}

Vector3 GameAi::pick_point()
{
	Vector3	open = goldSpawnLocations[goldSpawnCounter % goldSpawnPointCount];
	goldSpawnCounter++;
	//int pt = rand() % m_open_points.size();
	//Vector3 open = m_open_points[pt];
	//m_open_points.erase(m_open_points.begin() + pt);
	return open;
}

void GameAi::open_point(Vector3 pt)
{
	//if (--active_powerups <= 0)
	//	DEBUGOUT("Warning: Extra powerup location appeared\n");
	//m_open_points.push_back(pt);
}

Events::PowerupPlacementEvent * GameAi::spawn_powerup(Entities::powerup_t p_type, Vector3 pos)
{
	auto p_event = NEWEVENT(PowerupPlacement);
	p_event->powerup_type = p_type;
	p_event->pos = pos;
	p_event->powerup_id = this->next_powerup_id++;

	if(p_type == Entities::GoldCasePowerup)
		m_gold_case_id = p_event->powerup_id;

	active_powerups++;
	return p_event;
}

// Returns true if kart with id 'i' has a score greater than kart with id 'j'
bool sortByScore(entity_id i, entity_id j)
{
	return GETENTITY(i, CarEntity)->gold > GETENTITY(j, CarEntity)->gold;	
}

// Sorts the list of kart id's based on score and also issues round end event if player
// gets over the set score goal
void GameAi::updateScoreBoard()
{
	std::vector<Events::Event *> events_out;

	std::vector<entity_id> sortedList = kart_ids;
	std::sort(sortedList.begin(), sortedList.end(), sortByScore);

	auto scoreBoardEvent = NEWEVENT(ScoreBoardUpdate);
	scoreBoardEvent->kartsByScore = sortedList;
	events_out.push_back(scoreBoardEvent);

	if(!sortedList.empty())
	{
		if(GETENTITY(sortedList[0], CarEntity)->gold >= FINAL_SCORE_GOAL)
		{
			// Some one has reached the goal, end the round
			events_out.push_back(NEWEVENT(TogglePauseGame));	// Pause the karts
			auto roundEndEvent = NEWEVENT(RoundEnd);

			// Did a player win?
			for (auto player : player_kart_ids) {
				if (sortedList[0] == player) {
					roundEndEvent->playerWon = true;
				}
			}

			events_out.push_back(roundEndEvent);	
			currentState = RoundEnd;
			inputPauseTimer = TIME_TO_IGNORE_INPUT;
		}
	}

	m_mb->sendMail(events_out);

	outputScoreBoard(sortedList);
}

// Outputs the score borad to the console
void GameAi::outputScoreBoard(std::vector<entity_id> list)
{
	for (auto kart_id : list) {
		auto kart = GETENTITY(kart_id, CarEntity);
		DEBUGOUT("Id:%d|Score:%lu || ", kart_id, kart->gold);
	}
	DEBUGOUT("\n");
}

void GameAi::endRound()
{
	std::vector<Events::Event *> events_out;

	// Destroy all karts
	for (auto id : this->kart_ids) 
	{
		auto destroyKartEvent = NEWEVENT(KartDestroyed);
		destroyKartEvent->kart_id = id;
		events_out.push_back(destroyKartEvent);
	}

	this->kart_ids.clear();
	this->ai_kart_ids.clear();
	this->player_kart_ids.clear();

	goldSpawnCounter = 0;
	active_tresures = 0;
	m_exploding_karts.clear();

	auto removeGoldPow = NEWEVENT( PowerupDestroyed );
	removeGoldPow->powerup_type = Entities::GoldCasePowerup;
	removeGoldPow->powerup_id = m_gold_case_id;

	events_out.push_back(removeGoldPow);

	m_mb->sendMail(events_out);
}

void GameAi::newRound(int numPlayers, int numAi)
{
	std::vector<Events::Event *> events;
	// Create karts
	for (int i = 0; i < numPlayers + numAi; i++) 
	{
		std::string kart_name = "Kart #" + i;
		auto kart = new Entities::CarEntity(kart_name);
		entity_id kart_id = g_inventory->AddEntity(kart);

		kart->playerNumber = i + 1;
		kart->health = STARTING_HEALTH;
		kart->isExploding = false;
		kart->powerup_slot = Entities::SpeedPowerup;

		this->kart_ids.push_back(kart_id);

		// First N karts are players
		if (i < numPlayers) {
			this->player_kart_ids.push_back(kart_id);
			kart->isHumanPlayer = true;
		} else {
			this->ai_kart_ids.push_back(kart_id);
			kart->isHumanPlayer = false;
		}

		// Tell people of the new kart
		auto new_kart_ev = NEWEVENT(KartCreated);
		new_kart_ev->kart_id = kart_id;
		events.push_back(new_kart_ev);
	}

	init_powerups_not_gold();

	m_mb->sendMail(events);
}

void GameAi::spawn_a_powerup_not_gold(Vector3 pos, std::vector<Events::Event *> &events_out)
{
	int type_num =  3; //rand() %4 ;
		switch (type_num)
		{
			case 0:
				events_out.push_back(spawn_powerup(Entities::HealthPowerup, pos));
				DEBUGOUT("SPAWNING A HEALTH POWERUP\n!")
			break;

			case 1:
				events_out.push_back(spawn_powerup(Entities::SpeedPowerup, pos));
				DEBUGOUT("SPAWNING A SPEED POWERUP!\n")
			break;

			case 2:
				events_out.push_back(spawn_powerup(Entities::RocketPowerup, pos));
				DEBUGOUT("SPAWNING A ROCKET POWERUP!\n")
			break;

			case 3:
				events_out.push_back(spawn_powerup(Entities::PulsePowerup, pos));
				DEBUGOUT("SPAWNING A PULSE POWERUP!\n")
			break;

			default:
				events_out.push_back(spawn_powerup(Entities::SpeedPowerup, pos));
		}
		
		return;
}

void GameAi::updateExplodingKarts()
{
	std::vector<Events::Event *> events;
	float elapsedTime = frame_timer.CalcSeconds();

	for(int i = m_exploding_karts.size() - 1; i >= 0; i--)
	{
		auto kart = m_exploding_karts[i];
		kart->timer -= elapsedTime;

		if(kart->timer <= 0)
		{
			auto resetEvent = NEWEVENT( Reset );
			resetEvent->kart_id = kart->kart_id;
			events.push_back(resetEvent);

			auto kart_entity = GETENTITY(kart->kart_id, CarEntity);
			kart_entity->health = STARTING_HEALTH;
			kart_entity->isExploding = false;

			// Remove kart from local exploding kart list
			m_exploding_karts.erase(m_exploding_karts.begin() + i);
		}
	}

	m_mb->sendMail(events);
}
