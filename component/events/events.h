#pragma once
#include <vector>
#include <string>
#include <stdint.h>
#include "../entities/entities.h"


// Use this to get new events
#define NEWEVENT(type) (new Events::type##Event(Events::EventType::type))
namespace Events {
	enum EventType {
		NullEvent,
		Explosion,
		RoundStart,
		KartMove,
		ArenaCollider,
		StateUpdate,
		Input,
		Quit
	};

	struct Event {
		EventType type;
		int64_t id;
	};
	
	#define EVENTSTRUCT(X)  struct X##Event : Event { X##Event(EventType our_type) {this->type = our_type;};
	#define ENDEVENT }

	EVENTSTRUCT(Explosion)
		entity_id exploder;
		float force;
	ENDEVENT;

	EVENTSTRUCT(StateUpdate)
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

		// ID of the kart this input event is targeted at
		int kartID;

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
		int32_t mailbox_id;
	};
}
