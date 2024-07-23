#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <WiFiClient.h>
#include <ArduinoJson.h>
#include <NTPClient.h>
#include <WiFiUdp.h>

#ifndef APSSID
#define APSSID "HSU_Students" // existing Wifi network
#define APPSK "dhhs12cnvch"
#endif

const char *ssid = APSSID;
const char *password = APPSK;
const char *URL = "http://10.106.27.117:1909/add";
WiFiClient client;
HTTPClient http;
ESP8266WebServer server(80);

WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "2.asia.pool.ntp.org", 7*3600);

#define MIC_PIN A0   // Analog pin connected to the microphone sensor
#define LED_PIN_1 D3 // Digital pin connected to the first LED
#define LED_PIN_2 D4 // Digital pin connected to the second LED
#define LED_PIN_3 D5

int clapCount;
int sensorValue;
int clapSensorValue = sensorValue;
char ledState1[] = "OFF";
char ledState2[] = "OFF";
char ledState3[] = "OFF";

unsigned long startTime;

void setup() {
  Serial.begin(115200);
  Serial.println();
  Serial.print("Connect to existing Wifi network...");
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());

  server.on("/", handleOnConnect);
  // server.enableCORS(true);
  server.begin();
  Serial.println("HTTP server started");
  delay(5000);

  timeClient.begin();

  pinMode(MIC_PIN, INPUT);
  pinMode(LED_PIN_1, OUTPUT);
  pinMode(LED_PIN_2, OUTPUT);
  pinMode(LED_PIN_3, OUTPUT);
}

void loop() {
  server.handleClient();

  sensorValue = analogRead(MIC_PIN);
  Serial.print("Received Sensor Value: ");
  Serial.println(sensorValue);

  if (sensorValue > 800 ) {
    Serial.print("Clap Sensor Value: ");
    clapSensorValue = sensorValue;
    Serial.println(sensorValue);
    startTime = millis();
    clapCount++;
    delay(500);

    while (millis() - startTime < 2000) {
      sensorValue = analogRead(MIC_PIN);
      if (sensorValue > 800) {
        clapCount++;
        delay(100);
      }
    }
  }

  Serial.print("Clap Count: ");
  Serial.println(clapCount);

  if (clapCount == 1) {
    digitalWrite(LED_PIN_1, !digitalRead(LED_PIN_1));  // Toggle LED state
    Serial.println("LED_1: " + String(digitalRead(LED_PIN_1) ? "ON" : "OFF"));
    strcpy(ledState1, digitalRead(LED_PIN_1) ? "ON" : "OFF");
    strcpy(ledState2, digitalRead(LED_PIN_2) ? "ON" : "OFF");
    strcpy(ledState3, digitalRead(LED_PIN_3) ? "ON" : "OFF");
    timeClient.update();
    postJsonData(clapSensorValue, clapCount, ledState1, ledState2, ledState3);
  } else if (clapCount == 2) {
    digitalWrite(LED_PIN_2, !digitalRead(LED_PIN_2));  // Toggle LED state
    Serial.println("LED_2: " + String(digitalRead(LED_PIN_2) ? "ON" : "OFF"));
    strcpy(ledState1, digitalRead(LED_PIN_1) ? "ON" : "OFF");
    strcpy(ledState2, digitalRead(LED_PIN_2) ? "ON" : "OFF");
    strcpy(ledState3, digitalRead(LED_PIN_3) ? "ON" : "OFF");
    timeClient.update();
    postJsonData(clapSensorValue, clapCount, ledState1, ledState2, ledState3);
  } else if (clapCount == 3) {
    digitalWrite(LED_PIN_3, !digitalRead(LED_PIN_3));  // Toggle LED state
    Serial.println("LED_3: " + String(digitalRead(LED_PIN_3) ? "ON" : "OFF"));
    strcpy(ledState1, digitalRead(LED_PIN_1) ? "ON" : "OFF");
    strcpy(ledState2, digitalRead(LED_PIN_2) ? "ON" : "OFF");
    strcpy(ledState3, digitalRead(LED_PIN_3) ? "ON" : "OFF");
    timeClient.update();
    postJsonData(clapSensorValue, clapCount, ledState1, ledState2, ledState3);
  }
  clapCount = 0;
}

void handleOnConnect()
{
  server.send(200, "text/html", "ok");
}

void postJsonData(int clapSensorValue, int clapCount, const char* ledState1, const char* ledState2, const char* ledState3)
{
  Serial.print("connecting to ");
  if (WiFi.status() == WL_CONNECTED)
  {
    Serial.print("[HTTP] begin...\n");
    if (http.begin(client, URL))
    {
      Serial.print("[HTTP] POST...\n");
      const int capacity = JSON_OBJECT_SIZE(2000);
      StaticJsonDocument<capacity> doc;
      // doc["id"] = "1";
      doc["clapSensorValue"] = clapSensorValue;
      doc["clapCount"] = clapCount;
      doc["led_1"] = ledState1;
      doc["led_2"] = ledState2;
      doc["led_3"] = ledState3;
      doc["createdAt"] = timeClient.getFormattedTime();
      char output[2048];
      serializeJson(doc, Serial);
      serializeJson(doc, output);
      http.addHeader("Content-Type", "application/json");
      int httpCode = http.POST(output);
      Serial.println();
      if(httpCode==200){
        Serial.print("Status: ");
        Serial.print(httpCode);
        Serial.println(" OK. Message: Successful POST!!! ");
      }else {
        Serial.println("Status: ERROR. Message: Failed POST!!!");
      }
      // delay(5000);
      http.end();
      Serial.println("closing connection");
    }
  }
}

