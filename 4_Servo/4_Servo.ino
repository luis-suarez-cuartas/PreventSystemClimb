

#include <ESP32Servo.h>

// Definir el pin al que está conectado el servo
const int servoPin = 2;

// Crear un objeto Servo
Servo myservo;

void setup() {
  // Iniciar comunicación serial
  Serial.begin(115200);

  // Adjuntar el servo al pin definido
  myservo.attach(servoPin);

  // Mover el servo a 0 grados
  myservo.write(0);
  Serial.println("Servo en 0 grados");
  delay(2000); // Esperar 2 segundos

  // Mover el servo a 90 grados
  myservo.write(90);
  Serial.println("Servo en 90 grados");
  delay(2000); // Esperar 2 segundos

  // Volver a mover el servo a 0 grados
  myservo.write(0);
  Serial.println("Servo en 0 grados");
}

void loop() {
  // No se necesita código en el loop para este ejemplo
}

