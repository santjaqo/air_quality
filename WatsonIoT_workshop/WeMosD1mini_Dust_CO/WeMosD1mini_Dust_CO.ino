/**
 IBM IoT Foundation managed Device

 Author: Ant Elder
 License: Apache License v2
*/
#include <ESP8266WiFi.h>
#include <PubSubClient.h> // https://github.com/knolleary/pubsubclient/releases/tag/v2.3
#include <ArduinoJson.h> // https://github.com/bblanchon/ArduinoJson/releases/tag/v5.0.7

//-------- Customise these values -----------
const char* ssid = "jaqo"; //"<yourWiFiSSID>"
const char* password = "comeandgetsome10!";//"<yourWiFiPassword>"

#define ORG "f1mdhs"//"<yourOrg>" This is not your BlueMix org. You get this from the first prefix of the URL of your sensor information webpage, in IBM IoT foundation.
#define DEVICE_TYPE "WeMos_D1_mini"//"yourDeviceType"
#define DEVICE_ID "my_WeMosD1miniPro"//"yourDevice"
#define TOKEN "uEtHBjMUC-AvaP-&zF"//"yourDeviceToken"
//-------- Customise the above values --------

char server[] = ORG ".messaging.internetofthings.ibmcloud.com";
char authMethod[] = "use-token-auth";
char token[] = TOKEN;
char clientId[] = "d:" ORG ":" DEVICE_TYPE ":" DEVICE_ID;

const char publishTopic[] = "iot-2/evt/status/fmt/json";
const char responseTopic[] = "iotdm-1/response";
const char manageTopic[] = "iotdevice-1/mgmt/manage";
const char updateTopic[] = "iotdm-1/device/update";
const char rebootTopic[] = "iotdm-1/mgmt/initiate/device/reboot";
const char rainfallTopic[] = "iot-2/cmd/rainfall/fmt/json";

void callback(char* topic, byte* payload, unsigned int payloadLength);

WiFiClient wifiClient;
PubSubClient client(server, 1883, callback, wifiClient);
int ledPin = D4;
int publishInterval = 30000; // 30 seconds
long lastPublishMillis;
int ledStatus ;

//---------Dust Sensor---------
int dust_pin = 8;
unsigned long duration;
unsigned long starttime;
unsigned long sampletime_ms = 30000;//sampling 30s ;
unsigned long lowpulseoccupancy = 0;
float ratio = 0;
float concentration = 0;

//------------//SETUP//-------------

void setup() {
 Serial.begin(115200); Serial.println();
 pinMode(ledPin, OUTPUT);
 digitalWrite(ledPin, LOW);
 ledStatus = false;
 wifiConnect();
 mqttConnect();
 initManagedDevice();

//---------Dust Sensor---------
  pinMode(dust_pin,INPUT);
  starttime = millis();//get the current time;
}

void loop() {
 if (millis() - lastPublishMillis > publishInterval) {
   publishData();
   lastPublishMillis = millis();
 }

 if (!client.loop()) {
   mqttConnect();
   initManagedDevice();
 }
 duration = pulseIn(dust_pin, LOW);
    lowpulseoccupancy = lowpulseoccupancy+duration;

    if ((millis()-starttime) > sampletime_ms)//if the sampel time == 30s
    {
        ratio = lowpulseoccupancy/(sampletime_ms*10.0);  // Integer percentage 0=>100
        concentration = 1.1*pow(ratio,3)-3.8*pow(ratio,2)+520*ratio+0.62; // using spec sheet curve
        Serial.print(lowpulseoccupancy);
        Serial.print(",");
        Serial.print(ratio);
        Serial.print(",");
        Serial.println(concentration);
        lowpulseoccupancy = 0;
        starttime = millis();
    }
}

void wifiConnect() {
 Serial.print("Connecting to "); Serial.print(ssid);
 WiFi.begin(ssid, password);
 while (WiFi.status() != WL_CONNECTED) {
   delay(500);
   Serial.print(".");
 }
 Serial.print("nWiFi connected, IP address: "); Serial.println(WiFi.localIP());
}

void mqttConnect() {
 if (!!!client.connected()) {
   Serial.print("Reconnecting MQTT client to "); Serial.println(server);
   while (!!!client.connect(clientId, authMethod, token)) {
     Serial.print(".");
     delay(500);
   }
   Serial.println();
 }
}

void initManagedDevice() {
 if (client.subscribe("iotdm-1/response")) {
   Serial.println("subscribe to responses OK");
 } else {
   Serial.println("subscribe to responses FAILED");
 }

  if (client.subscribe(rainfallTopic)) {
   Serial.println("subscribe to rainfall OK");
 } else {
   Serial.println("subscribe to rainfall FAILED");
 }


 if (client.subscribe(rebootTopic)) {
   Serial.println("subscribe to reboot OK");
 } else {
   Serial.println("subscribe to reboot FAILED");
 }

 if (client.subscribe("iotdm-1/device/update")) {
   Serial.println("subscribe to update OK");
 } else {
   Serial.println("subscribe to update FAILED");
 }

 StaticJsonBuffer<300> jsonBuffer;
 JsonObject& root = jsonBuffer.createObject();
 JsonObject& d = root.createNestedObject("d");
 JsonObject& metadata = d.createNestedObject("metadata");
 metadata["publishInterval"] = publishInterval;
 JsonObject& supports = d.createNestedObject("supports");
 supports["deviceActions"] = true;

 char buff[300];
 root.printTo(buff, sizeof(buff));
 Serial.println("publishing device metadata:"); Serial.println(buff);
 if (client.publish(manageTopic, buff)) {
   Serial.println("device Publish ok");
 } else {
   Serial.print("device Publish failed:");
 }
}

void publishData() {
 String payload = "{\"d\":{\"counter\":";
 payload += millis() / 1000;
 payload += "}}";

 Serial.print("Sending payload: "); Serial.println(payload);

 if (client.publish(publishTopic, (char*) payload.c_str())) {
   Serial.println("Publish OK");
 } else {
   Serial.println("Publish FAILED");
 }
}

void handleUpdate(byte* payload) {
 StaticJsonBuffer<300> jsonBuffer;
 JsonObject& root = jsonBuffer.parseObject((char*)payload);
 if (!root.success()) {
   Serial.println("handleUpdate: payload parse FAILED");
   return;
 }
 Serial.println("handleUpdate payload:"); root.prettyPrintTo(Serial); Serial.println();

 JsonObject& d = root["d"];
 JsonArray& fields = d["fields"];
 for (JsonArray::iterator it = fields.begin(); it != fields.end(); ++it) {
   JsonObject& field = *it;
   const char* fieldName = field["field"];
   if (strcmp (fieldName, "metadata") == 0) {
     JsonObject& fieldValue = field["value"];
     if (fieldValue.containsKey("publishInterval")) {
       publishInterval = fieldValue["publishInterval"];
       Serial.print("publishInterval:"); Serial.println(publishInterval);
     }
   }
 }
}

void callback(char* topic, byte* payload, unsigned int payloadLength) {
 Serial.print("callback invoked for topic: "); 
 Serial.println(topic);

 if (strcmp (responseTopic, topic) == 0) {
   return; // just print of response for now
 }

 if (strcmp (rebootTopic, topic) == 0) {
   Serial.println("Rebooting...");
   ESP.restart();
 }

 if (strcmp (updateTopic, topic) == 0) {
   handleUpdate(payload);
 }

 }


