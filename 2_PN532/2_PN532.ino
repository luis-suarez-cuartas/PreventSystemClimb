#include <Wire.h>
#include <Adafruit_PN532.h>
 
// Configuración de pines I2C en el ESP32
#define SDA_PIN 21
#define SCL_PIN 22
 
// Crear una instancia del PN532 usando la interfaz I2C
Adafruit_PN532 nfc(SDA_PIN, SCL_PIN);
 
void setup(void) {
  Serial.begin(115200);
  Serial.println("Iniciando el lector NFC PN532...");
 
  Wire.begin(SDA_PIN, SCL_PIN);
 
  nfc.begin();
 
  uint32_t versiondata = nfc.getFirmwareVersion();
  if (!versiondata) {
    Serial.print("No se encontró el PN53x. Revisa las conexiones.");
    while (1); // parar aquí
  }
 
  // Mostrar información del firmware
  Serial.print("Encontrado PN5");
  Serial.print((versiondata>>24) & 0xFF, HEX);
  Serial.print(" Firmware ver. ");
  Serial.print((versiondata>>16) & 0xFF, DEC);
  Serial.print('.');
  Serial.println((versiondata>>8) & 0xFF, DEC);
 
  // Configurar el módulo para operar en modo RFID con la frecuencia de 106 kbps
  nfc.SAMConfig();
 
  Serial.println("Esperando una tarjeta NFC...");
}
 
void loop(void) {
  uint8_t success;
  uint8_t uid[] = { 0, 0, 0, 0, 0, 0 }; // Buffer para almacenar el UID
  uint8_t uidLength; // Tamaño del UID
 
  // Esperar a que se detecte una tarjeta
  success = nfc.readPassiveTargetID(PN532_MIFARE_ISO14443A, uid, &uidLength);
 
  if (success) {
    // Si se ha detectado una tarjeta, mostrar el UID
    Serial.println("Tarjeta detectada!");
    Serial.print("UID Length: ");Serial.print(uidLength, DEC);Serial.println(" bytes");
    Serial.print("UID Value: ");
    for (uint8_t i=0; i < uidLength; i++) {
      Serial.print(uid[i], HEX);
    }
    Serial.println("");
 
    // Esperar un momento antes de intentar leer otra vez
    delay(1000);
  }
}