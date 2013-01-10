#ifndef PUBLISHER_H
#define PUBLISHER_H

#include <Arduino.h>
#include "MeshBase.h"

class PublishApp : public MeshBase
{
public:
	PublishApp() : MeshBase(9, 10)
	{
		application_capabilities |= MeshBase::capability_publish_events;
	}

	void OnEvent(uint8_t event_data)
	{
		const Target* current = targets.first;
		while (current != NULL)
		{
			SendMessage(current->address, type_on_event, &event_data, sizeof(event_data));
			current = current->next;
		}
	}

	enum PublishMessageType {
		type_on_event = MeshBase::type_user,
		type_subscribe,
	};
protected:
	virtual void	OnMessage(const MeshBase::MessageHeader* meta, const void* data, uint8_t length)
	{
		if (meta->type == type_subscribe)
		{
			targets.Add(new Target(meta->address_from));
		}
	}
private:
	struct Target
	{
		Target(address_t target) : address(target), prev(NULL), next(NULL) {}
		address_t address;
		Target* prev;
		Target* next;
	};
	LinkedList2<Target> targets;
};

#endif // PUBLISHER_H
