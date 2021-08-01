#include <Wire.h>
#include <SPI.h>
#include <Adafruit_Sensor.h>
#include "Adafruit_BME680.h"
#include "secrets.h"
#include <WiFiClientSecure.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <NTPClient.h>


WiFiUDP ntpUDP; // UDP client
NTPClient timeClient(ntpUDP); // NTP client

WiFiClientSecure net = WiFiClientSecure();
PubSubClient client(net);


Adafruit_BME680 bme; // I2C

void connectAWS()
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
  
 // Configure WiFiClientSecure to use the AWS IoT device credentials
  net.setCACert(aws_root_ca);
  net.setCertificate(thingCA);
  net.setPrivateKey(thingKey);

  // Connect to the MQTT broker on the AWS endpoint we defined earlier
  client.setServer(AWS_IOT_ENDPOINT, 8883);

  // Create a message handler
  client.setCallback(messageHandler);

  Serial.print("Connecting to AWS IOT");

  while (!client.connect(THINGNAME)) {
    Serial.print(".");
    delay(100);
  }

  if(!client.connected()){
    Serial.println("AWS IoT Timeout!");
    return;
  }

  // Subscribe to a topic
  //client.subscribe(AWS_IOT_SUBSCRIBE_TOPIC);

  Serial.println("AWS IoT Connected!");
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

  client.publish(AWS_IOT_PUBLISH_TOPIC, jsonBuffer);
  Serial.println("Published!");
  Serial.println(jsonBuffer);
}

void messageHandler(char* topic, byte* payload, unsigned int length) {
  Serial.print("incoming: ");
  Serial.println(topic);

  StaticJsonDocument<200> doc;
  deserializeJson(doc, payload);
  const char* message = doc["message"];
  Serial.println(message);
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

  connectAWS();
}


void loop()
{
  while(!timeClient.update()) 
  {
    timeClient.forceUpdate();
  }
  publishMessage();
  client.loop();
  delay(UPLOAD_FREQUENCY);
}
