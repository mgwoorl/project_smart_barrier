#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <Servo.h>
#include <ArduinoJson.h>

const char* ssid = "Estriper";
const char* password = "lalala3031";
const char* serverURL = "http://0.0.0.0:8000";

WiFiClient client;
Servo barrierServo;

const String esp_id = "123"; // Уникальный ID ESP
const float DISTANCE_THRESHOLD = 30.0;

const int trigPin = D1;
const int echoPin = D2;
const int servoPin = D4;

String uartBuffer = "";
String co2 = "";
String light = "";
String temp = "";

bool isGateOpened = false;
unsigned long gateOpenedAt = 0;
unsigned long lastStatusCheck = 0;
unsigned long lastDataSend = 0;

void setup() {
  Serial.begin(9600);
  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);

  barrierServo.attach(servoPin);
  barrierServo.write(0); // Закрыто

  WiFi.begin(ssid, password);
  Serial.print("Connecting");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500); Serial.print(".");
  }
  Serial.println("\nConnected to WiFi!");
}

float readDistanceCM() {
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);
  long duration = pulseIn(echoPin, HIGH, 30000); // max 30ms
  if (duration == 0) return 999;
  return duration * 0.0343 / 2;
}

void parseUARTData() {
  while (Serial.available()) {
    char c = Serial.read();
    if (c == '\n') {
      if (uartBuffer.length() > 0) {
        // Пример строки: CO2=400;LIGHT=350;TEMP=23.5
        int c1 = uartBuffer.indexOf("CO2=");
        int c2 = uartBuffer.indexOf("LIGHT=");
        int c3 = uartBuffer.indexOf("TEMP=");

        if (c1 != -1) co2 = uartBuffer.substring(c1 + 4, uartBuffer.indexOf(";", c1));
        if (c2 != -1) light = uartBuffer.substring(c2 + 6, uartBuffer.indexOf(";", c2));
        if (c3 != -1) temp = uartBuffer.substring(c3 + 5);

        Serial.println("Parsed UART: CO2=" + co2 + ", LIGHT=" + light + ", TEMP=" + temp);
      }
      uartBuffer = "";
    } else {
      uartBuffer += c;
    }
  }
}

void sendSensorData() {
  if (WiFi.status() != WL_CONNECTED) return;

  HTTPClient http;
  http.begin(client, String(serverURL) + "/esp/update-data");
  http.addHeader("Content-Type", "application/json");

  StaticJsonDocument<256> doc;
  doc["esp_id"] = esp_id;
  doc["co2"] = co2;
  doc["light"] = light;
  doc["temp"] = temp;
  doc["distance"] = readDistanceCM();

  String json;
  serializeJson(doc, json);
  int httpCode = http.POST(json);
  Serial.println("Data POST: " + String(httpCode));
  http.end();
}

void checkStatusFromServer() {
  if (WiFi.status() != WL_CONNECTED || isGateOpened) return;

  HTTPClient http;
  String url = String(serverURL) + "/esp/status-check?esp_id=" + esp_id;
  http.begin(client, url);
  int httpCode = http.GET();

  if (httpCode == HTTP_CODE_OK) {
    String payload = http.getString();
    StaticJsonDocument<128> doc;
    DeserializationError error = deserializeJson(doc, payload);
    if (!error) {
      bool isWannaOpen = doc["isWannaOpen"];
      Serial.println("Status: isWannaOpen = " + String(isWannaOpen));

      if (isWannaOpen && readDistanceCM() < DISTANCE_THRESHOLD) {
        barrierServo.write(90);
        isGateOpened = true;
        gateOpenedAt = millis();
        Serial.println("Gate opened");
      }
    }
  } else {
    Serial.println("Status check failed");
  }
  http.end();
}

void sendCloseSignal() {
  if (WiFi.status() != WL_CONNECTED) return;

  HTTPClient http;
  http.begin(client, String(serverURL) + "/esp/close-request");
  http.addHeader("Content-Type", "application/json");

  StaticJsonDocument<128> doc;
  doc["esp_id"] = esp_id;
  doc["isWannaOpen"] = false;

  String json;
  serializeJson(doc, json);
  int httpCode = http.POST(json);
  Serial.println("Close POST: " + String(httpCode));
  http.end();
}

void handleGateTimer() {
  if (isGateOpened && millis() - gateOpenedAt >= 10000) {
    barrierServo.write(0);
    isGateOpened = false;
    sendCloseSignal();
    Serial.println("Gate closed after 10 sec");
  }
}

void loop() {
  parseUARTData();
  handleGateTimer();

  unsigned long now = millis();
  if (!isGateOpened && now - lastStatusCheck >= 5000) {
    lastStatusCheck = now;
    checkStatusFromServer();
  }

  if (now - lastDataSend >= 5000) {
    lastDataSend = now;
    sendSensorData();
  }
}
