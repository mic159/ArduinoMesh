#include <SPI.h>
#include "RF24.h"
#include "MeshBase.h"
#include "Publisher.h"
#include "LinkedList.h"

PublishApp app;

unsigned long last_time;
uint8_t sequence;

void setup()
{
    Serial.begin(19200);
    Serial.println("Starting...");
    randomSeed(analogRead(0));
    app.Begin();
    last_time = millis();
    sequence = 0;
}

void loop()
{
    app.Update();
    delay(100);
    if (millis() - last_time > 10000)
    {
		app.OnEvent(sequence);
		++sequence;
		last_time = millis();
    }
}

