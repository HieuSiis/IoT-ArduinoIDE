#include <Servo.h>
#include <ESP8266WiFi.h>
#include <FirebaseESP8266.h>
#include <MFRC522.h>

#define SERVO_PIN D8
#define AP_SSID "Julia.Wifi"
#define AP_PASSWORD "9876543210"
#define DB_URL "https://iotlightsensor-default-rtdb.asia-southeast1.firebasedatabase.app/"
#define DB_SECRET "9MM6RlzfEO2g4azepHbw5GdjUoi71MLW1Ye2GQHt"
#define SDA_PIN_RFID D3
#define RST_PIN D4

Servo servo;
FirebaseConfig config;
FirebaseAuth auth;
FirebaseData fbdo;
MFRC522 mfrc522(SDA_PIN_RFID, RST_PIN);

int oldValue = 0;

void setup() {
  Serial.begin(115200);
  Serial.println("Starting setup...");

  initServo();
  initMFRC522();
  initWifi();
  initFirebase();
  setValueToFirebase(0);

  Serial.print("Initial free heap memory: ");
  Serial.println(ESP.getFreeHeap());

  Serial.println("Setup completed.");
}

void loop() {

  
  String uid = getCardUID();
  Serial.print("Card UID: ");
  Serial.println(uid);
  int newValue = getValueFromFirebase();
  if (newValue == 1 && oldValue == 0) {
    downServo();
    oldValue = 1;
  } else if (newValue == 0 && oldValue == 1) {
    upServo();
    oldValue = 0;
  }
  if (uid == "13115149166" && oldValue == 1) {
    Serial.println("blue card detected");
    upServo();
    setValueToFirebase(0);
    oldValue = 0;
  } else if (uid == "83246196149" && oldValue == 0) {
    Serial.println("white card detected");
    downServo();
    setValueToFirebase(1);
    oldValue = 1;
  }

  delay(100);
  Serial.print("Free heap memory: ");
  Serial.println(ESP.getFreeHeap());
}

void initServo() {
  Serial.println("Initializing servo...");
  servo.attach(SERVO_PIN);
  servo.write(0);
}

void upServo() {
  Serial.println("Moving servo up...");
  for (int pos = 0; pos <= 180; pos += 1) {
    servo.write(pos);
    delay(15);
    yield();
  }
}

void downServo() {
  Serial.println("Moving servo down...");
  for (int pos = 180; pos >= 0; pos -= 1) {
    servo.write(pos);
    delay(15);
    yield();
  }
}

void initMFRC522() {
  Serial.println("Initializing MFRC522...");
  SPI.begin();
  mfrc522.PCD_Init();
}

String getCardUID() {
  String uid = "";
  if (mfrc522.PICC_IsNewCardPresent() && mfrc522.PICC_ReadCardSerial()) {
    uid = getCardUIDinDEC(mfrc522.uid.uidByte, mfrc522.uid.size);
    mfrc522.PICC_HaltA();
  }
  return uid;
}

String getCardUIDinDEC(byte* buffer, byte bufferSize) {
  String result = "";
  for (byte i = 0; i < bufferSize; i++) {
    result += String(buffer[i]);
  }
  return result;
}

void initWifi() {
  Serial.print("Connecting to WiFi: ");
  Serial.println(AP_SSID);
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

void initFirebase() {
  Serial.println("Initializing Firebase...");
  config.database_url = DB_URL;
  config.signer.tokens.legacy_token = DB_SECRET;
  Firebase.begin(&config, &auth);
}

void setValueToFirebase(int value) {
  if (Firebase.ready()) {
    if (!Firebase.setInt(fbdo, "SERVO/value", value)) {
      Serial.println("Failed to set value to Firebase");
      Serial.println(fbdo.errorReason());
    } else {
      Serial.println("Value set to Firebase successfully");
    }
  } else {
    Serial.println("Firebase not ready");
  }
}

int getValueFromFirebase() {
  int value = -1;
  if (Firebase.ready()) {
    if (Firebase.getInt(fbdo, "SERVO/value")) value = fbdo.intData();
    Serial.println("value: " + String(value));
  }
  return value;
}