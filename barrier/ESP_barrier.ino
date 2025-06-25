#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <Servo.h>
#include <ArduinoJson.h>

const char* ssid = "Tenda_653568";
const char* password = "381117667";

const char* baseUrl = "http://192.168.0.101:5050";
const char* dataEndpoint = "/sensors/data";
const char* resetEndpoint = "/sensors/reset-gate-status";
const int espId = 1;

const int servoPin = D4;
Servo barrierServo;
WiFiClient client;

const int trigPin_enter = D1;
const int echoPin_enter = D2;

const int trigPin_excit = D8;
const int echoPin_excit = D5;

bool isGateOpened = false;
unsigned long gateOpenedAt = 0;
unsigned long lastStatusCheck = 0;
unsigned long lastDataSend = 0;

const int TOTAL_SPOTS = 2;
int freePlaces = -1;
int co2Value = -1;
int distanceEntrance = 25;
int distanceExit = 25;

enum GateAction { NONE, ENTRANCE, EXIT };
GateAction lastGateAction = NONE;

void setup() {
  Serial.begin(9600);
  barrierServo.attach(servoPin);
  barrierServo.write(0);

  pinMode(trigPin_enter, OUTPUT);
  pinMode(echoPin_enter, INPUT);
  pinMode(trigPin_excit, OUTPUT);
  pinMode(echoPin_excit, INPUT);

  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWi-Fi подключен");
  Serial.println("Ожидание данных от Arduino...");
}

void loop() {
  unsigned long now = millis();

  handleSerialInput();

  if (isGateOpened) {
    if (now - gateOpenedAt >= 15000) {
      resetGateStatus();
      isGateOpened = false;
    }
    return;
  }

  if (now - lastStatusCheck > 3000) {
    checkOpenRequest();
    lastStatusCheck = now;
  }

  if (now - lastDataSend > 5000) {
    sendSensorData();
    lastDataSend = now;
  }
}

float readDistanceCM(int trigPin, int echoPin) {
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);

  long duration = pulseIn(echoPin, HIGH, 30000);
  float distance = duration * 0.0343 / 2;

  if (duration == 0) {
    return 999.0;
  }

  return distance;
}

void handleSerialInput() {
  if (Serial.available()) {
    String input = Serial.readStringUntil('\n');
    input.trim();

    if (input.startsWith("CO:")) {
      int coStart = input.indexOf("CO:") + 3;
      int coEnd = input.indexOf(";", coStart);
      String coStr = input.substring(coStart, coEnd);
      co2Value = coStr.toInt();

      int freeCount = 0;
      if (input.indexOf("P1:free") != -1) freeCount++;
      if (input.indexOf("P2:free") != -1) freeCount++;
      freePlaces = freeCount;

      Serial.print("Получены данные: CO2 = ");
      Serial.print(co2Value);
      Serial.print(" ppm, Свободно мест: ");
      Serial.println(freePlaces);
    }
  }
}

void sendSensorData() {
  if (co2Value < 0 || freePlaces < 0) {
    Serial.println("Ожидаю корректные данные от Arduino...");
    return;
  }

  StaticJsonDocument<256> doc;
  doc["id"] = espId;
  doc["distance_entrance"] = distanceEntrance;
  doc["distance_exit"] = distanceExit;
  doc["free_places"] = freePlaces;
  doc["co2"] = co2Value;

  String payload;
  serializeJson(doc, payload);

  HTTPClient http;
  http.begin(client, String(baseUrl) + dataEndpoint);
  http.addHeader("Content-Type", "application/json");
  int httpCode = http.POST(payload);

  Serial.print("Отправка данных: ");
  Serial.println(httpCode);
  http.end();
}

void checkOpenRequest() {
  HTTPClient http;
  http.begin(client, String(baseUrl) + dataEndpoint);
  int httpCode = http.GET();

  if (httpCode == 200) {
    String response = http.getString();

    StaticJsonDocument<256> doc;
    DeserializationError error = deserializeJson(doc, response);
    if (!error) {
      bool wannaEntranceOpen = doc["isWannaEntranceOpen"] | false;
      bool wannaExitOpen = doc["isWannaExitOpen"] | false;

      if (wannaEntranceOpen) {
        openBarrier(ENTRANCE);
      } else if (wannaExitOpen) {
        openBarrier(EXIT);
      }
    } else {
      Serial.println("Ошибка парсинга JSON с сервера");
    }
  } else {
    Serial.print("Ошибка HTTP GET: ");
    Serial.println(httpCode);
  }

  http.end();
}

void openBarrier(GateAction action) {
  barrierServo.write(90);
  isGateOpened = true;
  gateOpenedAt = millis();
  lastGateAction = action;

  if (action == ENTRANCE) {
    Serial.println("🟢 Шлагбаум на ВЪЕЗД ОТКРЫТ");
  } else if (action == EXIT) {
    Serial.println("🟢 Шлагбаум на ВЫЕЗД ОТКРЫТ");
  }
}

void resetGateStatus() {
  barrierServo.write(0);
  Serial.println("🔴 Шлагбаум ЗАКРЫТ");

  StaticJsonDocument<128> doc;
  doc["reset_entrance"] = (lastGateAction == ENTRANCE);
  doc["reset_exit"] = (lastGateAction == EXIT);

  String payload;
  serializeJson(doc, payload);

  HTTPClient http;
  http.begin(client, String(baseUrl) + resetEndpoint);
  http.addHeader("Content-Type", "application/json");

  int httpCode = http.POST(payload);
  Serial.print("Сброс флагов: ");
  Serial.println(httpCode);
  http.end();

  lastGateAction = NONE;
}
