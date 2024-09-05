const int trigPins[4] = {25, 12, 13, 5};
const int echoPins[4] = {26, 33, 32, 18};
const int pinZumbador = 4;
const int canal = 0, frec = 2000, resolucion = 8;
 
void setup() {
  Serial.begin(115200);
  for (int i = 0; i < 4; i++) {
    pinMode(trigPins[i], OUTPUT);
    pinMode(echoPins[i], INPUT);
  }
    // Configuración del canal PWM
  ledcSetup(canal, frec, resolucion);
  // Asignación del pin al canal PWM
  ledcAttachPin(pinZumbador, canal);
}
 
void loop() {
  for (int i = 0; i < 4; i++) {
    long duration;
    float distance;
 
    digitalWrite(trigPins[i], LOW);
    delayMicroseconds(2);
    digitalWrite(trigPins[i], HIGH);
    delayMicroseconds(10);
    digitalWrite(trigPins[i], LOW);
 
    duration = pulseIn(echoPins[i], HIGH);
    distance = duration * 0.034 / 2;
 
    Serial.print("S");
    Serial.print(i + 1);
    Serial.print(": ");
    Serial.print(distance);
    Serial.print("cm | ");
  }
      Serial.println("");

  delay(1000);
}
   
   