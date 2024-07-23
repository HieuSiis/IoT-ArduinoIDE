#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>

#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
#include <WiFiClient.h>


#include <Arduino.h>
#include <ESP8266mDNS.h>

#ifndef APSSID
#define APSSID "HSU_Students"
#define APPSK "dhhs12cnvch"
#endif

const char* ssid = APSSID;
const char* password = APPSK;
// Thông tin về MQTT Broker
#define mqtt_server "broker.emqx.io"
const uint16_t mqtt_port = 1883;  //Port của MQTT broker
#define mqtt_topic_pub_waterpump "hieu/smarthome/WaterPump"
#define mqtt_topic_sub_waterpump "hieu/smarthome/WaterPump"

#define mqtt_topic_pub_light "hieu/smarthome/Light"
#define mqtt_topic_sub_light "hieu/smarthome/Light"

#define BUTTON_PIN D6
#define BUTTON_LIGHT D1
#define WATER_PUMP D5
#define Flame D7
#define Buzzer D4
#define MIC_PIN A0
#define LIGHT D2

int lastButtonState = HIGH;
int buttonState;
int numberOfPresses = 1;

int lastButtonLightState = HIGH;

char lightState[4];


int sensorValue = 0;
int clapSensorValue = 0;
int clapCount = 0;
unsigned long startTime = 0;
char FlameState[100];
bool isPumpOn = false;

const char* URL = "http://10.106.21.209:1909/add";
HTTPClient http;
ESP8266WebServer server(80);

WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "2.asia.pool.ntp.org", 7 * 3600);


WiFiClient espClient;
PubSubClient client(espClient);

WiFiClient httpClient;

StaticJsonDocument<256> doc;
char waterPump[4];

void setup() {
  pinMode(WATER_PUMP, OUTPUT);
  pinMode(BUTTON_PIN, INPUT);
  pinMode(BUTTON_LIGHT, INPUT);

  pinMode(MIC_PIN, INPUT);
  pinMode(LIGHT, OUTPUT);

  pinMode(Flame, INPUT);
  pinMode(Buzzer, OUTPUT);
  digitalWrite(Buzzer, LOW);
  server.on("/", handleOnConnect);
  server.begin();
  timeClient.begin();

  Serial.begin(115200);
  setup_wifi();
  client.setServer(mqtt_server, mqtt_port);
  client.setCallback(callback);
  reconnect();
}
void setup_wifi() {
  delay(10);
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}
void callback(char* topic, byte* payload, unsigned int length) {
  // Chuyển đổi byte thành chuỗi JSON
  deserializeJson(doc, payload, length);

  // Xác định dữ liệu từ các topic khác nhau
  if (strcmp(topic, mqtt_topic_sub_waterpump) == 0) {
    // Xử lý dữ liệu cho water pump
    strlcpy(waterPump, doc["status"] | "off", sizeof(waterPump));
    Serial.print("Water Pump Status: ");
    Serial.println(waterPump);
    if (strcmp(waterPump, "on") == 0) {
      digitalWrite(WATER_PUMP, HIGH);
      isPumpOn = true;
    } else {
      digitalWrite(WATER_PUMP, LOW);
      isPumpOn = false;
    }
  } else if (strcmp(topic, mqtt_topic_sub_light) == 0) {
    // Xử lý dữ liệu cho đèn
    strlcpy(lightState, doc["status"] | "off", sizeof(lightState));
    Serial.print("Light State: ");
    Serial.println(lightState);
    if (strcmp(lightState, "on") == 0) {
      digitalWrite(LIGHT, HIGH);
    } else {
      digitalWrite(LIGHT, LOW);
    }
  }
}

void reconnect() {
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Hàm connect với các tham số là id của MQTT client, username và password
    if (client.connect("Hieu")) {
      Serial.println("connected");
      // Publish thông điệp khi kết nối lại thành công
      char buffer[256];
      doc["message"] = "Hello esp8266!";
      size_t n = serializeJson(doc, buffer);
      client.publish(mqtt_topic_pub_waterpump, buffer, n);
      client.publish(mqtt_topic_pub_light, buffer, n);
      // Đăng ký nhận thông điệp từ các topic tương ứng
      client.subscribe(mqtt_topic_sub_waterpump);
      client.subscribe(mqtt_topic_sub_light);
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      delay(3000);
    }
  }
}

void loop() {

  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  int currentButtonLightState = digitalRead(BUTTON_LIGHT);

  if (currentButtonLightState == HIGH && lastButtonLightState == LOW) {
    // Đảo trạng thái của đèn
    if (strcmp(lightState, "on") == 0) {
      strcpy(lightState, "off");
      Serial.println("Turn OFF LIGHT");
      digitalWrite(LIGHT, LOW);
    } else {
      strcpy(lightState, "on");
      Serial.println("Turn ON LIGHT");
      digitalWrite(LIGHT, HIGH);
    }
    // Tạo một buffer để lưu trữ dữ liệu JSON
    char buffer[256];
    doc["name"] = "Light";
    doc["status"] = lightState;
    doc["message"] = "Button";
    size_t n = serializeJson(doc, buffer);
    // Gửi dữ liệu JSON tới MQTT broker
    client.publish(mqtt_topic_pub_light, buffer, n);
  }
  // Lưu trạng thái hiện tại của nút nhấn đèn để so sánh ở lần lặp tiếp theo
  lastButtonLightState = currentButtonLightState;

  sensorValue = analogRead(MIC_PIN);
  Serial.print("Received Sensor Value: ");
  Serial.println(sensorValue);

  if (sensorValue > 800) {
    Serial.print("Clap Sensor Value: ");
    clapSensorValue = sensorValue;
    Serial.println(sensorValue);
    startTime = millis();
    clapCount++;
    delay(1000);

    while (millis() - startTime < 2000) {
      sensorValue = analogRead(MIC_PIN);
      if (sensorValue > 800) {
        clapCount++;
        delay(1000);
      }
    }
  }

  Serial.print("Clap Count: ");
  Serial.println(clapCount);

  if (clapCount == 1) {
    digitalWrite(LIGHT, HIGH);
    Serial.println("LIGHT: ON");
    strcpy(lightState, "on");
    char buffer[256];
    doc["name"] = "Light";
    doc["status"] = lightState;
    doc["message"] = "Clap 1";
    size_t n = serializeJson(doc, buffer);
    client.publish(mqtt_topic_pub_light, buffer, n);
    timeClient.update();
    postJsonData(clapSensorValue, clapCount, lightState);
  } else if (clapCount == 2) {
    digitalWrite(LIGHT, LOW);
    Serial.println("LIGHT: OFF");
    strcpy(lightState, "off");
    timeClient.update();
    postJsonData(clapSensorValue, clapCount, lightState);
    char buffer[256];
    doc["name"] = "Light";
    doc["status"] = lightState;
    doc["message"] = "Clap 2";
    size_t n = serializeJson(doc, buffer);
    client.publish(mqtt_topic_pub_light, buffer, n);
  }
  clapCount = 0;

  // Xử lý nhấn nút nhấn WATER_PUMP
  int currentButtonState = digitalRead(BUTTON_PIN);

  // Kiểm tra xem nút nhấn có được nhấn từ trạng thái không nhấn sang nhấn không
  if (currentButtonState == HIGH && lastButtonState == LOW) {
    // Cập nhật trạng thái của pump
    if (strcmp(waterPump, "on") == 0) {
      strlcpy(waterPump, "off", sizeof(waterPump));
    } else {
      strlcpy(waterPump, "on", sizeof(waterPump));
    }
    // Tạo một buffer để lưu trữ dữ liệu JSON
    char buffer[256];
    // Serialize dữ liệu JSON
    doc["name"] = "WaterPump";
    doc["status"] = waterPump;
    doc["message"] = "Button";
    size_t n = serializeJson(doc, buffer);
    // Gửi dữ liệu JSON tới MQTT broker
    client.publish(mqtt_topic_pub_waterpump, buffer, n);
  }
  // Lưu trạng thái hiện tại của nút nhấn để so sánh ở lần lặp tiếp theo
  lastButtonState = currentButtonState;

  // Kiểm tra trạng thái của pump và cảm biến lửa khi nút không được nhấn
  if (!isPumpOn) {
    int fire = digitalRead(Flame);
    Serial.print("Value Flame: ");
    Serial.println(fire);

    if (fire == 1) {
      strcpy(FlameState, "NO FIRE");
      Serial.println("NO FIRE!!!");
      digitalWrite(Buzzer, LOW);
      digitalWrite(WATER_PUMP, LOW);
    } else {
      strcpy(FlameState, "FIRE");
      Serial.println("FIRE!!!");
      digitalWrite(Buzzer, HIGH);
      digitalWrite(WATER_PUMP, HIGH);
    }
  }
  delay(1000);
}

void handleOnConnect() {
  server.send(200, "text/html", "ok");
}

void postJsonData(int clapSensorValue, int clapCount, const char* lightState) {
  Serial.print("connecting to ");
  if (WiFi.status() == WL_CONNECTED) {
    Serial.print("[HTTP] begin...\n");
    if (http.begin(httpClient, URL)) {
      Serial.print("[HTTP] POST...\n");
      // doc["id"] = "1";
      doc["clapSensorValue"] = clapSensorValue;
      doc["clapCount"] = clapCount;
      doc["Light"] = lightState;
      doc["createdAt"] = timeClient.getFormattedTime();
      char output[2048];
      serializeJson(doc, Serial);
      serializeJson(doc, output);
      http.addHeader("Content-Type", "application/json");
      int httpCode = http.POST(output);
      Serial.println(httpCode);
      if (httpCode == 200) {
        Serial.print("Status: ");
        Serial.print(httpCode);
        Serial.println(" OK. Message: Successful POST!!! ");
      } else {
        Serial.print("HTTP Error: ");
        Serial.println(http.errorToString(httpCode).c_str());
      }
      // delay(3000);
      http.end();
      Serial.println("closing connection");
      doc.clear();
    }
  }
}
