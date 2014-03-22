#include <chrono>
#include <thread>
#include <math.h>

#include "gameai.h"

// Score to win
#define FINAL_SCORE_GOAL 100000
// How much health to substruct on bullet hit
#define DAMAGE_FROM_BULLET 1
// Number of karts
#define NUM_KARTS 10
// Big Gold Powerup
#define BIG_GOLD_VALUE 5000
// Small Gold Powerup
#define SMALL_GOLD_VALUE 500
// How much health does a kart has to start with
#define STARTING_HEALTH 1

GameAi::GameAi()
{
	active_powerups = 0;
	active_tresures = 0;
	next_powerup_id = 3;

	m_mb = new Events::Mailbox();	
	m_mb->request( Events::EventType::Quit );
	m_mb->request( Events::EventType::PowerupPickup );
	m_mb->request( Events::EventType::TogglePauseGame );
	m_mb->request( Events::EventType::KartHitByBullet );
	m_mb->request( Events::EventType::RoundStart );
	
	Vector3 p_positions[] = {
		Vector3(0.0, 0.0, 5.5),
		Vector3(1.0, 0.0, 5.5),
		Vector3(-1.0, 0.0, 5.5),
		Vector3(3.22, 0.0, 7.95),
		Vector3(-3.22, 0.0, 7.59)};
	for (auto pt : p_positions) {
		m_open_points.push_back(pt);
	}

	gamePaused = false;
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

		this->kart_ids.push_back(kart_id);

		// Tell people of the new kart
		auto new_kart_ev = NEWEVENT(KartCreated);
		new_kart_ev->kart_id = kart_id;
		events.push_back(new_kart_ev);
	}
	m_mb->sendMail(events);
}

int GameAi::planFrame()
{
	std::vector<Events::Event *> events_out;

	const std::vector<Events::Event*> events_in = m_mb->checkMail();
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

				// Return point to pool
				open_point(pickup->pos);

				// Score or store pickup
				auto kart = GETENTITY(kart_id, CarEntity);
				switch (powerup) 
				{
					case Entities::GoldCasePowerup:
						active_tresures--;
						kart->gold += BIG_GOLD_VALUE;
						break;

					case Entities::GoldCoinPowerup:
						kart->gold += SMALL_GOLD_VALUE;
						break;

					default:
						// Player loses any unused powerups
						kart->powerup_slot = powerup;
						break;
				}
			}
			break;

			case Events::EventType::PowerupDestroyed:
			{
				auto pickup = ((Events::PowerupPickupEvent *)event);
				open_point(pickup->pos);
			}
			break;

			case Events::EventType::TogglePauseGame:
			{
				gamePaused = !gamePaused;
				//DEBUGOUT("Pause the game!\n");
			}
			break;

			case Events::EventType::RoundStart:
			{
				resetGame();
			}
			break;

			case Events::EventType::KartHitByBullet:
			{
				// Apply damage to kart
				auto kart_id = ((Events::KartCreatedEvent *)event)->kart_id;
				auto kart = GETENTITY(kart_id, CarEntity);
				kart->health -= DAMAGE_FROM_BULLET;

				DEBUGOUT("FROM GAME AI: HIT KART %d, health left: %d", kart_id, kart->health)
				
				// Destroy kart if health gone
				if (kart->health <= 0)
				{
					auto destroy_event = NEWEVENT(KartDestroyed);
					destroy_event->kart_id = kart_id;
					events_out.push_back(destroy_event);
				}

			}
			break;
			
			default:
			break;
		}
	}
	m_mb->emptyMail();
	
	// Direct controllers to karts
	bool first_kart = true;
	for (auto id : this->kart_ids) 
	{
		Events::Event *event;
		if (first_kart) 
		{
			first_kart = false;
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

	// Spawn Powerups?
	if (!gamePaused)
	{
		if (active_tresures <= 0) 
		{
			active_tresures++;
			events_out.push_back(spawn_powerup(Entities::GoldCasePowerup));
		} 
		else if (active_powerups <= 3) 
		{
			events_out.push_back(spawn_powerup(Entities::BulletPowerup));
		} 
	}

	// Update the scoreboard to be send to rendering
	updateScoreBoard();

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

Events::PowerupPlacementEvent *GameAi::spawn_powerup(Entities::powerup_t p_type)
{
	auto p_event = NEWEVENT(PowerupPlacement);
	p_event->powerup_type = p_type;
	p_event->pos = pick_point();
	p_event->powerup_id = this->next_powerup_id++;

	active_powerups++;
	return p_event;
}

// Returns true if kart with id 'i' has a score greater than kart with id 'j'
bool sortByScore(entity_id i, entity_id j)
{
	return ( GETENTITY(i, CarEntity)->gold > GETENTITY(j, CarEntity)->gold );	
}

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
			events_out.push_back(NEWEVENT(RoundEnd));
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



