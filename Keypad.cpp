#include <Servo.h>
#define SERVO_PIN D8
Servo servo;

#include <ESP8266WiFi.h>

#define AP_SSID "Julia.Wifi"
#define AP_PASSWORD "9876543210"

/* FIREBASE */
#include <FirebaseESP8266.h>                                                                // ref: https://github.com/mobizt/Firebase - ESP8266 #elif defined (ESP32)
#define DB_URL "https://iotlightsensor-default-rtdb.asia-southeast1.firebasedatabase.app/"  // Realtime Database > Data > URL
#define DB_SECRET "9MM6RlzfEO2g4azepHbw5GdjUoi71MLW1Ye2GQHt"                                // Project settings > Service accounts > Database secrets
FirebaseConfig config;
FirebaseAuth auth;
FirebaseData fbdo;  // firebase data object // setup to run only once


#include <Keypad.h>
#define ROWS 4  // four rows
#define COLS 4  // four columns

#include <SSD1306.h>
#define SDA_PIN SDA
#define SCL_PIN SCL
SSD1306 oled(0x3c, SDA_PIN, SCL_PIN);




void setup() {
  Serial.begin(115200);
  initOled();
  initServo();
  initWifi();
  initFirebase();
  setValueToFirebase(0);
}

byte rowPins[ROWS] = { D3, D2, D1, D0 };  // connect to the row pinouts of the keypad
byte colPins[COLS] = { D4, D5, D6, D7 };  // connect to the column pinouts of the keypad

char keys[ROWS][COLS] = {
  { '1', '2', '3', 'A' },
  { '4', '5', '6', 'B' },
  { '7', '8', '9', 'C' },
  { '*', '0', '#', 'D' }
};

Keypad keypad = Keypad(makeKeymap(keys), rowPins, colPins, ROWS, COLS);

String pwd = "6789";
int pos = 0;

void loop() {
  char key = keypad.getKey();  // '*#': down, 'pwd': up
  int newValue = getValueFromFirebase();
  Serial.print(newValue);
  printOled("Keypad", String(key));
  // Serial.println(key);
  if (key == '*' || key == '#') {
    Serial.println(key);  // for DEBUG
    // printOled("Keypad", String(key));
    setValueToFirebase(0);
    downServo();
    pos = 0;
  } else if (key == pwd[pos]) {
    Serial.print(key);  // for DEBUG
    // printOled("Keypad", String(key));
    pos++;
    if (pos == pwd.length()) {  // correct password
    setValueToFirebase(1);
      upServo();
      
      pos = 0;
    }
  }
  delay(100);

  // char key = keypad.getKey();
  // if (key) {
  //   Serial.println(key);
  //   printOled("Keypad", String(key));
  // }
  // delay(100);
}

void initOled() {
  oled.init();
  oled.flipScreenVertically();
}

void printOled(String line1, String line2) {
  oled.clear();
  oled.setTextAlignment(TEXT_ALIGN_LEFT);
  oled.setFont(ArialMT_Plain_24);
  oled.drawString(0, 0, line1);
  oled.drawString(0, 36, line2);
  oled.display();
}

void initServo() {
  servo.attach(SERVO_PIN);
  servo.write(0);
}

void upServo() {
  for (int pos = 0; pos <= 180; pos += 1) {
    servo.write(pos);
  }
}

void downServo() {
  for (int pos = 180; pos >= 0; pos -= 1) {
    servo.write(pos);
  }
}
void initWifi() {

  blinkBuiltinLed(10);
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
  blinkBuiltinLed(10);
}

void blinkBuiltinLed(int n) {
  pinMode(BUILTIN_LED, OUTPUT);
  for (int i = 1; i <= n; i++) {
#if defined(ESP8266)
    digitalWrite(BUILTIN_LED, LOW);
    delay(200);
    digitalWrite(BUILTIN_LED, HIGH);
    delay(200);
#elif defined(ESP32)
    digitalWrite(BUILTIN_LED, HIGH);
    delay(200);
    digitalWrite(BUILTIN_LED, LOW);
    delay(200);
#endif
  }
}

/* FIREBASE */
void initFirebase() {
  config.database_url = DB_URL;
  config.signer.tokens.legacy_token = DB_SECRET;
  Firebase.begin(&config, &auth);
  Serial.print("\nWiFi connected, IP address: ");
  Firebase.reconnectNetwork(true);
}

int getValueFromFirebase() {
  int value = -1;
  if (Firebase.ready()) {
    if (Firebase.getInt(fbdo, "SERVO/value")) value = fbdo.intData();
    Serial.println("value: " + String(value));
  }
  return value;
}

void setValueToFirebase(int value) {

  if (Firebase.ready()) {
    Firebase.setInt(fbdo, "SERVO/value", value);
  }
}