/**
 IBM IoT Foundation managed Device

 Author: S. Gaitan
 Based on: Ant Elder's code
 License: Only internal use at IBM
*/
//---------Dust Sensor---------
int dust_pin = D1;
unsigned long duration;
unsigned long starttime;
unsigned long sampletime_ms = 5000;//sample 30s ;
unsigned long lowpulseoccupancy = 0;
float ratio = 0;
float concentration = 0;

//---------DHT Sensor---------
#include <DHT.h>
#define DHTPIN D2
#define DHTTYPE DHT11
DHT dht(DHTPIN, DHTTYPE);
float humidity = 0;
float temperature = 0;
float heat_index = 0;

// Start of IoT Settings:____________________________
#include <PubSubClient.h> // https://github.com/knolleary/pubsubclient/releases/tag/v2.3
#include <ArduinoJson.h> // https://github.com/bblanchon/ArduinoJson/releases/tag/v5.0.7
#include <ESP8266WiFi.h>
//-------- Customise these values -----------
const char* ssid = "jaqo";
const char* password = "comeandgetsome10!";

#define ORG "w0xz9w"
#define DEVICE_TYPE "waterdroid"
#define DEVICE_ID "ESP_test"
#define TOKEN "4eFmfb8w6h@5s4rUh*"
//#define DEVICE_ID "ESP_test_kerem"
//#define TOKEN "3OTBoanwjTv@Rb_q+q"
//#define DEVICE_ID "ESP_test_omar"
//#define TOKEN "opwjtE7INx2vPtXl@w"

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

void callback(char* topic, byte* payload, unsigned int payloadLength);

WiFiClient wifiClient;
PubSubClient client(server, 1883, callback, wifiClient);

int publishInterval = 5000;
long lastPublishMillis;

// End of IoT Settings:____________________________

void setup() {
    Serial.begin(115200);

    // Start of IoT Settings:____________________________    
    wifiConnect();
    mqttConnect();
    initManagedDevice();
    // End of IoT Settings:____________________________
    
    //---------Dust Sensor---------
    pinMode(dust_pin,INPUT);
    starttime = millis();//get the current time;

    //---------DHT Sensor---------
    dht.begin();
}

void loop() {
  //---------Dust Sensor---------
    duration = pulseIn(dust_pin, LOW);
    lowpulseoccupancy = lowpulseoccupancy+duration;

    //---------DHT Sensor---------
    humidity = dht.readHumidity();
    temperature = dht.readTemperature();
    heat_index = dht.computeHeatIndex(temperature, humidity, false);
    
    if (millis() - lastPublishMillis > publishInterval) {
        //---------Dust Sensor---------
        ratio = lowpulseoccupancy/(publishInterval*10.0);  // Integer percentage 0=>100
        concentration = 1.1*pow(ratio,3)-3.8*pow(ratio,2)+520*ratio+0.62; // using spec sheet curve
        Serial.print("Low pulse occupancy:" + String(lowpulseoccupancy));
        Serial.println();
        Serial.print("Ratio: " + String(ratio));
        Serial.println();
        Serial.println("Concentration: " + String(concentration));
        Serial.println();
        lowpulseoccupancy = 0;
        //---------DHT Sensor---------
        Serial.println("Humidity: " + String(humidity));
        Serial.println();
        Serial.println("Temperature: " + String(temperature));
        Serial.println();
        Serial.println("Heat index: " + String(heat_index));
        Serial.println();
      publishData();
      lastPublishMillis = millis();
    }
    delay(10);  
}

// Start of IoT Functions:____________________________

void wifiConnect() {
  Serial.println("\n Connecting to: "); Serial.print(ssid);
  WiFi.begin(ssid, password);
  delay(4000);
  while (WiFi.status() != WL_CONNECTED) {
    delay(4000);
    Serial.print("... \n");
    wifiConnect();
    }
  Serial.print("nWiFi connected, IP address: "); Serial.println(WiFi.localIP());
  }

void mqttConnect() {
  if (!!!client.connected()) {
    Serial.print("Reconnecting MQTT client to "); Serial.println(server);
    while (!!!client.connect(clientId, authMethod, token)) {
      Serial.print("*");
      delay(500);
      }
    Serial.println();
    }
   }

void initManagedDevice() {
  if (client.subscribe("iotdm-1/response")) {
    Serial.println("subscribe to responses OK");
    } 
  else {
    Serial.println("subscribe to responses FAILED");
    }
  if (client.subscribe(rebootTopic)) {
    Serial.println("subscribe to reboot OK");
    } 
  else {
    Serial.println("subscribe to reboot FAILED");
    }
  if (client.subscribe("iotdm-1/device/update")) {
    Serial.println("subscribe to update OK");
    } 
  else {
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
    } 
  else {
    Serial.print("device Publish failed:");
    }
  }

void publishData() {
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print("Disconnected! \n Trying to connect again.");
    wifiConnect();
  }
  String payload = "{\"d\":{\"concentration\":";
  payload += String(concentration);
  payload += ",\"humidity\":";
  payload += String(humidity);
  payload += ",\"temperature\":";
  payload += String(temperature);
  payload += ",\"heat_index\":";
  payload += String(heat_index);
  payload += "}}";

  Serial.println();
  Serial.print("Publishing payload: "); 
  Serial.println(payload);

  if (client.publish(publishTopic, (char*) payload.c_str())) {
    Serial.println("Publish OK");
    }
  else {
    Serial.println("Publish FAILED");
    mqttConnect();
    initManagedDevice();
    }
  }

void callback(char* topic, byte* payload, unsigned int payloadLength) {
 Serial.print("callback invoked for topic: "); Serial.println(topic);

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
// End of IoT Functions:____________________________
