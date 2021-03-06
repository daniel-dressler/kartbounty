#pragma once
#include <vector>
#include <string>
#include <stdint.h>

#include <BulletCollision/CollisionShapes/btTriangleMesh.h>

#include "../entities/entities.h"

typedef uint64_t event_id_t;
typedef uint64_t mailbox_id_t;

// Use this to get new events
#define NEWEVENT(type) (new Events::type##Event(Events::EventType::type))
namespace Events {
	enum EventType {
		NullEvent,
		Explosion,
		RoundStart,
		RoundEnd,
		KartMove,
		ArenaCollider,
		StateUpdate,
		Input,
		StartMenu,
		StartMenuInput,
		PowerupPickup,
		PowerupPlacement,
		PowerupActivated,
		PowerupDestroyed,
		Reset,
		ArenaMeshCreated,
		AiKart,
		PlayerKart,
		KartCreated,
		KartDestroyed,
		AudioPlayPause,
		KartColideKart,
		KartColideArena,
		KartHandbrake,
		TogglePauseGame,
		ChangeMusic,
		ShootReport,
		MusicVolumeChange,
		Shoot,
		BulletList,
		ScoreBoardUpdate,
		KartHitByBullet,
		RocketHit,
		Quit
	};
	
	struct Event {
		EventType type;
		event_id_t id;
	};
	
	#define EVENTSTRUCT(X)  struct X##Event : Event { X##Event(enum EventType our_type) { \
		memset(this, 0, sizeof(*this));\
		this->type = our_type;};
	#define ENDEVENT }

	EVENTSTRUCT(KartCreated)
		entity_id kart_id;
	ENDEVENT;

	EVENTSTRUCT(KartDestroyed)
		entity_id kart_id;
	ENDEVENT;

	EVENTSTRUCT(MusicVolumeChange)
		bool increase;		// If false decrease music vol
	ENDEVENT;

	EVENTSTRUCT(PlayerKart)
		entity_id kart_id;
	ENDEVENT;

	EVENTSTRUCT(AiKart)
		entity_id kart_id;
	ENDEVENT;

	EVENTSTRUCT(ArenaMeshCreated)
		btTriangleMesh *arena;
	ENDEVENT;

	EVENTSTRUCT(ChangeMusic)
	ENDEVENT;

	EVENTSTRUCT(Explosion)
		entity_id exploder;
		float force;
		Vector3 pos;
	ENDEVENT;

	EVENTSTRUCT(StateUpdate)
	ENDEVENT;

	EVENTSTRUCT(PowerupPlacement)
		powerup_id_t powerup_id;
		Vector3 pos;
		Entities::powerup_t powerup_type;
		int floating_index;
	ENDEVENT;

	EVENTSTRUCT(PowerupPickup)
		powerup_id_t powerup_id;
		Vector3 pos;
		Entities::powerup_t powerup_type;
		entity_id kart_id;
		int floating_index;
	ENDEVENT;

	EVENTSTRUCT(PowerupDestroyed)
		powerup_id_t powerup_id;
		Vector3 pos;
		Entities::powerup_t powerup_type;
	ENDEVENT;

	EVENTSTRUCT(PowerupActivated)
		entity_id kart_id;
		powerup_id_t powerup_id;
		Vector3 pos;
		Entities::powerup_t powerup_type;
	ENDEVENT;


	EVENTSTRUCT(KartColideKart)
		entity_id kart_id;
		entity_id kart_id_alt;
		Vector3 pos;
		Real force;
	ENDEVENT;

	EVENTSTRUCT(KartColideArena)
		entity_id kart_id;
		Vector3 pos;
		Real force;
	ENDEVENT;

	EVENTSTRUCT(KartHandbrake)
		entity_id kart_id;
		Vector3 pos;
		Real speed;
	ENDEVENT;

	EVENTSTRUCT(Reset)
		entity_id kart_id;
	ENDEVENT;

	EVENTSTRUCT(ShootReport)
		entity_id shooting_kart_id;
		entity_id kart_being_hit_id;	
	ENDEVENT;

	EVENTSTRUCT(Shoot)
		entity_id kart_id;
		btVector3 forward;
		Vector3 kart_pos;
	ENDEVENT;

	EVENTSTRUCT(BulletList)
		void *list_of_bullets;
		void *list_of_rockets;
	ENDEVENT;

	EVENTSTRUCT(KartHitByBullet)
		entity_id kart_id;
		entity_id source_kart_id;
		Vector3 normal;
		Vector3 pos;
	ENDEVENT;

	EVENTSTRUCT(BulletHitWall)
		Vector3 normal;
		Vector3 pos;
	ENDEVENT;

	EVENTSTRUCT(Input)
		// Controller Buttons
		bool xPressed;
		bool yPressed;
		bool bPressed;
		bool aPressed;

		// Axes
		float rightTrigger;
		float leftTrigger;
		float leftThumbStickRL;
		float leftThumbStickUD;
		float rightThumbStickRL;
		float rightThumbStickUD;

		// Numbers
		bool onePressed;
		bool twoPressed;
		bool threePressed;
		bool fourPressed;

		// Development Cheats
		bool print_position;
		bool reset_requested;
		
		// The kart producing this event
		entity_id kart_id;
	ENDEVENT;

	EVENTSTRUCT(StartMenu)
	ENDEVENT;
	
	EVENTSTRUCT(StartMenuInput)
		// Controller Buttons
		bool xPressed;
		bool yPressed;
		bool bPressed;
		bool aPressed;

		// Numbers
		bool onePressed;
		bool twoPressed;
		bool threePressed;
		bool fourPressed;
	ENDEVENT;

	EVENTSTRUCT(AudioPlayPause)
	ENDEVENT;

	EVENTSTRUCT(TogglePauseGame)
	ENDEVENT;

	EVENTSTRUCT(RoundStart)
	ENDEVENT;

	EVENTSTRUCT(RoundEnd)
		bool playerWon;
	ENDEVENT;

	EVENTSTRUCT(ScoreBoardUpdate)
		std::vector<entity_id> kartsByScore;		// Karts are ordered from highest score to lowest
	ENDEVENT;	
	
	EVENTSTRUCT(RocketHit)
		entity_id shooting_kart_id;
		entity_id kart_hit_id;
		btVector3 hit_pos;
	ENDEVENT;


	EVENTSTRUCT(Quit)
	ENDEVENT;

	#undef EVENTSTRUCT
	#undef ENDEVENT

	// Simple types like RoundStart are class-less

	// Use your mailbox to ask and check for events
	// Note: You cannot unrequest an eventtype.
	// I shall add unrequesting if we run into
	// performance problems In either case you must
	// support getting stale mail.
	class Mailbox {
		public:
		Mailbox();

		// Must use 'new' to allocate event
		void sendMail(std::vector<Event *> events);

		// Register for events
		void request(EventType type);

		// Check for any sent events.
		// Do not free or delete anything from here
		const std::vector<Event *> checkMail();

		// Call once events have been handled
		// Events performs reference counting
		// and will delete all events once seen
		// by all requesters.
		void emptyMail();

		private:
		std::string mailbox_name;
		mailbox_id_t mailbox_id;
	};
}
