#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <Servo.h>

const char* ssid = "Estriper";
const char* password = "lalala3031";

const char* baseUrl = "http://192.168.109.122:5050";
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
  Serial.begin(9600);  // UART от Arduino
  barrierServo.attach(servoPin);
  barrierServo.write(0);  // закрыто

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

  // Получение данных с Arduino
  handleSerialInput();

  // Пока шлагбаум открыт, ничего не делаем
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

  String payload = "{\"id\":" + String(espId) +
                   ",\"distance_entrance\":" + String(distanceEntrance) +
                   ",\"distance_exit\":" + String(distanceExit) +
                   ",\"free_places\":" + String(freePlaces) +
                   ",\"co2\":" + String(co2Value) + "}";

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
    bool wannaEntranceOpen = response.indexOf("\"isWannaEntranceOpen\":true") >= 0;
    bool wannaExitOpen = response.indexOf("\"isWannaExitOpen\":true") >= 0;

    if (wannaEntranceOpen) {
      openBarrier(ENTRANCE);
    } else if (wannaExitOpen) {
      openBarrier(EXIT);
    }
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

  HTTPClient http;
  http.begin(client, String(baseUrl) + resetEndpoint);
  http.addHeader("Content-Type", "application/json");

  String payload = "{\"reset_entrance\":" + String(lastGateAction == ENTRANCE ? "true" : "false") +
                   ",\"reset_exit\":" + String(lastGateAction == EXIT ? "true" : "false") + "}";

  int httpCode = http.POST(payload);
  Serial.print("Сброс флагов: ");
  Serial.println(httpCode);
  http.end();

  lastGateAction = NONE;
}
