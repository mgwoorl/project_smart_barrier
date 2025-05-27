#include <ESP8266WiFi.h> 
#include <WiFiClientSecure.h>
#include <UniversalTelegramBot.h>
#include <ArduinoJson.h>
#include <Servo.h>

const char* ssid = "Estriper";
const char* password = "lalala3031";
#define BOTtoken "8148150074:AAEtVWxB62mpDiIRsrUQJCHgci61_igmqRo"

WiFiClientSecure client;
UniversalTelegramBot bot(BOTtoken, client);

Servo barrierServo;
const int servoPin = D4;

const int trigPin = D1;
const int echoPin = D2;
const float DISTANCE_THRESHOLD = 30.0;

bool protection = true;
int chatID_access[] = {
  123456789,
  987654321,
  1342960019
};
const String bossChatID = "1342960019";

int maxCO2Today = 0;
int visitorCountToday = 0;

String uartData = "";
bool co2StatusOK = true; // true — CO2 в норме, false — превышение
bool notified = false;   // чтобы не спамить уведомления

String lightStatus1 = "open"; // по умолчанию свободно
String lightStatus2 = "open";

int lastCo2Value = -1;
int lastCoValue = -1;

bool isChatAllowed(String chat_id) {
  if (!protection) return true;
  for (int i = 0; i < sizeof(chatID_access) / sizeof(chatID_access[0]); i++) {
    if (String(chatID_access[i]) == chat_id) return true;
  }
  bot.sendMessage(chat_id, "У вас нет доступа. Ваш chat_id: " + chat_id, "");
  return false;
}

bool isBoss(String chat_id) {
  return chat_id == bossChatID;
}

float readDistanceCM() {
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);
  long duration = pulseIn(echoPin, HIGH, 30000);
  float distance = duration * 0.0343 / 2;
  if (duration == 0) return 999;
  return distance;
}
