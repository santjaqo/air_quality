/*
AirQuality Demo V1.0.
connect to A1 to start testing. it will needs about 20s to start
* By: http://www.seeedstudio.com
*/
//#include "AirQuality.h"
#include "Arduino.h"
//AirQuality airqualitysensor;
int current_quality =-1;
int publishInterval = 5000; // 30 seconds
long lastPublishMillis = 0;
void setup()
{
    Serial.begin(115200);
}
void loop()
{
   if (millis() - lastPublishMillis > publishInterval) {
      // read the input on analog pin 0:
      int sensorValue = analogRead(A0);
      // Convert the analog reading (which goes from 0 - 1023) to a voltage (0 - 3.2V):
      float voltage = sensorValue * (3.2 / 1023.0);
      // print out the value you read:
      Serial.println(voltage);
     lastPublishMillis = millis();
 }
    }
