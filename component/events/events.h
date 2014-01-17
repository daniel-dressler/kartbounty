#pragma once
#include <vector>
#include <string>
#include <stdint.h>
#include "../entities/entities.h"


// Use this to get new events
#define NEWEVENT(type) (new type##Event(type))
namespace Events {
	enum EventType {
		NullEvent,
		Explosion,
		RoundStart
	};

	struct Event {
		EventType type;
		Event(EventType our_type) : type(our_type) {};

		int64_t id;
	};

	struct ExplosionEvent : Event {
		entity_id exploder;
		float force;
	};

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
