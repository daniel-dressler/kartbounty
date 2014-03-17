#pragma once

#include "../../Standard.h"
#include "../events/events.h"

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

	enum drivingMode {Reverse, Roaming};

	struct ai_kart {
		Vector3 target_to_move;
		Vector3 lastPos;

		float target_timer;

		float time_stuck;
		entity_id kart_id;
		int current_target_index;

		drivingMode driving_mode;
	};

	std::map<entity_id, ai_kart *> m_karts;

	void think_of_target(struct ai_kart *);
	void get_target_roaming(struct ai_kart *kart);
	void get_target_aggressive(struct ai_kart *kart);
	void get_target_pickups(struct ai_kart *kart);

	Events::InputEvent *move_kart(struct ai_kart *, Real);
	Events::InputEvent *drive(btScalar ang, btScalar dist, struct ai_kart *, Real elapse_time);
	void avoid_obs(int index, bool send);
	float avoid_obs_sqr(struct ai_kart *);
	
	void init_graph();
	void init_obs();
	void init_obs_sqr();
	btScalar getAngle(Vector2 target, Vector3 pos, btVector3 *forward);

	float get_distance(Vector3 a, Vector3 b);
	Vector3 findIntersection(Sphere s, Vector3 rayDirection, Vector3 rayOrigin);
};

