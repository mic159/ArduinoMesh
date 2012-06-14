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
    //randomSeed(analogRead(0));
    app.Begin();
}

void loop()
{
    app.Update();
    delay(100);
}
/*#include <SPI.h>
//#include "RF24.h"
//#include "printf.h"


//RF24 radio(9,10);

typedef enum
{
    BROADCAST_PEER_DISCOVERY = 1,
    BROADCAST_MISC,
    
    BROADCAST_MAX,
} broadcast_chanel;

const uint64_t broadcast_addresses[BROADCAST_MAX] = { 0, 0xF0F0F0F0E1LL, 0xF0F0F0F0D2LL };
const uint64_t client_addresses[4] = { 0x112100000F0LL, 0x11210000F1LL, 0x11210000F2LL, 0x11210000F3LL };
const uint8_t client_num = 1;

unsigned long last_broadcast;

struct Peer
{
    uint64_t address;
    uint32_t last_seen;
    boolean used;
    Peer() : address(0), last_seen(0), used(false) {}
};

Peer peers[20];


void setup(void)
{
    Serial.begin(9600);
    Serial.println("RF_TEST");
    Serial.print("ADDRESS: ");
    Serial.print(client_num, DEC);
    Serial.print(" - ");
    Serial.print((uint32_t)(client_addresses[client_num] >> 32), HEX);
    Serial.print((uint32_t)(client_addresses[client_num]      ), HEX);
    Serial.println("");

    radio.begin();
    radio.enableDynamicPayloads();
    radio.setRetries(2,1);
    //radio.openWritingPipe(client_addresses[client_num]);
    radio.openReadingPipe(0, client_addresses[client_num]);
    radio.openReadingPipe(BROADCAST_PEER_DISCOVERY,broadcast_addresses[BROADCAST_PEER_DISCOVERY]);
    radio.openReadingPipe(BROADCAST_MISC,broadcast_addresses[BROADCAST_MISC]);
    radio.setAutoAck(0, true);
    radio.setAutoAck(BROADCAST_PEER_DISCOVERY, false);
    radio.setAutoAck(BROADCAST_MISC, false);

    // Im here!
    sendPD();
}

bool doPD()
{
    return millis() - last_broadcast > 3000;
}

void sendPD()
{
    radio.stopListening();
    radio.openWritingPipe(broadcast_addresses[BROADCAST_PEER_DISCOVERY]);
    radio.write(&client_addresses[client_num], sizeof(client_addresses[client_num]));
    last_broadcast = millis();
    radio.startListening();
}

void sendMessage(uint64_t address, const void* message, uint8_t length)
{
    radio.stopListening();
    radio.openWritingPipe(address);
    radio.write(message, length);
    radio.startListening();
}


void loop(void)
{
    uint8_t pipe_num;
    if (radio.available(&pipe_num))
    {
        uint8_t len = radio.getDynamicPayloadSize();
        
        if (pipe_num == BROADCAST_PEER_DISCOVERY)
        {
            uint64_t payload;
            radio.read( &payload, len );
            Serial.print("Found client: ");
            Serial.print((uint32_t)(payload >> 32), HEX);
            Serial.print((uint32_t)(payload      ), HEX);
            Serial.println("");
            sendMessage(payload, "Hello there.", 12);
        }
        else //if (pipe_num == 0)
        {
            char payload[33];
            radio.read(payload, len);
            payload[len] = 0;
            Serial.print("Got packet. Message: ");
            Serial.println(payload);
        }
    }
    
    // Periodically send a PD broadcast.
    if (doPD()) { sendPD(); }
    
    delay(10);

}*/
