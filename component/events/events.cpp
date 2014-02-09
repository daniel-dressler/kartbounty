#include <map>
#include "events.h"

using namespace Events;

class PostOffice {
	std::map<int64_t, Event *> events;
	std::map<int64_t, int32_t> ref_counts;

	std::map<int32_t, std::vector<Event *> > mailboxes;
	std::map<EventType, std::vector<int32_t> > registrants;
	int32_t last_event_id;

	public:
	void emptyMail(int32_t mailbox_id)
	{
		// TODO: lock the postoffice
		for (Event *event : mailboxes[mailbox_id]) {
			int32_t id = (int32_t)event->id;
			if (--ref_counts[id] <= 0) {
				events.erase(id);
				ref_counts.erase(id);
				delete event;
			}
		}
		mailboxes[mailbox_id].clear();
	}

	const std::vector<Event *> checkMail(int32_t mailbox_id)
	{
		return mailboxes[mailbox_id];
	}

	void request(int32_t mailbox_id, EventType type)
	{
		// TODO: lock the postoffice
		registrants[type].push_back(mailbox_id);
	}

	void sendMail(int32_t sending_mailbox_id, std::vector<Event *> events)
	{
		// TODO: lock the postoffice
		for (Event *new_event : events) {
			if (new_event->type == EventType::NullEvent)
				continue;

			std::vector<int32_t> destinations = registrants[new_event->type];
	
			int32_t receiver_count = destinations.size();
			if (receiver_count == 0) {
				 // No one is listening
				delete new_event;
				continue;
			}
	
			// Stamp the mail
			static int64_t last_event_id = 1;
			int64_t id = ++last_event_id;
			new_event->id = id;
			this->events[id] = new_event;
			ref_counts[id] = receiver_count;
	
			// Send it
			for (int32_t mailbox : registrants[new_event->type]) {
				mailboxes[mailbox].push_back(new_event);
			}
		}

		return;
	}

	int32_t getMailboxId()
	{
		// TODO: lock the postoffice
		static int32_t mailbox_count = 0;
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


