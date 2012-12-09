#include <SPI.h>
#include "RF24.h"
#include "MeshBase.h"

class App : public MeshBase
{
public:
    App() : MeshBase(9, 10) {}
protected:
    virtual void	OnMessage(const MeshBase::MessageHeader* meta, const void* data, uint8_t length)
    {
        Serial.print(meta->address_from, DEC);
        Serial.print(" : ");
        Serial.println((const char*)data);
        Serial.print("split_part = ");
        Serial.println(meta->split_part, DEC);
    }
    virtual void OnNewPeer(Peer* p)
    {
        if (!IsReady()) return;
        char buff[255];
        int len = snprintf(buff, 255, "Hello %u. This is a super long message for some reason. Please keep adding to this message for great good and other things. I would very much like it if this could be put together.", p->address);
        Serial.print("Me : ");
        Serial.println(buff);
        SendMessage(p->address, type_user, buff, len + 1);
    }
};

App app;

void setup()
{
    Serial.begin(9600);
    Serial.println("Starting RF_TEST");
    randomSeed(analogRead(0));
    app.Begin();
}

void loop()
{
    app.Update();
    delay(100);
}

