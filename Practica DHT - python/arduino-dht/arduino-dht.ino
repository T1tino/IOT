#include <FB_Const.h>
#include <FB_Error.h>
#include <FB_Network.h>
#include <FB_Utils.h>
#include <Firebase.h>
#include <FirebaseESP32.h>
#include <FirebaseFS.h>
#include <MB_File.h>

#include <DHT.h>
#include <DHT_U.h>

#include <ArduinoJson.h>
#include <ArduinoJson.hpp>

// === Configuración del sensor DHT ===
#define DHTPIN 4       // Pin de datos del DHT
#define DHTTYPE DHT11  // Tipo de sensor DHT (DHT11 o DHT22)

DHT dht(DHTPIN, DHTTYPE);  // Crear objeto DHT

// === Firebase Config ===
#define API_KEY "TU_API_KEY"  // Firebase Web API Key
#define DATABASE_URL "ttps://monitoreoambientaltijuan-7c502-default-rtdb.firebaseio.com"  // URL de Realtime DB

void setup() {
  Serial.begin(115200);   // Inicia comunicación serial
  dht.begin();            // Inicia el sensor DHT
}

void loop() {
  // Leer temperatura y humedad
  float temp = dht.readTemperature();
  float hum = dht.readHumidity();

  // Crear documento JSON
  StaticJsonDocument<200> doc;
  doc["temperatura"] = temp;
  doc["humedad"] = hum;

  // Imprimir JSON por el puerto serial
  serializeJson(doc, Serial);
  Serial.println();  // Salto de línea para mejor lectura

  delay(5000); // Espera 5 segundos antes de la siguiente lectura
}