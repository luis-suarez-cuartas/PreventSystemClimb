#include <WiFi.h>
#include <HTTPClient.h>

// Configuración de red
const char* ssid = "TP-Link_82BA"; // Nombre de tu red WiFi
const char* password = "61540239"; // Contraseña de tu red WiFi

// Configuración de ThingSpeak
const char* server = "http://api.thingspeak.com";
String apiKey = "JMP0YVYBESO5HRBD"; // Reemplaza con tu clave de API
//2644979
// Variables para control de la temperatura
float temperaturaAnterior = 25.0; // Valor inicial de temperatura

// Temporizador basado en millis
unsigned long intervaloEnvio = 30000; // 30 segundos
unsigned long tiempoAnterior = 0;

void setup() {
  Serial.begin(115200);
  
  // Conectarse a la red WiFi
  conectarWiFi();

  // Asegurarse de que se haya realizado la conexión antes de proceder
  if(WiFi.status() == WL_CONNECTED) {
    Serial.println("Conexión establecida, comenzando a enviar datos...");
  }
}

void loop() {
  unsigned long tiempoActual = millis();

  // Verificar si ha pasado el intervalo de tiempo
  if (tiempoActual - tiempoAnterior >= intervaloEnvio) {
    enviarDatos(); // Enviar los datos a ThingSpeak
    tiempoAnterior = tiempoActual; // Actualizar el tiempo anterior
  }
}

// Función para conectarse a la red WiFi
void conectarWiFi() {
  Serial.print("Conectando a ");
  Serial.println(ssid);
  
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi conectado");
  Serial.print("IP asignada: ");
  Serial.println(WiFi.localIP());
}

// Función para leer la temperatura (emulada)
float leerTemperatura() {
  // Generamos una variación aleatoria pequeña
  float variacion = (random(-20, 21) / 10.0); // Variación de -2.0 a 2.0 grados

  // Calculamos la nueva temperatura basándonos en la anterior
  float nuevaTemperatura = temperaturaAnterior + variacion;

  // Limitar la temperatura a un rango creíble
  if (nuevaTemperatura < 15.0) nuevaTemperatura = 15.0;
  if (nuevaTemperatura > 35.0) nuevaTemperatura = 35.0;

  // Actualizamos la temperatura anterior
  temperaturaAnterior = nuevaTemperatura;

  Serial.print("Temperatura leída: ");
  Serial.println(nuevaTemperatura);
  
  return nuevaTemperatura;
}

// Función para enviar datos a ThingSpeak
void enviarDatos() {
  if(WiFi.status() == WL_CONNECTED) { // Asegurarse de que estamos conectados a WiFi
    HTTPClient http;
    float temperatura = leerTemperatura();
    String url = String(server) + "/update?api_key=" + apiKey + "&field1=" + String(temperatura);
    
    Serial.print("Enviando datos a ThingSpeak: ");
    Serial.println(url);
    
    http.begin(url); // Iniciar conexión HTTP
    int httpCode = http.GET(); // Realizar solicitud GET

    if(httpCode > 0) {
      String payload = http.getString();
      Serial.print("Respuesta del servidor: ");
      Serial.println(payload); // Esto es el número total de entradas en ThingSpeak
    } else {
      Serial.print("Error en la solicitud HTTP: ");
      Serial.println(http.errorToString(httpCode).c_str());
      reintentarConexion(); // Reintentar si la conexión es rechazada
    }
    
    http.end(); // Finalizar conexión HTTP
  } else {
    Serial.println("Error: No conectado a WiFi");
    conectarWiFi(); // Intentar reconectar si la conexión WiFi se ha perdido
  }
}

// Función para reintentar conexión HTTP
void reintentarConexion() {
  Serial.println("Reintentando conexión a ThingSpeak...");
  enviarDatos();
}
