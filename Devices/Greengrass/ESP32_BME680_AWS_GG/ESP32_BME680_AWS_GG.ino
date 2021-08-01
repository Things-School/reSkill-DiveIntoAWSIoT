#include <Wire.h>
#include <SPI.h>
#include <Adafruit_Sensor.h>
#include "Adafruit_BME680.h"
#include "secrets.h"
#include <WiFiClientSecure.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <NTPClient.h>
#include <AWSGreenGrassIoT.h>

Adafruit_BME680 bme; // I2C

WiFiUDP ntpUDP; // UDP client
NTPClient timeClient(ntpUDP); // NTP client

WiFiClientSecure net = WiFiClientSecure();
PubSubClient client(net);
AWSGreenGrassIoT * greengrass;
bool ConnectedToGGC = false;
bool SubscribedToGGC = false;

void connectWiFi()
{
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  Serial.println("Connecting to Wi-Fi");

  while (WiFi.status() != WL_CONNECTED){
    delay(500);
    Serial.print(".");
  }

  timeClient.begin(); // init NTP
  timeClient.setTimeOffset(0); // 0= GMT, 3600 = GMT+1
  Serial.println("Time Configured!");
}

void ConnToGGC(){

    ConnectedToGGC = false;
    SubscribedToGGC = false;

     while(ConnectedToGGC == false){
      
        greengrass = new AWSGreenGrassIoT(AWS_IOT_ENDPOINT, THINGNAME, aws_root_ca, thingCA, thingKey);

        if(greengrass->connectToGG() == true)
        {
            Serial.println("Connected to AWS GreenGrass");
            ConnectedToGGC = true;                              //Set flag to true    
               
               while(SubscribedToGGC == false){
                       if(greengrass->subscribe(AWS_IOT_SUBSCRIBE_TOPIC,subscribeCallback) == true) {
                            Serial.print("Subscribe to topic ");
                            Serial.print(AWS_IOT_SUBSCRIBE_TOPIC);
                            Serial.println(" was successful");
                            SubscribedToGGC = true;
                       }
                       else{
                            Serial.print("Subscribe to topic ");
                            Serial.print(AWS_IOT_SUBSCRIBE_TOPIC);
                            Serial.println(" failed");
                            SubscribedToGGC = false;
                       }
                       delay(1000); //wait for retry
               }
            
         }
        else
        {
            Serial.println("Connection to Greengrass failed, check if Greengrass core is on and connected to the WiFi");
        }
    }
}

void publishMessage()
{
  
  StaticJsonDocument<200> doc;
  long unsigned int timestamp = timeClient.getEpochTime();
  doc["timestamp"] = timestamp;
  doc["Device"] = "ESP32";
  doc["Sensor"] = "BME680";
  doc["Device_ID"] = THINGNAME;
  doc["temperature"] = bme.temperature;
  doc["pressure"] = bme.pressure / 100.0;
  doc["humidity"] = bme.humidity;
  doc["gasResistance"] = bme.gas_resistance / 1000.0;
  doc["altitude"] = bme.readAltitude(SEALEVELPRESSURE_HPA);
  
  char jsonBuffer[512];
  serializeJson(doc, jsonBuffer); // print to client

  if(greengrass->publish(AWS_IOT_PUBLISH_TOPIC,jsonBuffer)) { 
      Serial.print("Publish Message:");
      Serial.println(jsonBuffer);
    }
    else {
      Serial.println("Publish failed");
    }  
}

static void subscribeCallback (int topicLen, char *topicName, int payloadLen, char *payLoad) {
    Serial.println("subscribeCallback");
}


void setup() {
  Serial.begin(115200);
  while (!Serial);
  Serial.println(F("BME680 test"));
  if (!bme.begin())
  {
    Serial.println("Could not find a valid BME680 sensor, check wiring!");
    while (1);
    }
  // Set up oversampling and filter initialization
  bme.setTemperatureOversampling(BME680_OS_8X);
  bme.setHumidityOversampling(BME680_OS_2X);
  bme.setPressureOversampling(BME680_OS_4X);
  bme.setIIRFilterSize(BME680_FILTER_SIZE_3);
  bme.setGasHeater(320, 150); // 320*C for 150 ms
  
  connectWiFi();
  ConnToGGC();
}
void loop()
{
  while(!timeClient.update()) 
  {
    timeClient.forceUpdate();
  }
  if (greengrass->isConnected()){
      publishMessage();
      }
    else{
        Serial.println("Greengrass not connected");
        ConnToGGC();
    }
  delay(publish_frequency);
}
