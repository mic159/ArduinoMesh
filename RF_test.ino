#include <SPI.h>
#include "RF24.h"
#include "MeshBase.h"

class App : public MeshBase
{
public:
    App() : MeshBase(9, 10) {}
protected:
    virtual void	HandleMessage(uint32_t sender, const void* data, uint8_t length)
    {
        Serial.println("Got Message");
    }
    virtual void OnNewPeer(Peer* p)
    {
        SendMessage(p->address, "Hello", 6);
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
