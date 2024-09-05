#include <Wire.h>
#include <Adafruit_PN532.h>
#include <MySQL_Generic.h>

// Configuración de pines I2C en el ESP32
#define SDA_PIN 21
#define SCL_PIN 22
#include "Credentials.h"

#define MYSQL_DEBUG_PORT      Serial
#define _MYSQL_LOGLEVEL_      1
#define USING_HOST_NAME       true

char server[] = "us-cluster-east-01.k8s.cleardb.net"; //URL de tu servidor

uint16_t server_port = 3306;

char default_database[] = "heroku_caf1501af1ac492";
char default_table[]    = "authentication_customuser";

// Crear instancia de MySQL_Connection
MySQL_Connection conn((Client *)&client);

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
  Serial.print((versiondata >> 24) & 0xFF, HEX);
  Serial.print(" Firmware ver. ");
  Serial.print((versiondata >> 16) & 0xFF, DEC);
  Serial.print('.');
  Serial.println((versiondata >> 8) & 0xFF, DEC);

  // Configurar el módulo para operar en modo RFID con la frecuencia de 106 kbps
  nfc.SAMConfig();

  Serial.println("Esperando una tarjeta NFC...");

  while (!Serial && millis() < 1000); // Esperar a que se conecte el puerto serie

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

void runInsert() {
  String default_value = "Hola Luisete!";
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
int runReadCustomUserByRFID(String rfidTag)
{
  // Construimos la consulta SQL para seleccionar el id del usuario donde el rfid_tag coincide con el valor proporcionado
  String SELECT_SQL = String("SELECT id, name FROM ") + default_database + "." + default_table + 
                      String(" WHERE rfid_tag = '") + rfidTag + "'";

  MySQL_Query query_mem = MySQL_Query(&conn);
  int userId = -1;  // Inicializamos con un valor que indica que no se encontró ningún usuario

  if (conn.connected())
  {
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
            MYSQL_DISPLAY3("ID: ", row->values[0], ", Nombre: ", row->values[1]);
            userId = atoi(row->values[0]);  // Convertir el ID del usuario a un entero
          } else {
            // Si no se encuentra el RFID, imprimir un mensaje adecuado
            Serial.println("RFID no asociado a ningún usuario.");
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
    MYSQL_DISPLAY("Disconnected from Server. Can't read.");
  }

  return userId;  // Devuelve el ID del usuario o -1 si no se encontró
}


void createTrainingForUser(int userId, String trainingName, int duration, String notes)
{
  // Usar DATE(NOW()) en MySQL para obtener solo la fecha actual
  String INSERT_SQL = String("INSERT INTO ") + default_database + ".training_training (user_id, name, date, duration, notes, loaded, created_at, updated_at) VALUES (" +
                       String(userId) + ", '" + trainingName + "', DATE(NOW()), " + 
                       String(duration) + ", '" + notes + "', 0, NOW(), NOW());";

  MySQL_Query query_mem = MySQL_Query(&conn);

  if (conn.connected())
  {
    MYSQL_DISPLAY(INSERT_SQL);

    if (query_mem.execute(INSERT_SQL.c_str())) 
    {
      Serial.println("Entrenamiento creado con éxito.");
    }
    else
    {
      MYSQL_DISPLAY("Error al crear el entrenamiento.");
    }
  }
  else
  {
    MYSQL_DISPLAY("Desconectado del servidor. No se pudo crear el entrenamiento.");
  }
}

void loop(void) {
  uint8_t success;
  uint8_t uid[] = { 0, 0, 0, 0, 0, 0 }; // Buffer para almacenar el UID
  uint8_t uidLength; // Tamaño del UID
  int idUser;

  // Esperar a que se detecte una tarjeta
  success = nfc.readPassiveTargetID(PN532_MIFARE_ISO14443A, uid, &uidLength);

  if (success) {
    // Si se ha detectado una tarjeta, mostrar el UID
    Serial.println("Tarjeta detectada!");
    Serial.print("UID Length: "); Serial.print(uidLength, DEC); Serial.println(" bytes");
    Serial.print("UID Value: ");

    String rfidTag = "";
    for (uint8_t i = 0; i < uidLength; i++) {
      Serial.print(uid[i], HEX);
      rfidTag += String(uid[i], HEX);
    }
    Serial.println("");

    // Convertir el UID a mayúsculas (opcional, depende de cómo se almacene en la base de datos)
    rfidTag.toUpperCase();

    MYSQL_DISPLAY("Connecting...");

    if (conn.connectNonBlocking(server, server_port, user, password) != RESULT_FAIL) {
      delay(500);

      // Llamada a la función para buscar el RFID en la base de datos
      
      idUser = runReadCustomUserByRFID(rfidTag);
      

      createTrainingForUser(idUser, "Entrenamiento", 45, "Ninguna");
      conn.close(); // Cerrar la conexión
    } else {
      MYSQL_DISPLAY("\nConnect failed. Trying again on next iteration.");
    }
  }

  // Esperar un momento antes de intentar leer otra vez
  delay(1000);
}

