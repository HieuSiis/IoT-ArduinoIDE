#include <ESP8266WiFi.h>
#include <FirebaseESP8266.h>
#include <UniversalTelegramBot.h>
#include <WiFiClientSecure.h>

#define AP_SSID "Julia.Wifi"
#define AP_PASSWORD "9876543210"

/* FIREBASE */
#define DB_URL "https://iotlightsensor-default-rtdb.asia-southeast1.firebasedatabase.app/"  // Realtime Database > Data > URL
#define DB_SECRET "9MM6RlzfEO2g4azepHbw5GdjUoi71MLW1Ye2GQHt"                                // Project settings > Service accounts > Database secrets
FirebaseConfig config;
FirebaseAuth auth;
FirebaseData fbdo;  // firebase data object // setup to run only once

/* Epoch time */
#include <time.h>
#define NTP_SERVER "pool.ntp.org"

/* TELEGRAM */
#define BOTtoken "6845264953:AAEv7xzJOwsUQOrAHaSA3TX9gUS4jSajJ0c"
#define CHAT_ID "6354366568"
WiFiClientSecure client;
UniversalTelegramBot bot(BOTtoken, client);
X509List cert(TELEGRAM_CERTIFICATE_ROOT);

#define PIR D0
#define Flame D1
#define Obstacle D2
#define Gas D5
#define Moisture D4
#define Button D8

#define Motor D3
#define Led D6
#define BUZZER D7

int lastButtonState = LOW;
bool motorState;

void setup() {
  Serial.begin(115200);

  initBuzzer();
  initWifi();
  initFirebase();
  initTelegram();
  //initEpochTime();

  pinMode(PIR, INPUT);
  pinMode(Flame, INPUT);
  pinMode(Obstacle, INPUT);
  pinMode(Moisture, INPUT);
  pinMode(Button, INPUT);
  pinMode(Gas, INPUT);

  pinMode(Motor, OUTPUT);
  pinMode(Led, OUTPUT);
  pinMode(BUZZER, OUTPUT);
}

enum ControlState {
  NONE,
  BUTTON,
  MOISTURE,
  FIRE,
  OBSTACLE
};

enum ControlStateBuzzer {
  BUZZER_NONE,
  BUZZER_FIRE,
  BUZZER_RADAR
};

ControlState currentControl = NONE;
ControlStateBuzzer currentControlBuzzer = BUZZER_NONE;

void loop() {

  int valueStatus = getValueStatusFromFirebase();
  if (valueStatus == false) {
    int valueBuzzer = getValueBuzzerFromFirebase();
    if (valueBuzzer) {
      sosBuzzer();
    } else {
      offBuzzer();
    }

    int valueLight = getValueLightFromFirebase();
    if (valueLight) {
      digitalWrite(Led, HIGH);
    } else {
      digitalWrite(Led, LOW);
    }

    int valueWaterPump = getValueWaterPumpFromFirebase();
    if (valueWaterPump) {
      digitalWrite(Motor, HIGH);
    } else {
      digitalWrite(Motor, LOW);
    }
  } else {
    int motion = digitalRead(PIR);
    if (motion == 1) {
      Serial.println("Motion: +");
      digitalWrite(Led, HIGH);
      setLightToFirebase(1);
    } else {
      Serial.println("Motion: -");
      digitalWrite(Led, LOW);
      setLightToFirebase(0);
    }

    int currentButtonState = digitalRead(Button);
    // Check button press to toggle motor state
    if (currentButtonState == HIGH && lastButtonState == LOW) {

      if (currentControl == NONE || currentControl == BUTTON) {
        if (!motorState) {
          setWaterPumpToFirebase(1);
          digitalWrite(Motor, HIGH);
          Serial.println("Button: +");
          motorState = true;
          currentControl = BUTTON;
        } else {
          setWaterPumpToFirebase(0);
          digitalWrite(Motor, LOW);
          Serial.println("Button: -");
          motorState = false;
          currentControl = NONE;
        }
      }
    }
    lastButtonState = currentButtonState;
    // Đọc giá trị cảm biến độ ẩm
    int moisture = digitalRead(Moisture);
    if (moisture == 1) {
      Serial.println("Moisture: +");
      if (currentControl == MOISTURE || currentControl == NONE) {
        setWaterPumpToFirebase(1);
        digitalWrite(Motor, HIGH);
        currentControl = MOISTURE;
      }
    } else if (moisture == 0) {
      Serial.println("Moisture: -");
      if (currentControl == MOISTURE || currentControl == NONE) {
        setWaterPumpToFirebase(0);
        digitalWrite(Motor, LOW);
        currentControl = NONE;
      }
    }

    int gas = digitalRead(Gas);
    if (gas == 0) {
      Serial.println("Gas: +");

      if (currentControlBuzzer == BUZZER_NONE || currentControlBuzzer == BUZZER_RADAR) {

        setBuzzerToFirebase(1);
        sosBuzzer();
        currentControlBuzzer = BUZZER_RADAR;
      }
      bot.sendMessage(CHAT_ID, "Detected Gas !!!", "");

    } else if (gas == 1) {
      Serial.println("Gas: -");
      if (currentControlBuzzer == BUZZER_NONE || currentControlBuzzer == BUZZER_RADAR) {
        setBuzzerToFirebase(0);
        offBuzzer();
        currentControlBuzzer = BUZZER_NONE;  // Reset State
      }
    }

    // Đọc giá trị cảm biến lửa
    int fire = digitalRead(Flame);
    if (fire == 1) {
      Serial.println("Fire: -");
      if (currentControlBuzzer == BUZZER_NONE || currentControlBuzzer == BUZZER_FIRE) {
        setBuzzerToFirebase(0);
        offBuzzer();
        currentControlBuzzer = BUZZER_NONE;
      }
      if (currentControl == FIRE || currentControl == NONE) {
        setWaterPumpToFirebase(0);
        digitalWrite(Motor, LOW);
        currentControl = NONE;  // Reset State
      }
    } else if (fire == 0) {
      Serial.println("Fire: +");
      if (currentControlBuzzer == BUZZER_NONE || currentControlBuzzer == BUZZER_FIRE) {
        setBuzzerToFirebase(1);
        sosBuzzer();
        currentControlBuzzer == BUZZER_FIRE;
      }

      if (currentControl == NONE || currentControl == FIRE) {
        setWaterPumpToFirebase(1);
        digitalWrite(Motor, HIGH);
        currentControl = FIRE;
      }
      bot.sendMessage(CHAT_ID, "Detected Fire !!!", "");
    }

    // Đọc giá trị cảm biến chướng ngại vật
    int obstacle = digitalRead(Obstacle);
    if (obstacle == 1) {
      Serial.println("Obstacle: -");
      if (currentControl == OBSTACLE || currentControl == NONE) {
        setWaterPumpToFirebase(0);
        digitalWrite(Motor, LOW);
        currentControl = NONE;
      }
    } else if (obstacle == 0) {
      Serial.println("Obstacle: +");
      if (currentControl == OBSTACLE || currentControl == NONE) {
        setWaterPumpToFirebase(1);
        digitalWrite(Motor, HIGH);
        currentControl = OBSTACLE;
      }
    }
  }

  setTimeToFirebase();

  delay(100);
}

bool readDOLight() {
  int dValue = digitalRead(Led);
  if (dValue) return 0;
  else return 1;
}

void initBuzzer() {
  pinMode(BUZZER, OUTPUT);
  offBuzzer();
}

void sosBuzzer() {
  for (int i = 1; i <= 3; i++) {
    digitalWrite(BUZZER, LOW);
    delay(50);
    digitalWrite(BUZZER, HIGH);
    delay(100);
  }
  delay(100);
  for (int i = 1; i <= 3; i++) {
    digitalWrite(BUZZER, LOW);
    delay(200);
    digitalWrite(BUZZER, HIGH);
    delay(100);
  }
  delay(100);
  for (int i = 1; i <= 3; i++) {
    digitalWrite(BUZZER, LOW);
    delay(50);
    digitalWrite(BUZZER, HIGH);
    delay(100);
  }
}

void offBuzzer() {
  digitalWrite(BUZZER, HIGH);
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

void initTelegram() {
  configTime(0, 0, "pool.ntp.org");
  client.setTrustAnchors(&cert);
  bot.sendMessage(CHAT_ID, "Bot started up", "");
}

/* FIREBASE */
void initFirebase() {
  config.database_url = DB_URL;
  config.signer.tokens.legacy_token = DB_SECRET;
  Firebase.begin(&config, &auth);
  Serial.print("\nWiFi connected, IP address: ");
  Firebase.reconnectNetwork(true);
}

int getValueBuzzerFromFirebase() {
  int value = 0;
  if (Firebase.ready()) {
    if (Firebase.getInt(fbdo, "BUZZER/value")) value = fbdo.intData();
  }
  return value;
}

int getValueLightFromFirebase() {
  int value = 0;
  if (Firebase.ready()) {
    if (Firebase.getInt(fbdo, "LIGHT/value")) value = fbdo.intData();
  }
  return value;
}

int getValueWaterPumpFromFirebase() {
  int value = 0;
  if (Firebase.ready()) {
    if (Firebase.getInt(fbdo, "WATERPUMP/value")) value = fbdo.intData();
  }
  return value;
}

int getValueStatusFromFirebase() {
  int value = 0;
  if (Firebase.ready()) {
    if (Firebase.getInt(fbdo, "STATUS/auto")) value = fbdo.intData();
  }
  return value;
}

void initEpochTime() {
  configTime(0, 0, NTP_SERVER);  // GMT time offset, daylight saving time, NTP server
}

long getEpochTime() {
  tm infoTime;
  if (!getLocalTime(&infoTime)) return 0;
  time_t epochTime;
  time(&epochTime);
  return epochTime;
}

void setLightToFirebase(int light) {

  if (Firebase.ready()) {
    Firebase.setInt(fbdo, "LIGHT/value", light);
  }
}

void setTimeToFirebase() {
  if (Firebase.ready()) {
    Firebase.setInt(fbdo, "TIME/value", getEpochTime());
  }
}

void setBuzzerToFirebase(int buzzer) {

  if (Firebase.ready()) {
    Firebase.setInt(fbdo, "BUZZER/value", buzzer);
  }
}

void setWaterPumpToFirebase(int waterPump) {

  if (Firebase.ready()) {
    Firebase.setInt(fbdo, "WATERPUMP/value", waterPump);
  }
}
