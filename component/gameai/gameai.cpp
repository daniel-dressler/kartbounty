#include <chrono>
#include <thread>
#include <math.h>

#include "gameai.h"
#include "../entities/entities.h"

// Rocket radius
#define ROCKET_DISTANCE 3
#define ROCKET_DAMAGE 1

// floating gold cooldown
#define TIMER_FOR_FLOATING_GOLD 25
// floating gold reward
#define REWARD_FOR_FLOATING_GOLD 5

// Score to win
#define FINAL_SCORE_GOAL 30
// How much health to substruct on bullet hit
#define DAMAGE_FROM_BULLET 0.3
// Number of karts
#define NUM_KARTS 8
// Big Gold Powerup
#define BIG_GOLD_VALUE 3
// Small Gold Powerup
#define SMALL_GOLD_VALUE 1
// Amount of gold for killing another kart
#define KART_KILL_GOLD_VALUE 2
// How much health does a kart has to start with
#define STARTING_HEALTH 1
#define HEALTH_POWERUP_AMOUNT 1
// timer to spawn powerups that aren't gold, in seconds
#define TIME_TO_SPAWN_POWERUPS 3
// time to stop listening to input events at end of round
#define TIME_TO_IGNORE_INPUT 0.5
// time to wait before resetting kart to show explosion
#define TIME_TO_EXPLODE 3

const Vector3 goldSpawnLocations[] = { Vector3(0,1.1, 0), Vector3(10, 1.1, -10), Vector3(-10, 1.1, 10), Vector3(17,2.1,0), Vector3(-17,2.1,0),
	Vector3(10,1.1,10), Vector3(-10,1.1,-10), Vector3(0,2.1,17), Vector3(0,2.1,-17) };
int goldSpawnCounter;
int goldSpawnPointCount;

std::vector<GameAi::floating_gold *> floating_gold_array;

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
	m_mb->request( Events::EventType::RocketHit );

	goldSpawnPointCount = sizeof goldSpawnLocations / sizeof Vector3;

	floating_gold_int();

	gamePaused = false;
	currentState = StartMenu;
}

GameAi::~GameAi()
{
}

void GameAi::floating_gold_int()
{
	// Floating gold locations
	floating_gold * fg0 = new floating_gold();
	fg0->location = Vector3(0,2.5f,0);
	fg0->active = false;
	fg0->timer = 0;
	fg0->isFloatingGold = true;
	fg0->index_in_vector = 0;
	floating_gold_array.push_back( fg0 );

	floating_gold * fg1 = new floating_gold();
	fg1->location = Vector3(0,3.5f,19);
	fg1->active = false;
	fg1->timer = 0;
	fg1->isFloatingGold = true;
	fg1->index_in_vector = 1;
	floating_gold_array.push_back( fg1 );
	
	struct floating_gold * fg2 = new floating_gold();
	fg2->location = Vector3(0,3.5f,-19);
	fg2->active = false;
	fg2->timer = 0;
	fg2->isFloatingGold = true;
	fg2->index_in_vector = 2;
	floating_gold_array.push_back( fg2 );

	// Other powerup locations

	struct floating_gold * fg3 = new floating_gold();
	fg3->location = Vector3(12,1,0);
	fg3->active = false;
	fg3->timer = 0;
	fg3->isFloatingGold = false;
	fg3->index_in_vector = 3;
	floating_gold_array.push_back( fg3 );

	struct floating_gold * fg4 = new floating_gold();
	fg4->location = Vector3(-12,1,0);
	fg4->active = false;
	fg4->timer = 0;
	fg4->isFloatingGold = false;
	fg4->index_in_vector = 4;
	floating_gold_array.push_back( fg4 );

	struct floating_gold * fg5 = new floating_gold();
	fg5->location = Vector3(0,1,12);
	fg5->active = false;
	fg5->timer = 0;
	fg5->isFloatingGold = false;
	fg5->index_in_vector = 5;
	floating_gold_array.push_back( fg5 );

	struct floating_gold * fg6 = new floating_gold();
	fg6->location = Vector3(0,1,-12);
	fg6->active = false;
	fg6->timer = 0;
	fg6->isFloatingGold = false;
	fg6->index_in_vector = 6;
	floating_gold_array.push_back( fg6 );
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

			case Events::EventType::RocketHit:
			{
				auto rocket_hit = ((Events::RocketHitEvent *)event);
				auto hit_pos = rocket_hit->hit_pos;

				for (auto affected_kart_id : kart_ids)
				{
					if (affected_kart_id != rocket_hit->shooting_kart_id)
					{
						auto current_kart = GETENTITY(affected_kart_id, CarEntity);
						auto pos = current_kart->Pos;

						btScalar delta_x =(hit_pos.getX() - pos.x);
						btScalar delta_z =(hit_pos.getZ() - pos.z);

						float karts_dist = sqrt(delta_x*delta_x + delta_z*delta_z);
						bool close_enough = (karts_dist < ROCKET_DISTANCE);

						if (close_enough)
						{
							current_kart->health -= ROCKET_DAMAGE;

							if (current_kart->health <= 0)
							{
								// Add points to the kart that shot the bullet
								auto shootingKart = GETENTITY(rocket_hit->shooting_kart_id, CarEntity);
								shootingKart->gold += KART_KILL_GOLD_VALUE;

								auto explodeEvent = NEWEVENT( Explosion );
								explodeEvent->exploder = affected_kart_id;
								explodeEvent->pos = current_kart->Pos;

								events_out.push_back(explodeEvent);

								current_kart->isExploding = true;
								auto localDeadKart = new explodingKart();
								localDeadKart->kart_id = affected_kart_id;
								localDeadKart->timer = TIME_TO_EXPLODE;

								m_exploding_karts.push_back(localDeadKart);	
							}
						}
					}
				}
			}
			break;

			case Events::EventType::PowerupPickup:
			{
				auto pickup = ((Events::PowerupPickupEvent *)event);
				auto powerup = pickup->powerup_type;
				entity_id kart_id = pickup->kart_id;

				// Score or store pickup
				auto kart = GETENTITY(kart_id, CarEntity);
				
				int index_fg;

				switch (powerup) 
				{
					case Entities::GoldCasePowerup:

						active_tresures--;
						kart->gold += BIG_GOLD_VALUE;

						DEBUGOUT("Kart %d picked up gold!", kart_id)
						break;

					case Entities::GoldCoinPowerup:
						kart->gold += SMALL_GOLD_VALUE;
						break;

					case Entities::FloatingGoldPowerup:
						index_fg = pickup->floating_index;
						floating_gold_array[index_fg]->active = false;
						floating_gold_array[index_fg]->timer = TIMER_FOR_FLOATING_GOLD;
						kart->gold += REWARD_FOR_FLOATING_GOLD;
						break;

					default:
						// Player loses any unused powerups
						kart->powerup_slot = powerup;
						index_fg = pickup->floating_index;
						floating_gold_array[index_fg]->active = false;
						floating_gold_array[index_fg]->timer = TIME_TO_SPAWN_POWERUPS;
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
						kart_entity->health = HEALTH_POWERUP_AMOUNT;
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

	// handle floating gold powerups
	update_floating_gold( frame_timer.CalcSeconds() );
	
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

	return open;
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

	//outputScoreBoard(sortedList);
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

	m_mb->sendMail(events);
}

Events::PowerupPlacementEvent * GameAi::spawn_a_powerup_not_gold(Vector3 pos)
{
	int type_num = rand() %4 ;
		switch (type_num)
		{
			case 0:
				DEBUGOUT("SPAWNING A HEALTH POWERUP\n!")
				return spawn_powerup(Entities::HealthPowerup, pos);
				
			break;

			case 1:
				DEBUGOUT("SPAWNING A SPEED POWERUP!\n")
				return spawn_powerup(Entities::SpeedPowerup, pos);
			break;

			case 2:
				DEBUGOUT("SPAWNING A ROCKET POWERUP!\n")
				return spawn_powerup(Entities::RocketPowerup, pos);
			break;

			case 3: default:
				DEBUGOUT("SPAWNING A PULSE POWERUP!\n")
				return spawn_powerup(Entities::PulsePowerup, pos);
			break;

				
		}

		return spawn_powerup(Entities::SpeedPowerup, pos);

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

void GameAi::update_floating_gold( double time)
{
	// handle floating gold powerups
	std::vector<Events::Event *> events_out_floating_gold ;

	for (auto gold : floating_gold_array)
	{
		if ( !gold->active && gold->timer <= 0)
		{
			Events::PowerupPlacementEvent * event;
			if (gold->isFloatingGold)
				event = spawn_powerup(Entities::FloatingGoldPowerup, gold->location);
			else
				event = spawn_a_powerup_not_gold(gold->location);

			event->floating_index = gold->index_in_vector;
			events_out_floating_gold.push_back( event );
			gold->active = true;
		}
		else
		{
			gold->timer -= time;
		}
	}

	// Issue planned events
	m_mb->sendMail(events_out_floating_gold);
}
