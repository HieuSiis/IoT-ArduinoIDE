#include <Stepper.h>
#include <DHT11.h>  // ref: https://github.com/dhrubasaha08/DHT11

#include <ESP8266WiFi.h>
#include <FirebaseESP8266.h>
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

const int stepsPerRevolution = 2048;
const int stepsPerThreeQuartersRevolution = stepsPerRevolution * 3 / 4;
#define DHT_PIN D2
Stepper myStepper(stepsPerRevolution, D5, D7, D6, D8);
DHT11 dht11(DHT_PIN);

#define ButtonFan D1
#define FAN D3

#define Rain D4
#define ButtonMotorStep D0

int lastButtonState = LOW;
bool fanState;

int lastButtonStateMotorStep = LOW;
bool motorStepState;

int lastRainState = LOW;

enum ControlState {
  NONE,
  BUTTON_FAN,
  DHT,
};

enum ControlStateMotorStep {
  NONE_CONTROL,
  BUTTON_MOTOR,
  RAIN,
};

ControlState currentControl = NONE;
ControlStateMotorStep currentControlMotorStep = NONE_CONTROL;


int temp = 0, humi = 0;

void setup() {
  Serial.begin(115200);
  initWifi();
  initFirebase();

  myStepper.setSpeed(10);

  pinMode(ButtonFan, INPUT);
  pinMode(ButtonMotorStep, INPUT);
  pinMode(Rain, INPUT);

  pinMode(FAN, OUTPUT);
}

void loop() {
  int valueStatus = getValueStatusFromFirebase();
  if (valueStatus == false) {
    int valueFan = getValueFanFromFirebase();
    if (valueFan) {
      digitalWrite(FAN, HIGH);
    } else {
      digitalWrite(FAN, LOW);
    }

    int valueMotorStep = getValueMotorStepFromFirebase();
    static bool motorStepped = false;  // Track if motor has already been stepped
    if (valueMotorStep && !motorStepped) {
      moveStepper(-stepsPerThreeQuartersRevolution);
      motorStepped = true;  // Set flag to true after stepping
    } else if (!valueMotorStep && motorStepped) {
      moveStepper(stepsPerThreeQuartersRevolution);
      motorStepped = false;  // Set flag to false after stepping
    }
  } else {
    handleButtonFan();
    delay(100);
    handleButtonMotorStep();
    delay(100);
    handleDHT11();
    delay(100);
    handleRain();
    delay(100);
  }
  delay(100);
}

void handleButtonFan() {
  int currentButtonState = digitalRead(ButtonFan);
  // Kiểm tra nút bấm để chuyển trạng thái quạt
  if (currentButtonState == LOW && lastButtonState == LOW) {
    if (currentControl == NONE || currentControl == BUTTON_FAN) {
      if (!fanState) {
        digitalWrite(FAN, HIGH);
        Serial.println("Button Fan: +");
        Serial.println("FAN_BUTTON: ON");
        setFanToFirebase(1);
        fanState = true;
        currentControl = BUTTON_FAN;
      } else {
        digitalWrite(FAN, LOW);
        Serial.println("Button Fan: -");
        Serial.println("FAN_BUTTON: ON");
        setFanToFirebase(0);
        fanState = false;
        currentControl = NONE;
      }
    }
  }
  lastButtonState = currentButtonState;
}

void handleButtonMotorStep() {
  int currentButtonStateMotorStep = digitalRead(ButtonMotorStep);
  // Kiểm tra nút bấm để chuyển trạng thái quạt
  if (currentButtonStateMotorStep == HIGH && lastButtonStateMotorStep == LOW) {
    if (currentControlMotorStep == NONE_CONTROL || currentControlMotorStep == BUTTON_MOTOR) {
      if (!motorStepState) {
        setMotorStepToFirebase(1);
        moveStepper(-stepsPerThreeQuartersRevolution);
        Serial.println("Button Motor Step: +");
        Serial.println("MOTORSTEP_BUTTON: CLOSE");
        motorStepState = true;
        currentControlMotorStep = BUTTON_MOTOR;
      } else {
        setMotorStepToFirebase(0);
        moveStepper(stepsPerThreeQuartersRevolution);
        Serial.println("Button Motor Step: -");
        Serial.println("MOTORSTEP_BUTTON: OPEN");
        motorStepState = false;
        currentControlMotorStep = NONE_CONTROL;
      }
    }
  }
  lastButtonStateMotorStep = currentButtonStateMotorStep;
}

void handleDHT11() {
  readDHT11(temp, humi);
  setDHTToFirebase(temp, humi);
  Serial.println("Temperature: " + String(temp));
  Serial.println("Humidity: " + String(humi));
  if (currentControl == DHT || currentControl == NONE) {
    if (temp > 31) {
      Serial.println("FAN: ON");
      setFanToFirebase(1);
      digitalWrite(FAN, HIGH);
      currentControl = DHT;
    } else {
      Serial.println("FAN: OFF");
      setFanToFirebase(0);
      digitalWrite(FAN, LOW);
      currentControl = NONE;
    }
  }
}

void handleRain() {
  int currentRainState = digitalRead(Rain);
  if (currentControlMotorStep == RAIN || currentControlMotorStep == NONE_CONTROL) {
    if (currentRainState == HIGH && lastRainState == LOW) {
      Serial.println("Rain: +");
      setMotorStepToFirebase(1);

      moveStepper(-stepsPerThreeQuartersRevolution);
      currentControlMotorStep = RAIN;


    } else if (currentRainState == LOW && lastRainState == HIGH) {
      Serial.println("Rain: -");
      setMotorStepToFirebase(0);

      moveStepper(stepsPerThreeQuartersRevolution);
      currentControlMotorStep = NONE_CONTROL;
    }
    lastRainState = currentRainState;
  }
}

void readDHT11(int& temp, int& humi) {
  int temperature = dht11.readTemperature();
  delay(10);
  int humidity = dht11.readHumidity();
  if (temperature != DHT11::ERROR_CHECKSUM && temperature != DHT11::ERROR_TIMEOUT && humidity != DHT11::ERROR_CHECKSUM && humidity != DHT11::ERROR_TIMEOUT) {
    temp = temperature;
    humi = humidity;
  }
}

void moveStepper(int steps) {
  int stepsToMove = steps;
  int stepSize = 100;  // Kích thước bước nhỏ để giải phóng bộ giám sát thời gian
  while (stepsToMove != 0) {
    if (stepsToMove > stepSize) {
      myStepper.step(stepSize);
      stepsToMove -= stepSize;
    } else if (stepsToMove < -stepSize) {
      myStepper.step(-stepSize);
      stepsToMove += stepSize;
    } else {
      myStepper.step(stepsToMove);
      stepsToMove = 0;
    }
    yield();  // Giải phóng bộ giám sát thời gian
  }
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
  Serial.print("\nWiFi connected, IP address: ");
  Firebase.reconnectNetwork(true);
}

int getValueStatusFromFirebase() {
  int value = 0;
  if (Firebase.ready()) {
    if (Firebase.getInt(fbdo, "STATUSRD/auto")) value = fbdo.intData();
  }
  return value;
}

int getValueFanFromFirebase() {
  int value = 0;
  if (Firebase.ready()) {
    if (Firebase.getInt(fbdo, "FAN/value")) value = fbdo.intData();
  }
  return value;
}

int getValueMotorStepFromFirebase() {
  int value = 0;
  if (Firebase.ready()) {
    if (Firebase.getInt(fbdo, "MOTORSTEP/value")) value = fbdo.intData();
  }
  return value;
}

void setFanToFirebase(int fan) {

  if (Firebase.ready()) {
    Firebase.setInt(fbdo, "FAN/value", fan);
  }
}

void setMotorStepToFirebase(int motorStep) {

  if (Firebase.ready()) {
    Firebase.setInt(fbdo, "MOTORSTEP/value", motorStep);
  }
}

void setDHTToFirebase(int temp, int humi) {

  if (Firebase.ready()) {
    Firebase.setInt(fbdo, "DHT/temp", temp);
    Firebase.setInt(fbdo, "DHT/humi", humi);
  }
}
