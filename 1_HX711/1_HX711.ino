
#include "Adafruit_HX711.h"
// El siguiente código en Arduino IDE configura el ESP32 para comunicarse con el HX711 y realizar lecturas de peso utilizando la librería Adafruit_HX711.
// Definir los pines para la comunicación con el HX711
const uint8_t DATA_PIN = 26;  // Puede usar cualquier pin
const uint8_t CLOCK_PIN = 25; // Puede usar cualquier pin
 
Adafruit_HX711 hx711(DATA_PIN, CLOCK_PIN);
 
void setup() {
  Serial.begin(115200);
 
  // Esperar a que el puerto serial se conecte. Necesario solo para puertos USB nativos
  while (!Serial) {
    delay(10);
  }
 
  Serial.println("Prueba de Adafruit HX711!");
 
  // Inicializar el HX711
  hx711.begin();
 
  // Leer y descartar 3 valores para calibración
  Serial.println("Calibrando...");
  for (uint8_t t = 0; t < 3; t++) {
    hx711.tareA(hx711.readChannelRaw(CHAN_A_GAIN_128));
    hx711.tareA(hx711.readChannelRaw(CHAN_A_GAIN_128));
    hx711.tareB(hx711.readChannelRaw(CHAN_B_GAIN_32));
    hx711.tareB(hx711.readChannelRaw(CHAN_B_GAIN_32));
  }
} 
 
void loop() {
  // Leer desde el canal A con ganancia 128
  int32_t pesoA128 = hx711.readChannelBlocking(CHAN_A_GAIN_128);
  Serial.print("Canal A (Ganancia 128):----------- ");
  Serial.println(pesoA128);
 
  // Leer desde el canal B con ganancia 32
  int32_t pesoB32 = hx711.readChannelBlocking(CHAN_B_GAIN_32);
  Serial.print("Canal B (Ganancia 32): ");
  Serial.println(pesoB32);
   delay(1000); 
}