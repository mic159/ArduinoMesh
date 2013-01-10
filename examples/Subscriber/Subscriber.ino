#include <SPI.h>
#include "RF24.h"
#include "MeshBase.h"
#include "Publisher.h"
#include "LinkedList.h"

class SubscribeApp : public MeshBase
{
public:
	SubscribeApp() : MeshBase(9, 10) {}

	enum PublishMessageType {
		type_on_event = MeshBase::type_user,
		type_subscribe,
	};
protected:
	virtual void	OnMessage(const MeshBase::MessageHeader* meta, const void* data, uint8_t length)
	{
		if (meta->type == type_on_event)
		{
			if (length != sizeof(uint8_t))
			{
				Serial.println("LENGTH WRONG");
				return;
			}
			OnEvent(meta->address_from, *(uint8_t*)data);
		}
	}

	virtual void	OnNewPeer(Peer* p)
	{
		if (p->application_capabilities & MeshBase::capability_publish_events)
		{
			Serial.print("Found peer that has events! address=");
			Serial.println(p->address);
			SendMessage(p->address, type_subscribe, "", 1);
		}
	}

	virtual void	OnEvent(address_t address, uint8_t event_data)
	{
		Serial.print("Event from ");
		Serial.print(address);
		Serial.print(" data: ");
		Serial.println(event_data);
	}
};

SubscribeApp app;

void setup()
{
    Serial.begin(19200);
    Serial.println("Starting...");
    randomSeed(analogRead(0));
    app.Begin();
}

void loop()
{
    app.Update();
    delay(100);
}

