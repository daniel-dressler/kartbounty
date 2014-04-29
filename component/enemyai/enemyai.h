#pragma once

#include "../../Standard.h"
#include "../events/events.h"
#include <map>
#include "util/Sphere.h"

class EnemyAi 
{
public:
	EnemyAi();
	~EnemyAi();
	void setup();
	void update(Real elapsed_time);

private:
	Events::Mailbox m_mb;
	entity_id m_player_kart;
	bool has_player_kart;

	enum drivingMode {Reverse, Roaming, Aggressive, Gold};


	struct ai_kart 
	{
		Vector3 target_to_move;
		Vector3 lastPos;

		float target_timer; // timer for driving around

		float time_stuck;
		entity_id kart_id;
		entity_id target_kart_id;
		int current_target_index;
		drivingMode driving_mode;
		entity_id can_shoot;
		float shoot_timer;	// Cooldown for the next shot
	};

	std::map<entity_id, ai_kart *> m_karts;
	std::vector<entity_id> m_kart_ids;


	void think_of_target(struct ai_kart *);
	void get_target_roaming(struct ai_kart *kart);
	void get_target_aggressive(struct ai_kart *kart);

	Vector3 gold_position;
	void get_target_pickups(struct ai_kart *kart);

	Events::InputEvent *move_kart(struct ai_kart *, Real);
	Events::InputEvent *drive(btScalar ang, struct ai_kart *);
	void avoid_obs(int index, bool send);
	float avoid_obs_sqr(struct ai_kart *);
	
	void init_graph();
	void init_obs();
	void init_obs_sqr();
	btScalar getAngle(Vector2 target, Vector3 pos, btVector3 *forward);

	float get_distance(Vector3 a, Vector3 b);
	Vector3 findIntersection(Sphere s, Vector3 rayDirection, Vector3 rayOrigin);
	void kart_shoot(entity_id kart_id);
};

