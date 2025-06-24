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

bool isGateOpened = false;
unsigned long gateOpenedAt = 0;
unsigned long lastStatusCheck = 0;
unsigned long lastDataSend = 0;

int freePlaces = -1;
int co2Value = -1;
int distanceEntrance = 25;
int distanceExit = 25;

enum GateAction { NONE, ENTRANCE, EXIT };
GateAction lastGateAction = NONE;

void setup() {
  Serial.begin(9600);
  barrierServo.attach(servoPin);
  barrierServo.write(0);  // закрыто

  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWi-Fi подключен");
  Serial.println("Введите команду 'c <вход> <выход>' для установки расстояний.");
}

void loop() {
  unsigned long now = millis();

  if (isGateOpened) {
    if (now - gateOpenedAt >= 15000) {
      resetGateStatus();  // Закрытие и сброс флагов
      isGateOpened = false;
    }
    return;
  }

  handleSerialInput();
  generateFakeSensorData();

  if (now - lastStatusCheck > 3000) {
    checkOpenRequest();
    lastStatusCheck = now;
  }

  if (now - lastDataSend > 5000) {
    sendSensorData();
    lastDataSend = now;
  }
}

void generateFakeSensorData() {
  co2Value = random(300, 1200);
  freePlaces = random(0, 10);
}

void sendSensorData() {
  if (co2Value < 0 || freePlaces < 0) {
    Serial.println("Ожидаю данные от Arduino...");
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

  Serial.print("CO2: ");
  Serial.print(co2Value);
  Serial.print(" ppm, Свободно: ");
  Serial.print(freePlaces);
  Serial.print(", Вход: ");
  Serial.print(distanceEntrance);
  Serial.print(" см, Выход: ");
  Serial.println(distanceExit);
}

void checkOpenRequest() {
  HTTPClient http;
  http.begin(client, String(baseUrl) + dataEndpoint);
  int httpCode = http.GET();

  if (httpCode == 200) {
    String response = http.getString();
    Serial.println("Ответ от /sensors/data:");
    Serial.println(response);

    bool wannaEntranceOpen = response.indexOf("\"isWannaEntranceOpen\":true") >= 0;
    bool wannaExitOpen = response.indexOf("\"isWannaExitOpen\":true") >= 0;

    if (wannaEntranceOpen) {
      Serial.println("Запрос на открытие въезда обнаружен.");
      openBarrier(ENTRANCE);
    } else if (wannaExitOpen) {
      Serial.println("Запрос на открытие выезда обнаружен.");
      openBarrier(EXIT);
    }
  } else {
    Serial.print("Ошибка запроса к /sensors/data: ");
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
