const int photoPin1 = A0;
const int photoPin2 = A1;
const int lightThreshold = 500;

const int co2Pin = A3;
int co2Value = 0;
const int CO2_LIMIT = 600;  // допустимый уровень CO2

#define ANALOG_PIN A4
#define DIGITAL_PIN 8
int coAnalogValue = 0;
int coLimitStatus = 0;

void setup() {
  Serial.begin(9600);
  pinMode(DIGITAL_PIN, INPUT);
}

void loop() {
  int light1 = analogRead(photoPin1);
  int light2 = analogRead(photoPin2);
  String lightStatus1 = (light1 > lightThreshold) ? "close" : "open";
  String lightStatus2 = (light2 > lightThreshold) ? "close" : "open";

  co2Value = analogRead(co2Pin);
  coLimitStatus = (co2Value <= CO2_LIMIT) ? 1 : 0;
  
  const int co2StatusPin = 7;
  pinMode(co2StatusPin, OUTPUT);
  digitalWrite(co2StatusPin, coLimitStatus);
  
  coAnalogValue = analogRead(ANALOG_PIN);
  coLimitStatus = digitalRead(DIGITAL_PIN);
  
  String data = "CO2:" + String(co2Value);
  data += ";CO2_STATUS:" + String(coLimitStatus);
  data += ";L1:" + lightStatus1;
  data += ";L2:" + lightStatus2;
  
  Serial.println(data);
  
  delay(1000);

}



