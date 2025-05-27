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
