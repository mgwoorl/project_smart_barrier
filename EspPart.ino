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

void handleUART() {
  while (Serial.available()) {
    char c = Serial.read();
    if (c == '\n') {
      Serial.print("Получено от Arduino: ");
      Serial.println(uartData);

      int co2Value = -1;
      int co2Status = -1;
      int coValue = -1;

      int idxCO2 = uartData.indexOf("CO2:");
      int idxCO2Status = uartData.indexOf("CO2_STATUS:");
      int idxCO = uartData.indexOf("CO:");
      int idxL1 = uartData.indexOf("L1:");
      int idxL2 = uartData.indexOf("L2:");

      if (idxCO2 != -1) {
        int endIdx = uartData.indexOf(';', idxCO2);
        if (endIdx == -1) endIdx = uartData.length();
        String co2Str = uartData.substring(idxCO2 + 4, endIdx);
        co2Value = co2Str.toInt();
        lastCo2Value = co2Value;

        // Обновляем максимальный CO2 за день
        if (co2Value > maxCO2Today) maxCO2Today = co2Value;
      }

      if (idxCO != -1) {
        int endIdx = uartData.indexOf(';', idxCO);
        if (endIdx == -1) endIdx = uartData.length();
        String coStr = uartData.substring(idxCO + 3, endIdx);
        coValue = coStr.toInt();
        lastCoValue = coValue;
      }

      if (idxCO2Status != -1) {
        int endIdx = uartData.indexOf(';', idxCO2Status);
        if (endIdx == -1) endIdx = uartData.length();
        String co2StatusStr = uartData.substring(idxCO2Status + 11, endIdx);
        co2Status = co2StatusStr.toInt();
      }

      if (idxL1 != -1) {
        int endIdx = uartData.indexOf(';', idxL1);
        if (endIdx == -1) endIdx = uartData.length();
        lightStatus1 = uartData.substring(idxL1 + 3, endIdx);
        lightStatus1.trim();
      }
      if (idxL2 != -1) {
        int endIdx = uartData.indexOf(';', idxL2);
        if (endIdx == -1) endIdx = uartData.length();
        lightStatus2 = uartData.substring(idxL2 + 3, endIdx);
        lightStatus2.trim();
      }

      // Обработка CO2_STATUS
      if (co2Status == 1) {
        co2StatusOK = true;
      } else if (co2Status == 0) {
        co2StatusOK = false;
      }

      // Отправка предупреждения в Telegram при превышении CO2 только боссу
      if (!co2StatusOK && !notified) {
        String alertMsg = "⚠️ Внимание! Уровень CO2 превышен! Текущее значение: " + String(co2Value);
        bot.sendMessage(bossChatID, alertMsg, "");
        notified = true;
      }
      if (co2StatusOK) {
        notified = false;
      }

      uartData = "";
    } else {
      uartData += c;
    }
  }
}

String getParkingStatusMessage() {
  String occupied = "";
  if (lightStatus1 == "close") {
    occupied += "1, ";
  }
  if (lightStatus2 == "close") {
    occupied += "2, ";
  }
  if (occupied.length() > 0) {
    // Убираем последнюю запятую и пробел
    occupied.remove(occupied.length() - 2);
    return "Заняты парковочные места: " + occupied;
  } else {
    return "Все парковочные места свободны.";
  }
}

void handleNewMessages(int numNewMessages) {
  for (int i = 0; i < numNewMessages; i++) {
    String chat_id = bot.messages[i].chat_id;
    String text = bot.messages[i].text;
    String from_name = bot.messages[i].from_name;

    if (!isChatAllowed(chat_id)) continue;

    if (text == "/start") {
      String commands = "/start - начать\n"
                        "/open - открыть шлагбаум\n"
                        "/status - статус парковочных мест\n"
                        "/myid - ваш chat_id";
      if (isBoss(chat_id)) {
        commands += "\n/airdata - данные по воздуху (CO2, CO)";
      }
      bot.sendMessage(chat_id, "Привет, " + from_name + "!\nДоступные команды:\n" + commands, "");
      
      // Считаем это визитом
      visitorCountToday++;
    }
    else if (text == "/open") {
      float distance = readDistanceCM();
      if (distance < DISTANCE_THRESHOLD) {
        bot.sendMessage(chat_id, "Машина обнаружена на расстоянии " + String(distance, 1) + " см. Открываю шлагбаум.", "");
        barrierServo.write(90);
        delay(5000);
        barrierServo.write(0);
      } else {
        bot.sendMessage(chat_id, "Машина не обнаружена. Расстояние: " + String(distance, 1) + " см. Подъедьте ближе.", "");
      }
      visitorCountToday++; // считаем посещение и здесь
    }
    else if (text == "/status") {
      String msg = getParkingStatusMessage();
      bot.sendMessage(chat_id, msg, "");
    }
    else if (text == "/myid") {
      bot.sendMessage(chat_id, "Ваш chat_id: " + chat_id, "");
    }
    else if (text == "/airdata" && isBoss(chat_id)) {
      // Только боссу показываем данные воздуха
      String airDataMsg = "Данные воздуха:\n";
      airDataMsg += "CO2: " + String(lastCo2Value) + "\n";
      airDataMsg += "CO: " + String(lastCoValue) + "\n";
      airDataMsg += String("Статус CO2: ") + (co2StatusOK ? "В норме" : "Превышение") + "\n";
      bot.sendMessage(chat_id, airDataMsg, "");
    }
    else {
      bot.sendMessage(chat_id, "Неизвестная команда. Напишите /start для списка команд.", "");
    }
  }
}
