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

	struct ai_kart {
		Vector3 target_to_move;
		Vector3 lastPos;
		int TimeStartedTarget;
		float time_stuck;
		entity_id kart_id;
	};
	std::map<entity_id, ai_kart *> m_karts;

	Vector3 think_of_target(struct ai_kart *);
	Vector3 get_target_roaming();
	Vector3 get_target_aggressive();
	Vector3 get_target_pickups();

	Events::InputEvent *move_kart(struct ai_kart *, Real);
	Events::InputEvent *drive(btScalar ang, btScalar dist, struct ai_kart *);
	void avoid_obs(int index, bool send);
	float avoid_obs_sqr(struct ai_kart *);
	
	void init_graph();
	void init_obs();
	void init_obs_sqr();
	btScalar getAngle(Vector2 target, Vector3 pos, btVector3 forward);

	float get_distance(Vector3 a, Vector3 b);
	Vector3 findIntersection(Sphere s, Vector3 rayDirection, Vector3 rayOrigin);
};

