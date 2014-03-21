#include <chrono>
#include <thread>
#include <math.h>

#include "gameai.h"

GameAi::GameAi()
{
	active_powerups = 0;
	active_tresures = 0;
	next_powerup_id = 3;

	m_mb = new Events::Mailbox();	
	m_mb->request( Events::EventType::Quit );
	m_mb->request( Events::EventType::PowerupPickup );
	m_mb->request( Events::EventType::TogglePauseGame );

	Vector3 p_positions[] = {
		Vector3(0.0, 0.0, 5.5),
		Vector3(1.0, 0.0, 5.5),
		Vector3(-1.0, 0.0, 5.5),
		Vector3(3.22, 0.0, 7.95),
		Vector3(-3.22, 0.0, 7.59)};
	for (auto pt : p_positions) {
		m_open_points.push_back(pt);
	}
}

GameAi::~GameAi()
{
}

void GameAi::setup()
{
	std::vector<Events::Event *> events;
	// Create karts
	for (int i = 0; i < 2; i++) 
	{
		std::string kart_name = "Kart #" + i;
		auto kart = new Entities::CarEntity(kart_name);
		entity_id kart_id = g_inventory->AddEntity(kart);
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
	const std::vector<Events::Event*> events_in = m_mb->checkMail();
	for (Events::Event *event : events_in) {
		switch( event->type ) {
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
				switch (powerup) {
					case Entities::GoldCasePowerup:
						active_tresures--;
						kart->gold += 5000;
						break;
					case Entities::GoldCoinPowerup:
						kart->gold += 100;
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
				DEBUGOUT("Pause the game!\n");
			}
			break;
		default:
			break;
		}
	}
	m_mb->emptyMail();
	
	// Direct controllers to karts
	std::vector<Events::Event *> events_out;
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
	if (active_tresures <= 0) 
	{
		active_tresures++;
		events_out.push_back(spawn_powerup(Entities::GoldCasePowerup));
	} 
	else if (active_powerups <= 3) 
	{
		events_out.push_back(spawn_powerup(Entities::BulletPowerup));
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



