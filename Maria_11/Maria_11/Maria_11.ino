
#include <MySQL_Generic.h>

#if ! (ESP8266 || ESP32)
  #error This code is intended to run on the ESP8266/ESP32 platform! Please check your Tools->Board setting
#endif

#include "Credentials.h"
#define MYSQL_DEBUG_PORT      Serial
// Debug Level from 0 to 4
#define _MYSQL_LOGLEVEL_      1





// Dr Charles Bell 1.7.2 MySQL_MariaDB_Generic O

#define USING_HOST_NAME     true

char server[] = "us-cluster-east-01.k8s.cleardb.net"; //URL de tu servidor

uint16_t server_port = 3306;

char default_database[] = "heroku_caf1501af1ac492";
char default_table[]    = "tabla";

// Crear instancia de MySQL_Connection
MySQL_Connection conn((Client *)&client);

void setup()
{
  Serial.begin(115200);
  while (!Serial && millis() < 1000); // Esperar a que se conecte el puerto serie

  MYSQL_DISPLAY1("\nStarting Combined_Insert_Select_ESP on", ARDUINO_BOARD);
  MYSQL_DISPLAY(MYSQL_MARIADB_GENERIC_VERSION);

  // Conectar a la red WiFi
  MYSQL_DISPLAY1("Connecting to", ssid);
  
  WiFi.begin(ssid, pass);
  
  while (WiFi.status() != WL_CONNECTED) 
  {
    delay(500);
    MYSQL_DISPLAY0(".");
  }

  // Imprimir información de la conexión
  MYSQL_DISPLAY1("Connected to network. My IP address is:", WiFi.localIP());

  MYSQL_DISPLAY3("Connecting to SQL Server @", server, ", Port =", server_port);
  MYSQL_DISPLAY5("User =", user, ", PW =", password, ", DB =", default_database);
}

void runInsert()
{
  String default_value = "Hola Luisete!";
  String INSERT_SQL = String("INSERT INTO ") + default_database + "." + default_table 
                   + " (valor1) VALUES ('" + default_value + "')";

  MySQL_Query query_mem = MySQL_Query(&conn);

  if (conn.connected())
  {
    MYSQL_DISPLAY(INSERT_SQL);
    
    if ( !query_mem.execute(INSERT_SQL.c_str()) )
    {
      MYSQL_DISPLAY("Insert error");
    }
    else
    {
      MYSQL_DISPLAY("Data Inserted.");
    }
  }
  else
  {
    MYSQL_DISPLAY("Disconnected from Server. Can't insert.");
  }
}

void runRead()
{
  String SELECT_SQL = String("SELECT id, valor1 FROM ") + default_database + "." + default_table;

  MySQL_Query query_mem = MySQL_Query(&conn);

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
            MYSQL_DISPLAY3("ID: ", row->values[0], ", Valor1: ", row->values[1]);
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
}

void loop()
{
  MYSQL_DISPLAY("Connecting...");
  
  if (conn.connectNonBlocking(server, server_port, user, password) != RESULT_FAIL)
  {
    delay(500);

    // Llamada a la función para insertar datos
    runInsert();
    
    // Llamada a la función para leer datos
    runRead();
    
    conn.close(); // Cerrar la conexión
  } 
  else 
  {
    MYSQL_DISPLAY("\nConnect failed. Trying again on next iteration.");
  }

  MYSQL_DISPLAY("\nSleeping...");
  MYSQL_DISPLAY("================================================");
 
  delay(60000); // Esperar 60 segundos antes de repetir
}
