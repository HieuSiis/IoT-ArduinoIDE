#include <SoftwareSerial.h>
#include "DHT.h"

#define TX_PIN D7
#define RX_PIN D6

#define DHTTYPE DHT11
#define DATA_DHT D5

#define PIR D9      // D4
#define LED_PIR D8  // D3

#define FAN D0          // RX
#define LED_GREEN D4    // D2/
#define LED_YELLOW D3   // D1/
#define LED_RED D2      // D0
#define BUTTON_FAN D10  // D8/

String data;
int numberOfPresses = 1;


SoftwareSerial bluetooth(RX_PIN, TX_PIN);
DHT dht(DATA_DHT, DHTTYPE);

void setup() {
  Serial.begin(9600);
  bluetooth.begin(9600);

  pinMode(RX_PIN, INPUT);
  pinMode(TX_PIN, OUTPUT);

  dht.begin();

  pinMode(BUTTON_FAN, INPUT);
  pinMode(FAN, OUTPUT);
  pinMode(LED_GREEN, OUTPUT);
  pinMode(LED_YELLOW, OUTPUT);
  pinMode(LED_RED, OUTPUT);

  pinMode(PIR, INPUT);
  pinMode(LED_PIR, OUTPUT);
}

void loop() {

  if (bluetooth.available() > 0) {
    char c = bluetooth.read();
    switch (c) {
      case '0':
        data = "off";
        break;
      case '1':
        data = "on";
        break;
      default:
        break;
    }
  }

  float h = dht.readHumidity();
  float t = dht.readTemperature();

  Serial.print("Humidity: ");
  Serial.print(h);
  Serial.println("%");
  Serial.print("Temperature: ");
  Serial.print(t);
  Serial.println("*C");

  bluetooth.print("\nHumidity: ");
  bluetooth.print(h);
  bluetooth.println(" %");

  bluetooth.print("Temperature: ");
  bluetooth.print(t);
  bluetooth.println(" *C");

  if (isnan(h) || isnan(t)) {
    Serial.println("Failed to read from DHT sensor!");
    return;
  }

  int detect = digitalRead(PIR);

  if (detect == 1) {
    digitalWrite(LED_PIR, HIGH);
    Serial.println("PIR: Motion detected. Light On");
    bluetooth.println("\nPIR: Motion detected. Light On");
  } else {
    digitalWrite(LED_PIR, LOW);
    Serial.println("PIR: No motion detected. Light Off");
    bluetooth.println("\nPIR: No motion detected. Light Off");
  }

  if (h >= 40 && h <= 60) {
    digitalWrite(LED_GREEN, HIGH);
    bluetooth.println("Humidity Status: Good");
    digitalWrite(LED_YELLOW, LOW);
    digitalWrite(LED_RED, LOW);
  } else if (h < 40 && h >= 30 || h > 60 && h <= 70) {
    digitalWrite(LED_GREEN, LOW);
    digitalWrite(LED_YELLOW, HIGH);
    bluetooth.println("Humidity Status: Temporary");
    digitalWrite(LED_RED, LOW);
  } else {
    digitalWrite(LED_GREEN, LOW);
    digitalWrite(LED_YELLOW, LOW);
    digitalWrite(LED_RED, HIGH);
    bluetooth.println("Humidity: Bad");
  }

  int buttonPress = digitalRead(BUTTON_FAN);

  if (buttonPress == 1) {
    if (digitalRead(FAN) == HIGH) {
      digitalWrite(FAN, LOW);
      numberOfPresses--;
      Serial.println("Button_Fan OFF");
      bluetooth.println("Button_Fan: OFF");
    } else {
      digitalWrite(FAN, HIGH);
      numberOfPresses++;
      Serial.println("Button_Fan ON");
      bluetooth.println("Button_Fan: ON");
    }
  }
  if (numberOfPresses == 1) {
    if (data.length() > 0) {
      if (data == "on" || data == "1") {
        digitalWrite(FAN, HIGH);
        Serial.println("Data_Fan ON");
        bluetooth.println("Data_Fan: ON");
        // data = "";
      } else if (data == "off" || data == "0") {
        digitalWrite(FAN, LOW);
        Serial.println("Data_Fan OFF");
        bluetooth.println("Data_Fan: OFF");
        delay(5000);
        data = "";
      }
    } else {
      if (t > 25) {
        digitalWrite(FAN, HIGH);
        Serial.println("Auto_Fan ON");
        bluetooth.println("Auto_Fan: ON");
      } else {
        digitalWrite(FAN, LOW);
        Serial.println("Auto_Fan OFF");
        bluetooth.println("Auto_Fan: OFF");
      }
    }
  }
  delay(2000);
}
