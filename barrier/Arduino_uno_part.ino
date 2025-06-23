const int photoPin1 = A0;       
const int photoPin2 = A1;  
const int coPin = A4;          

const int lightThreshold = 500; 

void setup() {
  Serial.begin(9600);          
}

void loop() {
  int light1 = analogRead(photoPin1);
  int light2 = analogRead(photoPin2);
  int coValue = analogRead(coPin);

  String parking1 = (light1 < lightThreshold) ? "free" : "occupied";
  String parking2 = (light2 < lightThreshold) ? "free" : "occupied";

  String data = "CO:" + String(coValue);
  data += ";P1:" + parking1;
  data += ";P2:" + parking2;

  Serial.println(data);
  delay(1000);
}
