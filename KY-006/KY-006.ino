const int pinZumbador = 5;  // Pin donde está conectado el zumbador

void setup() {
  pinMode(pinZumbador, OUTPUT);  // Configura el pin como salida
}

void loop() {
  tone(pinZumbador, 1000, 1000);  // Genera un tono de 1000 Hz durante 1 segundo
  delay(1000);  // Espera 1 segundo

  tone(pinZumbador, 1500, 1000);  // Cambia a 1500 Hz durante 1 segundo
  delay(1000);  // Espera 1 segundo

  tone(pinZumbador, 2000, 1000);  // Cambia a 2000 Hz durante 1 segundo
  delay(1000);  // Espera 1 segundo
}

void tone(int pin, int frequency, int duration) {
  int period = 1000000 / frequency;  // Calcula el período en microsegundos
  int halfPeriod = period / 2;  // Calcula la mitad del período

  for (long i = 0; i < duration * 1000L; i += period) {
    digitalWrite(pin, HIGH);  // Establece el pin en alto
    delayMicroseconds(halfPeriod);  // Espera la mitad del período
    digitalWrite(pin, LOW);  // Establece el pin en bajo
    delayMicroseconds(halfPeriod);  // Espera la otra mitad del período
  }
}

