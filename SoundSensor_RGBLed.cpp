#include <ESP8266WiFi.h>
#include <FirebaseESP8266.h>
#include <WiFiClientSecure.h>

#define AP_SSID "Julia.Wifi"
#define AP_PASSWORD "9876543210"

/* FIREBASE */
#define DB_URL "https://iotlightsensor-default-rtdb.asia-southeast1.firebasedatabase.app/"
#define DB_SECRET "9MM6RlzfEO2g4azepHbw5GdjUoi71MLW1Ye2GQHt"
FirebaseConfig config;
FirebaseAuth auth;
FirebaseData fbdo;

/* Sound sensor */
// #define D0_PIN D5
#define A0_PIN A0

#define RED_LED_PIN D6
#define GREEN_LED_PIN D7
#define BLUE_LED_PIN D8

bool flashingMode = false;  // Global flag for flashing mode
int red = 0, green = 0, blue = 0;

/* Sound sensor */
int clap = 0;
long detection_range_start = 0;
long detection_range = 0;

void setup() {
  Serial.begin(115200);
  initRGBLed();
  initWifi();
  initFirebase();
}

void loop() {
  int valueStatus = getValueStatusRGBFromFirebase();
  if (valueStatus == false) {
    int valueAllRGB = getValueAllRGBFromFirebase();
    if (valueAllRGB == 1) {
      flashingMode = true;
      flashingRGBLed();
    } else {
      getRGBfromFirebase(red, green, blue);
      onRGBLed(red, green, blue);
    }
  } else {
    if (!flashingMode) {
      readDO();
    } else {
      flashingRGBLed();
    }
  }
  delay(100);
}

void readDO() {
  // int dValue = digitalRead(D0_PIN);  // 0:sound; 1:no-sound
  int aValue = analogRead(A0_PIN);  // 0:sound; 1:no-sound
      Serial.println(aValue);

  if (aValue > 500) {
    if (clap == 0) {
      detection_range_start = detection_range = millis();
      clap++;
    } else if (clap > 0 && millis() - detection_range >= 100) {
      detection_range = millis();
      clap++;
    }
  }

  if (millis() - detection_range_start >= 500) {
    if (clap == 1) {
      switchClapAction();
    } else if (clap == 2) {
      Serial.println("CLAP 2 times");
      onRGBLed(0, 0, 0);
      flashingMode = false;
      red = 0;
      green = 0;
      blue = 0;
      setRGBToFirebase(red, green, blue);
    }
    clap = 0;
  }
}

void switchClapAction() {
  static int singleClapCount = 0;
  singleClapCount++;
  if (singleClapCount == 1) {
    Serial.println("CLAP 1 time - Red LED");
    flashingMode = false;
    onRGBLed(1, 0, 0);
    red = 1;
    green = 0;
    blue = 0;
    setRGBToFirebase(red, green, blue);
  } else if (singleClapCount == 2) {
    Serial.println("CLAP 2 times - Green LED");
    flashingMode = false;
    onRGBLed(0, 1, 0);
    red = 0;
    green = 1;
    blue = 0;
    setRGBToFirebase(red, green, blue);
  } else if (singleClapCount == 3) {
    Serial.println("CLAP 3 times - Blue LED");
    flashingMode = false;
    onRGBLed(0, 0, 1);
    red = 0;
    green = 0;
    blue = 1;
    setRGBToFirebase(red, green, blue);
  } else if (singleClapCount == 4) {
    Serial.println("CLAP 4 times - Flashing LEDs");
    flashingMode = true;  // Enter flashing mode
    singleClapCount = 0;
  }
}

void flashingRGBLed() {
  // Loop periodically to allow state check
  while (flashingMode) {
    // Red blinks 10 times
    for (int i = 0; i < 10 && flashingMode; i++) {
      onRGBLed(1, 0, 0);
      red = 1;
      green = 0;
      blue = 0;
      setRGBToFirebase(red, green, blue);
      delay(100);
      onRGBLed(0, 0, 0);
      red = 0;
      green = 0;
      blue = 0;
      setRGBToFirebase(red, green, blue);
      delay(100);
      readDO();  // Check for clap input
      int valueStatus = getValueStatusRGBFromFirebase();
      if (valueStatus == false) {
        int valueAllRGB = getValueAllRGBFromFirebase();
        if (valueAllRGB == 0) {
          flashingMode = false;
        }
      }
    }

    // Green blinks 7 times
    for (int i = 0; i < 7 && flashingMode; i++) {
      onRGBLed(0, 1, 0);
      red = 0;
      green = 1;
      blue = 0;
      setRGBToFirebase(red, green, blue);
      delay(100);
      onRGBLed(0, 0, 0);
      red = 0;
      green = 0;
      blue = 0;
      setRGBToFirebase(red, green, blue);
      delay(100);
      readDO();  // Check for clap input
      int valueStatus = getValueStatusRGBFromFirebase();
      if (valueStatus == false) {
        int valueAllRGB = getValueAllRGBFromFirebase();
        if (valueAllRGB == 0) {
          flashingMode = false;
        }
      }
    }

    // Blue blinks 3 times
    for (int i = 0; i < 3 && flashingMode; i++) {
      onRGBLed(0, 0, 1);
      red = 0;
      green = 0;
      blue = 1;
      setRGBToFirebase(red, green, blue);
      delay(100);
      onRGBLed(0, 0, 0);
      red = 0;
      green = 0;
      blue = 0;
      setRGBToFirebase(red, green, blue);
      delay(100);
      readDO();  // Check for clap input
      int valueStatus = getValueStatusRGBFromFirebase();
      if (valueStatus == false) {
        int valueAllRGB = getValueAllRGBFromFirebase();
        if (valueAllRGB == 0) {
          flashingMode = false;
        }
      }
    }
  }
}

void onRGBLed(int red, int green, int blue) {
  digitalWrite(RED_LED_PIN, red == 1 ? HIGH : LOW);
  digitalWrite(GREEN_LED_PIN, green == 1 ? HIGH : LOW);
  digitalWrite(BLUE_LED_PIN, blue == 1 ? HIGH : LOW);
}

void initRGBLed() {
  pinMode(RED_LED_PIN, OUTPUT);
  pinMode(GREEN_LED_PIN, OUTPUT);
  pinMode(BLUE_LED_PIN, OUTPUT);
}

void initWifi() {
  Serial.print("\nConnecting to ");
  Serial.print(AP_SSID);
  WiFi.begin(AP_SSID, AP_PASSWORD);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.print("\nWiFi connected, IP address: ");
  Serial.println(WiFi.localIP());
  WiFi.setAutoReconnect(true);
  WiFi.persistent(true);
}

/* FIREBASE */
void initFirebase() {
  config.database_url = DB_URL;
  config.signer.tokens.legacy_token = DB_SECRET;
  Firebase.begin(&config, &auth);
  Firebase.reconnectNetwork(true);
}

int getValueStatusRGBFromFirebase() {
  int value = 1;
  if (Firebase.ready()) {
    if (Firebase.getInt(fbdo, "STATUSRGB/auto")) value = fbdo.intData();
  }
  return value;
}

void setRGBToFirebase(int red, int green, int blue) {
  if (Firebase.ready()) {
    Firebase.setInt(fbdo, "LEDs/R", red);
    Firebase.setInt(fbdo, "LEDs/G", green);
    Firebase.setInt(fbdo, "LEDs/B", blue);
  }
}

void getRGBfromFirebase(int& red, int& green, int& blue) {
  if (Firebase.ready()) {
    bool success = true;
    if (!Firebase.getInt(fbdo, "LEDs/R")) {
      Serial.println("Failed to get red value from Firebase");
      success = false;
    } else {
      red = fbdo.intData();
    }
    if (!Firebase.getInt(fbdo, "LEDs/G")) {
      Serial.println("Failed to get green value from Firebase");
      success = false;
    } else {
      green = fbdo.intData();
    }
    if (!Firebase.getInt(fbdo, "LEDs/B")) {
      Serial.println("Failed to get blue value from Firebase");
      success = false;
    } else {
      blue = fbdo.intData();
    }
    if (success) {
      Serial.println("RGB values retrieved successfully");
    } else {
      Serial.println("RGB values retrieval failed");
    }
    Serial.println("RGB: " + String(red) + ", " + String(green) + ", " + String(blue));  // for DEBUG
  }
}

int getValueAllRGBFromFirebase() {
  int value = 1;
  if (Firebase.ready()) {
    if (Firebase.getInt(fbdo, "LEDall/value")) value = fbdo.intData();
  }
  return value;
}
