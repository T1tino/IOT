#include <HTTPClient.h>

#include <WiFi.h>
#include <WiFiAP.h>
#include <WiFiClient.h>
#include <WiFiGeneric.h>
#include <WiFiMulti.h>
#include <WiFiSTA.h>
#include <WiFiScan.h>
#include <WiFiServer.h>
#include <WiFiType.h>
#include <WiFiUdp.h>

// WIFI 1
const char* ssid = "ARRIS-AAE2";
const char* password = "C3Y4M1D487870816";

// Firebase
const char* firebaseHost = "https://ecoluzbc-428be-default-rtdb.firebaseio.com/luz.json"; // Asegúrate de usar .json


// Pines
const int sensorPin = 34;
const int ledPin = 13;
const int umbral = 1000;


void setup() {
  Serial.begin(115200);
  pinMode(ledPin, OUTPUT);


  Serial.println("Conectando al WiFi...");
  WiFi.begin(ssid, password);


  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }


  Serial.println("\nWiFi conectado. Dirección IP: ");
  Serial.println(WiFi.localIP());


  Serial.println("Sistema de control de luz con LED y envío a Firebase.");
}


void loop() {
  int valorLuz = analogRead(sensorPin);
  Serial.print("Luz detectada (valor): ");
  Serial.println(valorLuz);


  // Control del LED
  if (valorLuz > umbral) {
    digitalWrite(ledPin, HIGH);
    Serial.println("Oscuridad → LED encendido");
  } else {
    digitalWrite(ledPin, LOW);
    Serial.println("Luz detectada → LED apagado");
  }


  // Enviar a Firebase
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;


    http.begin(firebaseHost);
    http.addHeader("Content-Type", "application/json");


    String jsonData = "{\"valorLuz\":" + String(valorLuz) + "}";


    int httpResponseCode = http.POST(jsonData);


    Serial.print("Código de respuesta Firebase: ");
    Serial.println(httpResponseCode);


    http.end();
  } else {
    Serial.println("Error de conexión WiFi.");
  }


  delay(10000);  // Tiempo entre lecturas/envíos
}
