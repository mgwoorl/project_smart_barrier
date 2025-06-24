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

