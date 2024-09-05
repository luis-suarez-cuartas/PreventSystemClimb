//ESP8266 ||
#if !(ESP32)
#error This code is intended to run on the ESP8266/ESP32 platform! Please check your Tools->Board setting
#endif

#include <ESP32Servo.h>  // 3.0.5 KEVIN HARRINGTON / John K. Bennett
#include <Arduino.h>
#include "HX711.h"  // aclarar cual ! Rob Tilaart 0.5.0
#include <MySQL_Generic.h>
#include "Credentials.h"
#include <HTTPClient.h>

const int pinZumbador = 19;

const int trigPins[4] = { 25, 12, 13, 5 };
const int echoPins[4] = { 26, 33, 32, 18 };
const int servoPin = 2;
const int alturas[4] = { 1, 2, 3, 4 };

// HX711 circuit wiring
const int LOADCELL_DOUT_PIN = 16;
const int LOADCELL_SCK_PIN = 4;

int usuarioId = -1;
int trainingId = -1;
int rutaActivaId = -1;
int routeId = -1;
String rfidId = "";

const float tensionCuerdaArribaMax = 1;
const float tensionCaidaMax = 15;   
const float tensionReposoMax = 4;  
const float tensionEscaladorMax = 8;

const float tensionCuerdaArribaMin = -1; 
const float tensionCaidaMin = 9;   
const float tensionReposoMin = 1.5;  
const float tensionEscaladorMin = 6;

int timeSection = 0;

unsigned long startTime = 0;
unsigned long startTimeTension = 0; 
bool timerStartedTension = false; 

bool espera = false;
 float tensionActual = 0;
HX711 scale;

#define SDA_PIN 21  // Configuración de pines I2C en el ESP32
#define SCL_PIN 22

// Crear un objeto Servo
Servo myservo;  // Librería de John K. Bennett




//Prueba Tingspeak - Dato: temperatura
// Variables para control de la temperatura
float temperaturaAnterior = 25.0; // Valor inicial de temperatura
// Temporizador basado en millis
unsigned long intervaloEnvio = 30000; // 30 segundos
unsigned long tiempoAnterior = 0;




#include <Wire.h>
#include <Adafruit_PN532.h>

// Crear una instancia del PN532 usando la interfaz I2C
Adafruit_PN532 nfc(SDA_PIN, SCL_PIN);



#define MYSQL_DEBUG_PORT Serial
// Debug Level from 0 to 4
#define _MYSQL_LOGLEVEL_ 1


#define USING_HOST_NAME true


uint16_t server_port = 3306;
char server[] = "us-cluster-east-01.k8s.cleardb.net";
char default_database[] = "heroku_caf1501af1ac492";

char default_table[] = "authentication_customuser";

// Crear instancia de MySQL_Connection
MySQL_Connection conn((Client *)&client);



void setup() {
  // Iniciar comunicación serial
  Serial.begin(115200);
  while (!Serial && millis() < 1000)
    ;  // Esperar a que se conecte el puerto serie

  Serial.println("Iniciando el lector NFC PN532...");

  Wire.begin(SDA_PIN, SCL_PIN);
  nfc.begin();

  uint32_t versiondata = nfc.getFirmwareVersion();
  if (!versiondata) {
    Serial.print("No se encontró el PN53x. Revisa las conexiones.");
    // while (1) ;  // parar aquí
    for (int i = 0; i < 50; i++) {
      Serial.print(".");
      delay(500);
    }
  }
  
  pinMode(pinZumbador, OUTPUT);

  // Mostrar información del firmware
  Serial.print("Encontrado PN5");
  Serial.print((versiondata >> 24) & 0xFF, HEX);
  Serial.print(" Firmware ver. ");
  Serial.print((versiondata >> 16) & 0xFF, DEC);
  Serial.print('.');
  Serial.println((versiondata >> 8) & 0xFF, DEC);

  // Configurar el módulo para operar en modo RFID con la frecuencia de 106 kbps
  nfc.SAMConfig();

  Serial.println("Esperando una tarjeta NFC...");

  // Configurar los pines de los sensores ultrasónicos
  for (int i = 0; i < 4; i++) {
    pinMode(trigPins[i], OUTPUT);
    pinMode(echoPins[i], INPUT);
  }

  myservo.attach(servoPin);  // Adjuntar el servo al pin definido

  // Inicializar el servo en 0 grados
  myservo.write(0);
  Serial.println("Servo en 0 grados");


  hx711Init();

  Serial.println("\n Starting MYSQL Motor");

  MYSQL_DISPLAY1("\nStarting Combined_Insert_Select_ESP on", ARDUINO_BOARD);
  MYSQL_DISPLAY(MYSQL_MARIADB_GENERIC_VERSION);

  // Conectar a la red WiFi
  MYSQL_DISPLAY1("Connecting to", ssid);

  WiFi.begin(ssid, pass);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    MYSQL_DISPLAY0(".");
  }

  // Imprimir información de la conexión
  MYSQL_DISPLAY1("Connected to network. My IP address is:", WiFi.localIP());

  MYSQL_DISPLAY3("Connecting to SQL Server @", server, ", Port =", server_port);
  MYSQL_DISPLAY5("User =", user, ", PW =", password, ", DB =", default_database);





}




int medirAltura(int distanciaAConsiderar) {
  int resultado = 0;
  for (int i = 0; i < 4; i++) {  // Se utilizan los 4 sensores
    long duration;
    float distance;
    // Medir la distancia del sensor
    digitalWrite(trigPins[i], LOW);
    delayMicroseconds(2);
    digitalWrite(trigPins[i], HIGH);
    delayMicroseconds(10);
    digitalWrite(trigPins[i], LOW);
    duration = pulseIn(echoPins[i], HIGH);
    distance = duration * 0.034 / 2;
    //Serial.print("S");
    //Serial.print(i + 1);
    //Serial.print(": ");
    //Serial.print(distance);
    //Serial.print("cm | ");

    if (distance < distanciaAConsiderar) {
      resultado = alturas[i];
    }
  }
  //Serial.println("");
  return resultado;
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
void alarma(int tipoAlarma) {
  int frecuencia;
  unsigned long alarmTime = millis();

  // Selecciona la frecuencia en función del tipo de alarma
  switch(tipoAlarma) {
    case 1: //CUERDA ARRIBA
      frecuencia = 1000;  // Tono 1: 1000 Hz
      enviarMensaje("Atención administrador: CUERDA ARRIBA");
      break;
    case 2: //PRE-ALARMA
      frecuencia = 1200;  // Tono 2: 1200 Hz
      enviarMensaje("Atención administrador: ALGUIEN COMENZÓ A ESCALAR SIN ASEGURARSE. POSICIÓN: TRAMO 1");
      break;
    case 3://DOS ESCALADORES EN LA PARED
      frecuencia = 1500;  // Tono 3: 1500 Hz
      enviarMensaje("Atención administrador: DOS ESCALADORES EN LA PARED");
      break;
    case 4://PERSONA ESCALANDO SIN ASEGURARSE
      frecuencia = 1800;  // Tono 4: 1800 Hz
      enviarMensaje("Atención administrador: PERSONA ESCALANDO SIN ASEGURARSE EN ALTURA PELIGROSA ");
      break;
    case 5: //ALARMA CHALECO DESCONECTAD0
      frecuencia = 2000;  // Tono 5: 2000 Hz
      enviarMensaje("Atención administrador: ESCALADOR SE HA QUITADO EL MOSQUETÓN MIENTRAS ESCALABA");
      break;
    default:
      frecuencia = 1000;  // Valor por defecto si no se reconoce el tipo
  }
   while (millis() - alarmTime < 5000) {  // Mantener la alarma durante 10 segundos
    tone(pinZumbador, frecuencia, 500);  // Genera un tono durante 500 ms (puedes ajustar el tiempo si es necesario)
    delay(500);  // Espera para el siguiente ciclo del tono
  }
}



int runReadCustomUserByRFID(String rfidTag)
{
  // Construimos la consulta SQL para seleccionar el id del usuario donde el rfid_tag coincide con el valor proporcionado
  String SELECT_SQL = String("SELECT id, name FROM ") + default_database + "." + default_table +
                      String(" WHERE rfid_tag = '") + rfidTag + "'";

  MySQL_Query query_mem = MySQL_Query(&conn);
  int userId = -1;  // Inicializamos con un valor que indica que no se encontró ningún usuario

  if (conn.connected())
  {
    Serial.print("------------------------------ SENTENCIA SQL:   ");
    MYSQL_DISPLAY(SELECT_SQL);

    if (query_mem.execute(SELECT_SQL.c_str()))
    {
      // Obtener las columnas primero
      column_names *cols = query_mem.get_columns();

      if (cols) {
        row_values *row = NULL;

        do {
          row = query_mem.get_next_row();
          if (row != NULL) {
            MYSQL_DISPLAY3("USUARIO ENCONTRADO CON ID: ", row->values[0], ", Nombre: ", row->values[1]);
            userId = atoi(row->values[0]);  // Convertir el ID del usuario a un entero
          } else {
            // Si no se encuentra el RFID, imprimir un mensaje adecuado
            Serial.println(" ");
          }
        } while (row != NULL);
      }
      else {
        MYSQL_DISPLAY("Error reading columns");
      }
    }
    else
    {
      MYSQL_DISPLAY("Select error");
    }
  }
  else
  {
    MYSQL_DISPLAY("Disconnected from Server. Can't read 1.");
  }

  return userId;  // Devuelve el ID del usuario o -1 si no se encontró
}


bool isTheFirstRoute(int userId) {
  // Construimos la consulta SQL para seleccionar entrenamientos del usuario con loaded = 0
  String SELECT_SQL = String("SELECT id FROM ") + default_database + ".training_training " +
                      String(" WHERE user_id = ") + userId + " AND loaded = 0 LIMIT 1;";

  MySQL_Query query_mem = MySQL_Query(&conn);
  bool isFirstRoute = true;  // Asumimos inicialmente que es la primera ruta

  if (conn.connected()) {
    Serial.print("------------------------------ SENTENCIA SQL:   ");
    MYSQL_DISPLAY(SELECT_SQL);

    if (query_mem.execute(SELECT_SQL.c_str())) {
      // Obtener las columnas primero
      column_names *cols = query_mem.get_columns();

      if (cols) {
        row_values *row = NULL;

        // Verificamos si existe algún resultado
        row = query_mem.get_next_row();
        if (row != NULL) {
          // Si hay un resultado, significa que hay un entrenamiento con loaded = 0
          isFirstRoute = false;
          trainingId = atoi(row->values[0]);  // Guardar el ID del entrenamiento en la variable global
          MYSQL_DISPLAY3("ID de entrenamiento encontrado: ", row->values[0], ", con loaded = 0", "");
        } else {
          // Si no hay resultados, es la primera ruta
          Serial.println("No se encontraron entrenamientos con loaded = 0 para este usuario.");
          trainingId = -1;  // Asegurarse de que trainingId esté en un estado seguro
        }
      } else {
        MYSQL_DISPLAY("Error reading columns");
      }
    } else {
      MYSQL_DISPLAY("Select error");
    }
  } else {
    MYSQL_DISPLAY("Disconnected from Server 2. Can't read.");
  }

  return isFirstRoute;  // Devuelve true si es la primera ruta, false si ya tiene entrenamientos con loaded = 0
}

void createTrainingForUser(int userId) {
  String trainingName = "Entrenamiento";
  int duration = 0;
  String notes = "Ninguna";

  // Usar DATE(NOW()) en MySQL para obtener solo la fecha actual
  String INSERT_SQL = String("INSERT INTO ") + default_database + ".training_training (user_id, name, date, duration, notes, loaded, created_at, updated_at) VALUES (" +
                      String(userId) + ", '" + trainingName + "', DATE(NOW()), " +
                      String(duration) + ", '" + notes + "', 0, NOW(), NOW());";

  MySQL_Query query_mem = MySQL_Query(&conn);

  if (conn.connected()) {
    Serial.print("------------------------------ SENTENCIA SQL:   ");
    MYSQL_DISPLAY(INSERT_SQL);

    if (query_mem.execute(INSERT_SQL.c_str())) {
      Serial.println("Entrenamiento creado con éxito.");

      // Ahora vamos a obtener el ID del entrenamiento recién creado
      String SELECT_NEW_SQL = String("SELECT id FROM ") + default_database + ".training_training " +
                              String(" WHERE user_id = ") + userId + " ORDER BY created_at DESC LIMIT 1;";

      MySQL_Query query_mem_new = MySQL_Query(&conn);

      if (query_mem_new.execute(SELECT_NEW_SQL.c_str())) {
        column_names *cols_new = query_mem_new.get_columns();

        if (cols_new) {
          row_values *row_new = query_mem_new.get_next_row();
          if (row_new != NULL) {
            trainingId = atoi(row_new->values[0]);  // Guardar el ID del nuevo entrenamiento en la variable global
            MYSQL_DISPLAY3("Nuevo entrenamiento creado con ID: ", row_new->values[0], "", "");
          } else {
            Serial.println("Error: No se pudo obtener el ID del nuevo entrenamiento.");
          }
        } else {
          MYSQL_DISPLAY("Error leyendo las columnas para el nuevo entrenamiento");
        }
      } else {
        MYSQL_DISPLAY("Error al seleccionar el nuevo entrenamiento");
      }
    } else {
      MYSQL_DISPLAY("Error al crear el entrenamiento.");
    }
  } else {
    MYSQL_DISPLAY("Desconectado del servidor. No se pudo crear el entrenamiento.");
  }
  conn.close();
}

void createClimbedRouteSession(int trainingSessionId) {
  int default_climbed_route_id = 2;
  int defaultValue = 0;
  int id = 0;
  
  if (trainingSessionId <= 0) {
    Serial.println("Error: trainingSessionId no es válido.");
    return;
  }

  // Consulta SQL para insertar una nueva fila
  String INSERT_SQL = String("INSERT INTO ") + default_database + ".training_climbedroutetrainingsession " +
                      "(training_session_id, climbed_route_id, date, updated_at, time_taken, fells, completed, timeSection_1, timeSection_2, timeSection_3, timeSection_4) " +
                      "VALUES (" +
                      String(trainingSessionId) + ", " +
                      String(default_climbed_route_id) + ", " +
                      "DATE(NOW()), NOW(), " +
                      String(defaultValue) + ", " +
                      String(defaultValue) + ", " +
                      String(defaultValue) + ", " +
                      String(defaultValue) + ", " +
                      String(defaultValue) + ", " +
                      String(defaultValue) + ", " +
                      String(defaultValue) + ");";

  MySQL_Query query_mem = MySQL_Query(&conn);

  // Comprobar la conexión a la base de datos
  if (conn.connected()) {
    Serial.println(" ");
  } else {
    Serial.println("Error: No hay conexión con la base de datos.");
    return;
  }

  // Ejecutar la consulta de inserción y comprobar si tuvo éxito
  Serial.print("------------------------------ SENTENCIA SQL:   ");
  MYSQL_DISPLAY(INSERT_SQL);
  if (query_mem.execute(INSERT_SQL.c_str())) {
    Serial.println("Sesión creada con éxito.");
  } else {
    Serial.println("Error al ejecutar la consulta de inserción. Verifica la consulta y la conexión.");
    return;
  }
  // Consulta para obtener el último ID asociado a ese training_session_id usando MAX(id)
  String SELECT_SQL = String("SELECT MAX(id) AS last_id FROM ") + default_database + ".training_climbedroutetrainingsession " +
                      "WHERE training_session_id = " + String(trainingSessionId) + ";";

  MySQL_Query query_mem_new = MySQL_Query(&conn);
  MYSQL_DISPLAY(SELECT_SQL);
  // Comprobar la conexión antes de la consulta SELECT
  if (!conn.connected()) {
    Serial.println("Error: Se perdió la conexión a la base de datos antes de la consulta de selección.");
    return;
  }else{

  // Ejecutar la consulta de selección y comprobar si fue exitosa
  if (query_mem_new.execute(SELECT_SQL.c_str())) {
    column_names *cols_new = query_mem_new.get_columns();
    if (cols_new) {
    // Comprobar si la consulta devolvió alguna fila
    row_values *row = query_mem_new.get_next_row();
      if (row != NULL) {
        id = atoi(row->values[0]);
        rutaActivaId = id;  // Convertir el resultado a entero
        Serial.print("ID de la nueva sesión: ");
        Serial.println(rutaActivaId);
      } else {
        Serial.println("Error: No se devolvió ninguna fila con SELECT MAX(id).");
        rutaActivaId = 0;  // Asegurarse de que rutaActivaId esté en un estado seguro
      }
    }else {
        MYSQL_DISPLAY("Error leyendo las columnas para el nuevo entrenamiento");
    }
  } else {
    Serial.println("Error al ejecutar la consulta SELECT MAX(id). Verifica la consulta y la conexión.");
    return;
  }

  }

  conn.close();
}




void updateSectionTime(int sectionNumber) {
  unsigned long currentTime = millis();
  int timeTaken = (currentTime - startTime) / 1000; // Tiempo en segundos
  enviarDatos(timeTaken);
  String column;
  if (sectionNumber == 1) {
    timeSection = timeTaken;
    column = "timeSection_1";
  } else if (sectionNumber == 2) {
    timeSection = timeSection + timeTaken;
    column = "timeSection_2";
    startTime = millis(); // Reiniciar el temporizador para la siguiente sección
  } else if (sectionNumber == 3) {
    timeSection = timeSection + timeTaken;
    column = "timeSection_3";
    startTime = millis(); // Reiniciar el temporizador para la siguiente sección
  } else if (sectionNumber == 4) {
    timeSection = timeSection + timeTaken;
    column = "timeSection_4";
    startTime = millis(); // Reiniciar el temporizador para la siguiente sección
  } else {
    return;
  }
 
  String UPDATE_SQL = String("UPDATE ") + default_database + ".training_climbedroutetrainingsession SET " + column +
                      " = " + String(timeTaken) + " WHERE id = " + String(rutaActivaId) + ";";

  MySQL_Query query_mem = MySQL_Query(&conn);

  
  if (conn.connected()) {
    Serial.print("------------------------------ SENTENCIA SQL:   ");
    MYSQL_DISPLAY(UPDATE_SQL);
    if (query_mem.execute(UPDATE_SQL.c_str())) {
      Serial.println("Se actualizó " + column + " con el valor " + String(timeTaken));
    } else {
      MYSQL_DISPLAY("Error al actualizar la base de datos.");
    }
  } else {
    MYSQL_DISPLAY("Desconectado del servidor. No se pudo actualizar la base de datos.");
  }
  conn.close();
}

void completeClimbingSession() {


  // Consulta SQL para actualizar las columnas "completed" y "time_taken"
  String UPDATE_SQL = String("UPDATE ") + default_database + ".training_climbedroutetrainingsession SET " +
                      "completed = true, time_taken = " + String(timeSection) + 
                      " WHERE id = " + String(rutaActivaId) + ";";

  // Crear el objeto para ejecutar la consulta
  MySQL_Query query_mem = MySQL_Query(&conn);

  // Comprobar si la conexión a la base de datos está establecida
  if (conn.connected()) {
    MYSQL_DISPLAY(UPDATE_SQL);

    // Ejecutar la consulta y comprobar si tuvo éxito
    if (query_mem.execute(UPDATE_SQL.c_str())) {
      Serial.println("La sesión de escalada se completó con éxito. Tiempo total: " + String(timeSection) + " segundos.");
    } else {
      MYSQL_DISPLAY("Error al actualizar la base de datos.");
    }
  } else {
    MYSQL_DISPLAY("Desconectado del servidor. No se pudo actualizar la base de datos.");
  }
  
  // Cerrar la conexión
  conn.close();
}

void incompletedClimbedSession() {
  // Consulta SQL para actualizar las columnas "fells", "completed" y "time_taken"
  String UPDATE_SQL = String("UPDATE ") + default_database + ".training_climbedroutetrainingsession SET " +
                      "fells = 1, completed = false, time_taken = " + String(timeSection) +
                      " WHERE id = " + String(rutaActivaId) + ";";

  // Crear el objeto para ejecutar la consulta
  MySQL_Query query_mem = MySQL_Query(&conn);

  // Comprobar si la conexión a la base de datos está establecida
  if (conn.connected()) {
    Serial.print("------------------------------ SENTENCIA SQL:   ");
    MYSQL_DISPLAY(UPDATE_SQL);

    // Ejecutar la consulta y comprobar si tuvo éxito
    if (query_mem.execute(UPDATE_SQL.c_str())) {
      Serial.println("La sesión incompleta fue registrada con éxito.");
    } else {
      MYSQL_DISPLAY("Error al actualizar la base de datos.");
    }
  } else {
    MYSQL_DISPLAY("Desconectado del servidor. No se pudo actualizar la base de datos.");
  }
  
  // Cerrar la conexión
  conn.close();
}


void leerTarjeta() {
  uint8_t success;
  uint8_t uid[] = { 0, 0, 0, 0, 0, 0, 0 };
  uint8_t uidLength;


  success = nfc.readPassiveTargetID(PN532_MIFARE_ISO14443A, uid, &uidLength, 100);
  if (success) {
    // Si se ha detectado una tarjeta, mostrar el UID
    Serial.println("Tarjeta detectada!");
    Serial.print("UID Length: "); Serial.print(uidLength, DEC); Serial.println(" bytes");
    Serial.print("UID Value: ");

    rfidId = "";
    for (uint8_t i = 0; i < uidLength; i++) {
      Serial.print(uid[i], HEX);
      rfidId += String(uid[i], HEX);
    }
    Serial.println("");

    // Convertir el UID a mayúsculas (opcional, depende de cómo se almacene en la base de datos)
    rfidId.toUpperCase();

    MYSQL_DISPLAY("Connecting...");

    if (conn.connectNonBlocking(server, server_port, user, password) != RESULT_FAIL) {
      delay(500);

      // Llamada a la función para buscar el RFID en la base de datos

      usuarioId = runReadCustomUserByRFID(rfidId);


      conn.close(); // Cerrar la conexión
    } else {
      MYSQL_DISPLAY("\nConnect failed. Trying again on next iteration.");
    }
  }

}



////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


void abrirPuerta();
bool tensionChange();
bool chalecoOk();
void comeinzaAscenso();
int registrar(int eventoRecibido);

int estado;

void loop() {

  ////////////////////////////// Timer cada 30/60s envio de dato de prueba a Thingspeak

  static  unsigned long tiempoActual = millis();

  // Verificar si ha pasado el intervalo de tiempo
  // if (tiempoActual - tiempoAnterior >= intervaloEnvio) {
  //   enviarDatos(); // Enviar los datos a ThingSpeak
  //   tiempoAnterior = tiempoActual; // Actualizar el tiempo anterior
  // }



  ////////////////////////////// PRUEBA TELEGRAM 1 MENSAJE c/5 minutos
  // static long timerTG = -60 * 1000 * 5;
  // static int numeroCincoMinutos;

  // if (millis() > 60 * 1000 * 5 + timerTG) {
  //   timerTG = millis();
  //   if (timerTG < 60 * 1000) {
  //     enviarMensaje("ESP32 conectado a Telegram!");

  //   } else {
  //     enviarMensaje("ESP32 manteniendo conexión " + (String)numeroCincoMinutos);

  //   }
  //   numeroCincoMinutos++;
  // }


  // Medir la altura con los sensores ultrasónicos
  int alturaActual = medirAltura(10);  // Pasar un argumento entero a la función

  //pruebaDeRegistro();

  /*
    Serial.print("Altura: ");
    Serial.println(alturaActual);
    Serial.println("");
    static unsigned long timerNfc;
    if (millis() > 100 + timerNfc) {
    timerNfc = millis();
    leerTarjeta();
    }
  */

  static unsigned long millisInicioActividad;
 
     

  switch (estado) {
    case 0:  /////////////////////////////////////////////////////////// CASO PARA PODER INICIAR
      tensionActual = 0;
      usuarioId = -1;
      leerTarjeta();
      Serial.println("Estado: 0 - Iniciar");
      if (alturaActual > 0) {
        estado = 2;
      }
      else if (usuarioId != -1) {
        for (int i = 0; i < 3; i++) {
          if (!conn.connected() && conn.connectNonBlocking(server, server_port, user, password) != RESULT_FAIL) {
            delay(500);  // Espera para asegurar la reconexión
          }
        }

        if (conn.connected()) {
          if (isTheFirstRoute(usuarioId)) {
            createTrainingForUser(usuarioId);
          }else{
            conn.close();
          }
          
          estado = 10;  // Cambio a estado 10 si se detecta una tarjeta válida
        } else {
          MYSQL_DISPLAY("Fallo al reconectar con la base de datos.");
        }// Cambio a estado 10 si se detecta una tarjeta válida es decir asociada a un usuario
      }
      break;


    case 2:  //Estado PRE-ALARMA
      Serial.println("Estado: 2 - Pre-alarma");
      if (alturaActual >= alturas[1]) estado = 4;
      else {
        alarma(2);
        estado = 0;
      }
      break;

    case 4:  //ALARMA ESCALADOR HA COMENZADO A ESCALAR 
      Serial.println("Estado: 4 - Alarma");
      alarma(4);
      estado = 0;
      break;

    case 10:  //Inicio actividad
      Serial.println("Estado: 10 - Inicio de actividad");
      abrirPuerta();
      millisInicioActividad = millis();
      Serial.println("Estado: 12 - Esperando que cojan la cuerda");
      estado = 12;
      break;



    case 12:  //////////////////////// ESPERANDO QUE COJAN LA CUERDA
      
      if (tensionChange()){
        Serial.print("tensionAnterior: 2");
        Serial.print("   tensionActual: ");
        Serial.println(tensionActual);
        Serial.println("Estado: 15 - Han cogido la cuerda");
        for (int i = 0; i < 5; i++) {
          if (!conn.connected() && conn.connectNonBlocking(server, server_port, user, password) != RESULT_FAIL) {
            delay(500);  // Espera para asegurar la reconexión
          }
        }
        if (conn.connected()) {
          createClimbedRouteSession(trainingId);  // Crear la sesión de ruta escalada
          startTime = millis();
          estado = 18;
          registrar(18);
        } else {
          MYSQL_DISPLAY("Fallo al reconectar con la base de datos.");
          estado = 0;  // Si falla la conexión, regresar al estado inicial
        }

        estado = 15;
      
      }
      else {
        if (millis() > millisInicioActividad + 35 * 1000) {  //Timeout si abandona el juego
          Serial.println("Apaga actividad");
          estado = 0;
        }
      }
      break;

    case 15:  //HAN COGIDO LA CUERDA. (A)ESPERANDO LEER RFID. (B)SI LA ALTURA AUMENTA, DEBE IR A ALARMA!
      tensionActual = medirTension();
      Serial.println(tensionActual);
      if (alturaActual >= alturas[1]){
        Serial.println("detetectoPesoReposo1");
        estado = 40;
      }
      else if (tensionActual >= tensionReposoMin && tensionActual <= tensionReposoMax){
         Serial.println("detetectoPesoReposo");
         estado = 12;
      }
      else if (tensionActual >= tensionCuerdaArribaMin && tensionActual <= tensionCuerdaArribaMax){
        Serial.println("detetectoPesoReposoo");
         estado = 41;
      }
      else if (tensionActual >= tensionEscaladorMin && tensionActual <= tensionEscaladorMax){
        Serial.println("Se ha detetectado: Tensión escalador");
         estado = 18;
      }
      // else{
      //   Serial.println("Estado: 18 - Primer tramo");
      //   estado = 18;
      // }
      
      break;

    //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
                                                                //COMIENZA SUBIDA
    ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

    case 18:  ///////////////////////////// 1/4 //Primer tramo
      tensionActual = medirTension();
       Serial.print("E");
       Serial.print(alturaActual);
      if (chalecoOk() == false) {
         Serial.println("Estado: 34 - Segundo amo");
        estado = 46;
        //Se soltó el chaleco
      } 
      else if (tensionActual >= tensionCaidaMin && tensionActual <= tensionCaidaMax){ //SE HA CAÍDO
        estado = 32;
      }
      else if (alturaActual == 1) {
          Serial.println("Estado: 20 - Segundo tramo");
          for (int i = 0; i < 5; i++) {
            if (!conn.connected() && conn.connectNonBlocking(server, server_port, user, password) != RESULT_FAIL) {
              delay(500);  // Espera para asegurar la reconexión
            }
          }

          if (conn.connected()) {
            updateSectionTime(1); 
            registrar(20);
            Serial.println("Estado: 20 - Segundo tramo");
            estado = 20;
          } else {
            MYSQL_DISPLAY("Fallo al reconectar con la base de datos.");
            estado = 0;  // Si falla la conexión, regresar al estado inicial
          }
          
      }  //Siguiente nivel
      else if (alturaActual > 1){
        Serial.println("Estado: 44 - Escalador en posicion indebida");
        estado = 44;
      }
      break;

    case 20:  ///////////////////////////// 2/4
      tensionActual = medirTension();
      if (!chalecoOk()) {
        estado = 46;  //Se soltó el chaleco
      }
      else if (tensionActual >= tensionCaidaMin && tensionActual <= tensionCaidaMax){ //SE HA CAÍDO
        estado = 32;
      }
      else if (alturaActual == 2) {
        for (int i = 0; i < 5; i++) {
            if (!conn.connected() && conn.connectNonBlocking(server, server_port, user, password) != RESULT_FAIL) {
              delay(500);  // Espera para asegurar la reconexión
            }
          }

          if (conn.connected()) {
            updateSectionTime(2); 
            registrar(22);
            Serial.println("Estado: 22 - Tercer tramo");
            estado = 22;
          } else {
            MYSQL_DISPLAY("Fallo al reconectar con la base de datos.");
            estado = 0;  // Si falla la conexión, regresar al estado inicial
          }
      }else if (alturaActual == 3 || alturaActual == 4){
        estado = 44;
      }

      break;

    case 22:  ///////////////////////////// 3/4
      if (!chalecoOk()) estado = 46;  //Se soltó el chaleco
      if (alturaActual == alturas[2]) {
        for (int i = 0; i < 5; i++) {
            if (!conn.connected() && conn.connectNonBlocking(server, server_port, user, password) != RESULT_FAIL) {
              delay(500);  // Espera para asegurar la reconexión
            }
          }

          if (conn.connected()) {
            updateSectionTime(3); 
            registrar(24);
            Serial.println("Estado: 24 - Cuarto tramo");
            estado = 24;
          } else {
            MYSQL_DISPLAY("Fallo al reconectar con la base de datos.");
            estado = 0;  // Si falla la conexión, regresar al estado inicial
          }
      }else if (alturaActual == 1 || alturaActual == 4){
        estado = 44;
      }
      break;

    case 24:  ///////////////////////////// 4/4
      if (!chalecoOk()) estado = 46;  //Se soltó el chaleco
      if (alturaActual == alturas[3]) {
        for (int i = 0; i < 5; i++) {
            if (!conn.connected() && conn.connectNonBlocking(server, server_port, user, password) != RESULT_FAIL) {
              delay(500);  // Espera para asegurar la reconexión
            }
          }

          if (conn.connected()) {
            updateSectionTime(4); 
            registrar(26);
            Serial.println("Estado: 26 - Llegaste al final: esperando a que bajes");
            estado = 26;
          } else {
            MYSQL_DISPLAY("Fallo al reconectar con la base de datos.");
            estado = 0;  // Si falla la conexión, regresar al estado inicial
          }
      }else if (alturaActual == 1 || alturaActual == 2){
        estado = 44;
      }
      break;

    case 26:  /////////////////////////////// Subida terminada
      for (int i = 0; i < 5; i++) {
            if (!conn.connected() && conn.connectNonBlocking(server, server_port, user, password) != RESULT_FAIL) {
              delay(500);  // Espera para asegurar la reconexión
            }
      }

      if (conn.connected()) {
            completeClimbingSession();
            registrar(26);
            Serial.println("Estado: 30 - Sesion guardada en la base de datos");
            estado = 30;
      } else {
            MYSQL_DISPLAY("Fallo al reconectar con la base de datos.");
            estado = 0;  // Si falla la conexión, regresar al estado inicial
      }
      
      break;

      

    case 30:
      Serial.println("Estado: 30");
      tensionActual = medirTension();
      if (tensionActual >= tensionCaidaMin && tensionActual <= tensionCaidaMax){
        
        estado = 60;
      }
      break;

    case 32:
      for (int i = 0; i < 5; i++) {
        if (!conn.connected() && conn.connectNonBlocking(server, server_port, user, password) != RESULT_FAIL) {
            delay(500);  // Espera para asegurar la reconexión
        }
      }

      if (conn.connected()) {
            incompletedClimbedSession();
            estado = 60;
      } else {
            MYSQL_DISPLAY("Fallo al reconectar con la base de datos.");
            estado = 0;  // Si falla la conexión, regresar al estado inicial
      }
      break;

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
                                                                //ALARMAS
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    case 40: //DETECTO ESCALADOR EN POSICIÓN INDEBIDA HABIÉNDO COMENZADO LA ACTIVIDAD
      Serial.println("Estado: 40 - Alarma durante la actividad");
      alarma(4);
      estado = 0;
      break;

    case 41:
      // Solo iniciamos el temporizador una vez
      if (!timerStartedTension) {
        startTimeTension = millis();  // Guardamos el tiempo de inicio
        timerStartedTension = true;   // Indicamos que el temporizador ha comenzado
      }

      // Comprobar si ya han pasado 10 segundos
      if (millis() - startTimeTension >= 10000) {  // 10,000 ms = 10 segundos
        tensionActual = medirTension();     // Medir la tensión después de los 10 segundos

        if (tensionActual >= tensionCuerdaArribaMin && tensionActual <= tensionCuerdaArribaMax) {
          estado = 42;  // Cambiar al estado 42 si la tensión es correcta
        } else {
          estado = 15;  // Cambiar al estado 15 si la tensión no está en el rango
        }

        timerStartedTension = false;  // Reiniciar el temporizador para futuros usos
      }
      break;

    case 42: //CUERDA ARRIBA
      Serial.println("Estado: 42");
      alarma(1); 
      estado = 0;
      break;

    case 44: //DOS ESCALADORES EN LA PARED
      Serial.println("Estado: 34");
      alarma(3); 
      estado = 0;
      break;

    case 46: //ESCALADOR SE HA DESENGANCHADO
      Serial.println("Estado: 46 - Chaleco suelto durante la actividad");
      alarma(5);
      if (chalecoOk()) {//Busca cuál fue el último caso registrado en la subida para volver a él al detectar RFID
        estado = registrar(-100); 
      }else{
        estado = 0;
      } 
      break;

    
    case 60:
      Serial.println("ENTRENAMIENTO TERMINADO");
      enviarDatos(0);
      estado = 0;
      break;




    case 99:
      Serial.println("Estado: 99");
      break;

    case 100:
      Serial.println("Estado: 100");
      break;

    case 101:
      Serial.println("Estado: 101");
      break;

    case 102:
      Serial.println("Estado: 102");
      break;
  }
}





float medirTension() {
  static float lastTensionEnLaCuerda;
  scale.power_up();
  // Medir con el sensor HX711
  //Serial.print("one reading:\t");
  //Serial.print(scale.get_units(), 1);
  //Serial.print("\t| average:\t");
  //Serial.println(scale.get_units(10), 5);
  float tensionEnLaCuerda = scale.get_units(10);
  scale.power_down();  // poner el ADC en modo de suspensión
  // delay(5000);
  lastTensionEnLaCuerda = tensionEnLaCuerda;
  return tensionEnLaCuerda;
}

// %

void comeinzaAscenso() {
  //Registrar en el servidor
}

bool tensionChange() {
  // Debe devolver true cuando concluya que una persona está manipulando la cuerda
  const int OFFSET_TENSION = 2;
  static float tensionAnterior = 2.5;  // Mantener el valor entre invocaciones
  static unsigned long timerSensorChange = 0;  // Mantener el temporizador entre invocaciones

  // Comprobar si han pasado 10 ms desde la última medición
  if (millis() - timerSensorChange >= 10) {
    tensionActual = medirTension();  // Medir la tensión actual

    // Comprobar si la tensión cambió significativamente
    if (tensionActual > tensionAnterior + OFFSET_TENSION) return true;
    if (tensionActual < tensionAnterior - OFFSET_TENSION) return true;

    // Actualizar el valor de la tensión anterior para la siguiente lectura
    tensionAnterior = tensionActual;
    
    // Actualizar el temporizador para la siguiente medición
    timerSensorChange = millis();
  }
  return false;
}




void abrirPuerta() {
  Serial.println("Abrir Puerta");
  myservo.write(90);
  delay(1000);
  myservo.write(0);
}

bool chalecoOk(){
  return true;
}
// bool chalecoOk() {
//   unsigned long startTime = millis();
//   String detectedRfid = "";

//   while (millis() - startTime < 3000) {  // Espera 3 segundos
//     uint8_t success;
//     uint8_t uid[] = { 0, 0, 0, 0, 0, 0, 0 };  
//     uint8_t uidLength;     

//     success = nfc.readPassiveTargetID(PN532_MIFARE_ISO14443A, uid, &uidLength, 100);
//     if (success) {
//       detectedRfid = "";  // Limpiar el valor anterior
//       for (uint8_t i = 0; i < uidLength; i++) {
//         detectedRfid += String(uid[i], HEX);
//       }
//       detectedRfid.toUpperCase();

//       if (detectedRfid == rfidId) {
//         Serial.println("Chaleco OK: Tarjeta RFID coincide");
//         return true;  // Tarjeta detectada coincide con la del usuario actual
//       }
//     }
//     delay(100);  // Pequeño retardo para evitar la saturación del bus
//   }

//   Serial.println("Chaleco NO OK: Tarjeta RFID no coincide o no detectada");
//   return true;  // Si
// }

//Registra los eventos en el servidor, si le paso -100, informa cuál fue el útimo número registrado
int registrar(int eventoRecibido) {

  static int lastRegistro;

  if (eventoRecibido == -100) return lastRegistro;
  /*

    20 Juego comenzado
    22 primer nivel alcanzado
    24 segundo nuevel alcanzaco
    26 tercer nivel alcanzado
  */
  
  lastRegistro = eventoRecibido;
  return eventoRecibido;
}




void runInsert() {
  String default_value = "Hola Pepe!";
  String INSERT_SQL = String("INSERT INTO ") + default_database + "." + default_table
                      + " (valor1) VALUES ('" + default_value + "')";
  MySQL_Query query_mem = MySQL_Query(&conn);

  if (conn.connected()) {
    MYSQL_DISPLAY(INSERT_SQL);

    if (!query_mem.execute(INSERT_SQL.c_str())) {
      MYSQL_DISPLAY("Insert error");
    } else {
      MYSQL_DISPLAY("Data Inserted.");
    }
  } else {
    MYSQL_DISPLAY("Disconnected from Server. Can't insert.");
  }
}

void runRead() {
  String SELECT_SQL = String("SELECT id, valor1 FROM ") + default_database + "." + default_table;

  MySQL_Query query_mem = MySQL_Query(&conn);

  if (conn.connected()) {
    Serial.print("------------------------------ SENTENCIA SQL:   ");
    MYSQL_DISPLAY(SELECT_SQL);

    if (query_mem.execute(SELECT_SQL.c_str())) {
      // Obtener las columnas primero
      column_names *cols = query_mem.get_columns();

      if (cols) {
        row_values *row = NULL;

        do {
          row = query_mem.get_next_row();
          if (row != NULL) {
            MYSQL_DISPLAY3("ID: ", row->values[0], ", Valor1: ", row->values[1]);
          }
        } while (row != NULL);
      } else {
        MYSQL_DISPLAY("Error reading columns");
      }
    } else {
      MYSQL_DISPLAY("Select error");
    }
  } else {
    MYSQL_DISPLAY("Disconnected from Server. Can't read.");
  }
}


void pruebaDeRegistro() {

  static unsigned long timerRegister;

  if (millis() > timerRegister + 60000) {
    timerRegister = millis();

    MYSQL_DISPLAY("Connecting...");

    if (conn.connectNonBlocking(server, server_port, user, password) != RESULT_FAIL) {
      delay(500);

      // Llamada a la función para insertar datos
      runInsert();

      // Llamada a la función para leer datos
      runRead();

      conn.close();  // Cerrar la conexión
    } else {
      MYSQL_DISPLAY("\nConnect failed. Trying again on next iteration.");
    }

    MYSQL_DISPLAY("\nSleeping...");
    MYSQL_DISPLAY("================================================");
  }
}


void hx711Init() {

  // Configurar el sensor HX711
  Serial.println("HX711 Demo");
  Serial.println("Initializing the scale");

  scale.begin(LOADCELL_DOUT_PIN, LOADCELL_SCK_PIN);

  Serial.println("Before setting up the scale:");
  Serial.print("read: \t\t");
  Serial.println(scale.read());  // print a raw reading from the ADC

  Serial.print("read average: \t\t");
  Serial.println(scale.read_average(20));  // print the average of 20 readings from the ADC

  Serial.print("get value: \t\t");
  Serial.println(scale.get_value(5));  // print the average of 5 readings from the ADC minus the tare weight (not set yet)

  Serial.print("get units: \t\t");
  Serial.println(scale.get_units(5), 1);  // print the average of 5 readings from the ADC minus tare weight (not set) divided
  // by the SCALE parameter (not set yet)

  scale.set_scale(-9116.11);
  scale.tare();  // reset the scale to 0

  Serial.println("After setting up the scale:");
  Serial.print("read: \t\t");
  Serial.println(scale.read());  // print a raw reading from the ADC

  Serial.print("read average: \t\t");
  Serial.println(scale.read_average(20));  // print the average of 20 readings from the ADC

  Serial.print("get value: \t\t");
  Serial.println(scale.get_value(5));  // print the average of 5 readings from the ADC minus the tare weight, set with tare()

  Serial.print("get units: \t\t");
  Serial.println(scale.get_units(5), 1);  // print the average of 5 readings from the ADC minus tare weight, divided
  // by the SCALE parameter set with set_scale

  //Serial.println("Readings:");
  ///////////////////////
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
void enviarDatos(int timeSection) {
  if (WiFi.status() == WL_CONNECTED) { // Asegurarse de que estamos conectados a WiFi
    HTTPClient http;
    
    String url = String(serverTS) + "/update?api_key=" + apiKey + "&field1=" + String(timeSection);

    Serial.print("Enviando datos a ThingSpeak: ");
    Serial.println(url);

    http.begin(url); // Iniciar conexión HTTP
    int httpCode = http.GET(); // Realizar solicitud GET

    if (httpCode > 0) {
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
  enviarDatos(0);
}



void enviarMensaje(String mensaje) {
  if(WiFi.status() == WL_CONNECTED) { // Verificar la conexión WiFi
    HTTPClient http;
    String url = "https://api.telegram.org/bot" + botToken + "/sendMessage?chat_id=" + chatID + "&text=" + mensaje;
    
    http.begin(url); // Iniciar la conexión HTTP
    int httpCode = http.GET(); // Realizar la solicitud GET

    if(httpCode > 0) {
      String payload = http.getString();
      Serial.print("Mensaje enviado, respuesta del servidor: ");
      Serial.println(payload);
    } else {
      Serial.print("Error en la solicitud HTTP: ");
      Serial.println(http.errorToString(httpCode).c_str());
    }

    http.end(); // Finalizar la conexión HTTP
  } else {
    Serial.println("Error: No conectado a WiFi");
  }
}
