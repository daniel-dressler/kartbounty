#include <chrono>
#include <thread>
#include <math.h>

#include "gameai.h"
#include "../entities/entities.h"

// Score to win
#define FINAL_SCORE_GOAL 25000
// How much health to substruct on bullet hit
#define DAMAGE_FROM_BULLET 1
// Number of karts
#define NUM_KARTS 4

// Big Gold Powerup
#define BIG_GOLD_VALUE 5000
// Small Gold Powerup
#define SMALL_GOLD_VALUE 500
// Amount of gold for killing another kart
#define KART_KILL_GOLD_VALUE 2000
// How much health does a kart has to start with
#define STARTING_HEALTH 5
#define HEALTH_POWERUP_AMOUNT 5
// timer to spawn powerups that aren't gold, in seconds
#define TIME_TO_SPAWN_POWERUPS 3

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
	std::vector<Events::Event *> events;
	// Create karts
	for (int i = 0; i < NUM_KARTS; i++) 
	{
		std::string kart_name = "Kart #" + i;
		auto kart = new Entities::CarEntity(kart_name);
		entity_id kart_id = g_inventory->AddEntity(kart);

		kart->health = STARTING_HEALTH;
		kart->powerup_slot = Entities::SpeedPowerup;

		this->kart_ids.push_back(kart_id);

		// Set the first created kart as player 1's kart
		if(i == 0)
		{
			player1KartId = kart_id;
		}

		// Tell people of the new kart
		auto new_kart_ev = NEWEVENT(KartCreated);
		new_kart_ev->kart_id = kart_id;
		events.push_back(new_kart_ev);
	}

	init_powerups_not_gold();

	m_mb->sendMail(events);
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
		int type_num = rand() %2;
		if (type_num == 0)
			events_out.push_back(spawn_powerup(Entities::HealthPowerup, pos));
		else
			events_out.push_back(spawn_powerup(Entities::SpeedPowerup, pos));
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
			int type_num = rand() %2;
			if (type_num == 0)
				events_out.push_back(spawn_powerup(Entities::HealthPowerup, pos));
			else
				events_out.push_back(spawn_powerup(Entities::SpeedPowerup, pos));
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
						break;

					case Entities::GoldCoinPowerup:
						kart->gold += SMALL_GOLD_VALUE;
						break;

					default:
						// Player loses any unused powerups
						kart->powerup_slot = powerup;
						add_to_future_respawn(pickup);
						break;
				}
			}
			break;

			// This never actually happens, lol
			case Events::EventType::PowerupDestroyed:
			{
				auto pickup = ((Events::PowerupDestroyedEvent *)event);
				if (pickup->powerup_type == Entities::GoldCasePowerup)
					open_point(pickup->pos);
			
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
							Entities::powerup_t p_type = kart->powerup_slot;

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
				auto inputEvent = ((Events::StartMenuInputEvent *)event);

				if(inputEvent->bPressed)
				{
					return 0;	// Quit the game
				}

				if(inputEvent->aPressed)
				{
					auto startRoundEvent = NEWEVENT( RoundStart );
					events_out.push_back(startRoundEvent);

					// Reset all karts
					resetGame();

					// Pause the game to allow audio to play countdown
					// If we are starting a second round the game is already paused so we skip this
					if(currentState == StartMenu)
						events_out.push_back( NEWEVENT( TogglePauseGame ));	

					currentState = RoundStart;
					roundStartCountdownTimer = 2.2;  // This is how long to wait until we unpause in seconds
				}
			}
			break;

			case Events::EventType::KartHitByBullet:
			{
				// Apply damage to kart
				entity_id kart_id = ((Events::KartHitByBulletEvent *)event)->kart_id;
				auto kart = GETENTITY(kart_id, CarEntity);
				kart->health -= DAMAGE_FROM_BULLET;

				DEBUGOUT("FROM GAME AI: HIT KART %d, health left: %d", kart_id, kart->health);

				// Reset kart if health gone
				// There should be a slight pause before this gets triggered to show the kart exploding and to
				// punish the kart for dying.
				if (kart->health <= 0)
				{
					// Add points to the kart that shot the bullet
					auto shootingKart = GETENTITY(((Events::KartHitByBulletEvent *)event)->source_kart_id, CarEntity);
					shootingKart->gold += KART_KILL_GOLD_VALUE;

					kart->health = STARTING_HEALTH;
					auto reset_kart_event = NEWEVENT(Reset);
					reset_kart_event->kart_id = kart_id;
					events_out.push_back(reset_kart_event);					
				}
			}
			break;		

		default:
			break;
		}
	}

	m_mb->emptyMail();
	
	// Direct controllers to karts
	for (auto id : this->kart_ids) 
	{
		Events::Event *event;
		if (id == player1KartId && (currentState != StartMenu || currentState != RoundEnd) )
		{
			auto kart_event = NEWEVENT(PlayerKart);
			kart_event->kart_id = id;
			event = kart_event;
		} 
		else 
		{
			auto kart_event = NEWEVENT(AiKart);
			kart_event->kart_id = id;
			event = kart_event;
		}

		events_out.push_back(event);
	}

	if(currentState == StartMenu)
	{
		auto menuEvent = NEWEVENT(StartMenu);
		events_out.push_back(menuEvent);
	}
	else if (currentState == RoundStart)
	{
		// Update countdown timer
		roundStartCountdownTimer -= frame_timer.CalcSeconds();

		if(roundStartCountdownTimer < 0)
		{
			auto unpauseEvent = NEWEVENT( TogglePauseGame );
			events_out.push_back(unpauseEvent);
			currentState = RoundInProgress;
		}

		// Update the scoreboard to be sent to rendering
		updateScoreBoard();
	}
	else if (currentState == RoundInProgress)
	{
		// Update the scoreboard to be sent to rendering
		updateScoreBoard();
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
	const bool PRINT_FPS = 1;
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
	int pt = rand() % m_open_points.size();
	Vector3 open = m_open_points[pt];
	m_open_points.erase(m_open_points.begin() + pt);
	return open;
}

void GameAi::open_point(Vector3 pt)
{
	if (--active_powerups <= 0)
		DEBUGOUT("Warning: Extra powerup location appeared\n");
	m_open_points.push_back(pt);
}

Events::PowerupPlacementEvent *GameAi::spawn_powerup(Entities::powerup_t p_type, Vector3 pos)
{
	auto p_event = NEWEVENT(PowerupPlacement);
	p_event->powerup_type = p_type;
	p_event->pos = pos;
	p_event->powerup_id = this->next_powerup_id++;

	active_powerups++;
	return p_event;
}

// Returns true if kart with id 'i' has a score greater than kart with id 'j'
bool sortByScore(entity_id i, entity_id j)
{
	return ( GETENTITY(i, CarEntity)->gold > GETENTITY(j, CarEntity)->gold );	
}


// Sorts the list of kart id's based on score and also issues round end event if player
// gets over the set score goal
void GameAi::updateScoreBoard()
{
	// Sorts the kart id list with the kart with the largest amount of gold first 
	std::sort (kart_ids.begin(), kart_ids.end(), sortByScore);

	std::vector<Events::Event *> events_out;
	auto scoreBoardEvent = NEWEVENT(ScoreBoardUpdate);
	scoreBoardEvent->kartsByScore = kart_ids;
	events_out.push_back(scoreBoardEvent);
	
	// Check for end of game condition
	if(!kart_ids.empty())
	{
		if(GETENTITY(kart_ids[0], CarEntity)->gold > FINAL_SCORE_GOAL)
		{
			// Some one has reached the goal, end the round
			events_out.push_back(NEWEVENT(TogglePauseGame));	// Pause the karts
			auto roundEndEvent = NEWEVENT(RoundEnd);
			if(kart_ids[0] == player1KartId)
				roundEndEvent->playerWon = true;
			else
				roundEndEvent->playerWon = false;

			events_out.push_back(roundEndEvent);	
			currentState = RoundEnd;
		}
	}

	m_mb->sendMail(events_out);

	//outputScoreBoard();
}

// Outputs the score borad to the console
void GameAi::outputScoreBoard()
{
	for(int i = 0; i < kart_ids.size(); i++)
	{
		auto kart = GETENTITY(kart_ids[i], CarEntity);
		DEBUGOUT("Id:%f|Score:%lu || ", kart_ids[i], kart->gold);
	}
	DEBUGOUT("\n");
}

void GameAi::resetGame()
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

	m_mb->sendMail(events_out);

	// Create a bunch of new ones
	setup();
}



