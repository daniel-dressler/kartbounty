#include <map>
#include "events.h"

using namespace Events;

class PostOffice {
	//std::map<event_id_t, Event *> events;
	std::map<event_id_t, int32_t> ref_counts;

	std::map<mailbox_id_t, std::vector<Event *> > mailboxes;
	std::map<EventType, std::vector<mailbox_id_t> > registrants;
	event_id_t last_event_id;

	public:
	void emptyMail(mailbox_id_t mailbox_id)
	{
		for (Event *event : mailboxes[mailbox_id]) {
			auto id = (event_id_t)event->id;
			if (--ref_counts[id] <= 0) {
				//events.erase(id);
				ref_counts.erase(id);
				delete event;
			}
		}
		mailboxes[mailbox_id].clear();
	}

	const std::vector<Event *> checkMail(mailbox_id_t mailbox_id)
	{
		return mailboxes[mailbox_id];
	}

	void request(mailbox_id_t mailbox_id, EventType type)
	{
		registrants[type].push_back(mailbox_id);
	}

	void sendMail(mailbox_id_t sending_mailbox_id, std::vector<Event *> events)
	{
		for (Event *new_event : events) {
			if (new_event->type == EventType::NullEvent)
				continue;

			std::vector<mailbox_id_t> destinations = registrants[new_event->type];
	
			int32_t receiver_count = destinations.size();
			if (receiver_count == 0) {
				 // No one is listening
				delete new_event;
				continue;
			}
	
			// Stamp the mail
			static event_id_t last_event_id = 1;
			event_id_t id = ++last_event_id;
			new_event->id = id;
			//this->events[id] = new_event;
			ref_counts[id] = receiver_count;
	
			// Send it
			for (mailbox_id_t mailbox : registrants[new_event->type]) {
				mailboxes[mailbox].push_back(new_event);
			}
		}

		return;
	}

	int32_t getMailboxId()
	{
		static mailbox_id_t mailbox_count = 0;
		return ++mailbox_count;
	}
};

static PostOffice *Events_Global_PostOffice = NULL;

#define PO() Events_Global_PostOffice
Mailbox::Mailbox()
{
	if (PO() == NULL) {
		PO() = new PostOffice();
	}
	this->mailbox_id = PO()->getMailboxId();
}

void Mailbox::sendMail(std::vector<Event *> events)
{
	PO()->sendMail(mailbox_id, events);
}

void Mailbox::request(EventType type)
{
	PO()->request(mailbox_id, type);
}

const std::vector<Event *> Mailbox::checkMail()
{
	return PO()->checkMail(mailbox_id);
}

void Mailbox::emptyMail()
{
	PO()->emptyMail(mailbox_id);
}


