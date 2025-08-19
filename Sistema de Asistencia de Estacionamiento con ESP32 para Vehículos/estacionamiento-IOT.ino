#include <HTTP_Method.h>
#include <Middlewares.h>
#include <Uri.h>
#include <WebServer.h>

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

// ===============================================
// CÓDIGO FINAL UNIFICADO ESP32 - SENSOR WEB COMPLETO
// Sistema de medición de distancia con interfaz web, historial, gráficas y exportación
// ===============================================

// 🔧 Configuración de red Wi-Fi
const char* ssid = "ARRIS-AAE2";
const char* password = "C3Y4M1D487870816";

// 🌐 Servidor Web
WebServer server(80);

// 📍 Pines del sensor y actuadores - ÚNICOS Y CORREGIDOS
const int trigPin = 5;        // Pin físico D5 - Trigger ultrasónico
const int echoPin = 18;       // Pin físico D18 - Echo ultrasónico
const int ledPin = 2;         // Pin físico D2 - LED indicador principal
const int servoLedPin = 4;    // Pin físico D4 - LED que simula servo
const int botonPin = 19;      // Pin físico D19 - Botón físico (opcional)

// 🔁 Variables del sistema
bool sistemaActivo = false;

// 🌍 Página web completa con todas las funcionalidades
const char* htmlPage = R"rawliteral(
<!DOCTYPE html>
<html>
  <head>
    <meta charset="utf-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>🚀 Sensor Web ESP32</title>
    <script src="https://cdnjs.cloudflare.com/ajax/libs/Chart.js/3.9.1/chart.min.js"></script>
    <style>
      body { 
        font-family: 'Segoe UI', Tahoma, Geneva, Verdana, sans-serif; 
        text-align: center; 
        margin: 0; 
        padding: 0; 
        background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
        color: #333;
        min-height: 100vh;
      }
      
      .container {
        background: white;
        margin: 20px auto;
        padding: 30px;
        border-radius: 15px;
        box-shadow: 0 10px 30px rgba(0,0,0,0.2);
        max-width: 95%;
      }
      
      h1 { 
        margin-top: 0;
        color: #4a5568;
        text-shadow: 2px 2px 4px rgba(0,0,0,0.1);
      }
      
      button {
        padding: 15px 30px;
        font-size: 18px;
        font-weight: bold;
        background: linear-gradient(45deg, #4299e1, #3182ce);
        color: white;
        border: none;
        border-radius: 10px;
        cursor: pointer;
        margin: 10px;
        transition: all 0.3s ease;
        box-shadow: 0 4px 15px rgba(66, 153, 225, 0.4);
      }
      
      button:hover {
        transform: translateY(-2px);
        box-shadow: 0 6px 20px rgba(66, 153, 225, 0.6);
      }
      
      #estado { 
        font-size: 24px; 
        margin: 20px 0; 
        font-weight: bold;
        padding: 15px;
        border-radius: 10px;
        background: #f7fafc;
        border-left: 5px solid #4299e1;
      }
      
      #resultado { 
        font-size: 18px; 
        margin: 20px 0; 
        white-space: pre-line;
        padding: 15px;
        background: #f8f9fa;
        border-radius: 8px;
        border: 1px solid #e2e8f0;
      }
      
      table {
        margin: 20px auto;
        border-collapse: collapse;
        width: 100%;
        background: white;
        border-radius: 8px;
        overflow: hidden;
        box-shadow: 0 4px 6px rgba(0,0,0,0.1);
      }
      
      th, td {
        border: 1px solid #e2e8f0;
        padding: 12px 8px;
        font-size: 14px;
        text-align: center;
      }
      
      th {
        background: linear-gradient(45deg, #4299e1, #3182ce);
        color: white;
        font-weight: bold;
      }
      
      tr:nth-child(even) {
        background-color: #f8f9fa;
      }
      
      .chart-container {
        margin: 30px auto;
        padding: 20px;
        background: white;
        border-radius: 10px;
        box-shadow: 0 2px 10px rgba(0,0,0,0.1);
      }
      
      .status-indicators {
        display: flex;
        justify-content: space-around;
        margin: 20px 0;
        flex-wrap: wrap;
      }
      
      .indicator {
        padding: 10px 20px;
        margin: 5px;
        border-radius: 20px;
        font-weight: bold;
        min-width: 120px;
      }
      
      .indicator.safe {
        background: #c6f6d5;
        color: #22543d;
        border: 2px solid #38a169;
      }
      
      .indicator.danger {
        background: #fed7d7;
        color: #742a2a;
        border: 2px solid #e53e3e;
      }
      
      .indicator.inactive {
        background: #e2e8f0;
        color: #4a5568;
        border: 2px solid #a0aec0;
      }
    </style>
  </head>
  <body>
    <div class="container">
      <h1>🚀 Sistema de Medición ESP32</h1>
      
      <button id="botonSistema" onclick="toggleSistema()">🔴 Sistema Apagado</button>
      
      <div id="estado">Sistema Inactivo</div>
      
      <div class="status-indicators">
        <div id="ledStatus" class="indicator inactive">💡 LED: Apagado</div>
        <div id="servoStatus" class="indicator inactive">🔄 Servo: Reposo</div>
      </div>
      
      <div id="resultado">📊 Esperando mediciones...</div>

      <h3>📊 Historial de Lecturas</h3>
      <table id="tablaHistorial">
        <tr><th>#</th><th>Distancia (cm)</th><th>Estado LED</th><th>Estado Servo</th><th>Hora</th></tr>
      </table>

      <button onclick="exportarCSV()">💾 Exportar a Excel</button>
      <button onclick="limpiarHistorial()">🗑️ Limpiar Historial</button>

      <div class="chart-container">
        <h3>📈 Gráfica de Distancia en Tiempo Real</h3>
        <canvas id="graficaDistancia" width="400" height="200"></canvas>
      </div>
    </div>

    <script>
      let encendido = false;
      let intervalo;
      let contador = 1;
      let grafica;
      let datosGrafica = {
        labels: [],
        datasets: [{
          label: "Distancia (cm)",
          data: [],
          borderColor: "#4299e1",
          backgroundColor: "rgba(66, 153, 225, 0.1)",
          fill: true,
          tension: 0.4,
          pointBackgroundColor: "#3182ce",
          pointBorderColor: "#2b6cb0",
          pointRadius: 4
        }]
      };

      // Inicializar gráfica
      window.onload = function() {
        let ctx = document.getElementById("graficaDistancia").getContext("2d");
        grafica = new Chart(ctx, {
          type: 'line',
          data: datosGrafica,
          options: {
            responsive: true,
            plugins: {
              legend: {
                display: true,
                position: 'top'
              }
            },
            scales: {
              y: { 
                beginAtZero: true,
                title: {
                  display: true,
                  text: 'Distancia (cm)'
                }
              },
              x: {
                title: {
                  display: true,
                  text: 'Tiempo'
                }
              }
            },
            animation: {
              duration: 750
            }
          }
        });
      };

      function toggleSistema() {
        encendido = !encendido;
        let accion = encendido ? "encender" : "apagar";

        fetch("/" + accion)
          .then(response => response.text())
          .then(data => {
            document.getElementById("botonSistema").innerText = encendido ? "🟢 Sistema Encendido" : "🔴 Sistema Apagado";
            document.getElementById("estado").innerText = encendido ? "🟢 Sistema Activo - Midiendo..." : "⚪ Sistema Inactivo";
            document.getElementById("estado").style.borderLeftColor = encendido ? "#38a169" : "#a0aec0";
            document.getElementById("resultado").innerText = data;
          });

        if (encendido) {
          intervalo = setInterval(() => {
            fetch("/medir")
              .then(r => r.text())
              .then(d => {
                procesarRespuesta(d);
              });
          }, 1000);
        } else {
          clearInterval(intervalo);
          document.getElementById("resultado").innerText = "🛑 Sistema apagado. Sin lecturas activas.";
          actualizarIndicadores("inactivo", "inactivo");
        }
      }

      function procesarRespuesta(data) {
        document.getElementById("resultado").innerText = data;
        
        // Extraer datos
        let lineas = data.split("\n");
        let distanciaLinea = lineas.find(l => l.includes("Distancia:"));
        if (!distanciaLinea) return;
        
        let distancia = distanciaLinea.split(":")[1].trim().replace("cm", "").trim();
        let esSeguro = data.includes("segura") || data.includes("apagado");
        let esPeligroso = data.includes("peligrosa") || data.includes("encendido");
        
        let estadoLed = esPeligroso ? "encendido" : "apagado";
        let estadoServo = esPeligroso ? "activo" : "reposo";
        let hora = new Date().toLocaleTimeString();

        // Agregar a tabla
        let tabla = document.getElementById("tablaHistorial");
        let fila = tabla.insertRow(-1);
        fila.insertCell(0).innerText = contador++;
        fila.insertCell(1).innerText = distancia;
        fila.insertCell(2).innerText = estadoLed.toUpperCase();
        fila.insertCell(3).innerText = estadoServo.toUpperCase();
        fila.insertCell(4).innerText = hora;

        // Colorear fila según estado
        if (esPeligroso) {
          fila.style.backgroundColor = "#fed7d7";
        } else {
          fila.style.backgroundColor = "#c6f6d5";
        }

        // Actualizar indicadores
        actualizarIndicadores(estadoLed, estadoServo);

        // Actualizar gráfica
        datosGrafica.labels.push(hora);
        datosGrafica.datasets[0].data.push(Number(distancia));
        if (datosGrafica.labels.length > 15) {
          datosGrafica.labels.shift();
          datosGrafica.datasets[0].data.shift();
        }
        grafica.update('none');
      }

      function actualizarIndicadores(estadoLed, estadoServo) {
        let ledIndicator = document.getElementById("ledStatus");
        let servoIndicator = document.getElementById("servoStatus");
        
        // Actualizar LED
        ledIndicator.className = "indicator " + (estadoLed === "encendido" ? "danger" : estadoLed === "apagado" ? "safe" : "inactive");
        ledIndicator.innerText = "💡 LED: " + estadoLed.toUpperCase();
        
        // Actualizar Servo
        servoIndicator.className = "indicator " + (estadoServo === "activo" ? "danger" : estadoServo === "reposo" ? "safe" : "inactive");
        servoIndicator.innerText = "🔄 Servo: " + estadoServo.toUpperCase();
      }

      function exportarCSV() {
        let tabla = document.getElementById("tablaHistorial");
        let filas = tabla.querySelectorAll("tr");
        let csv = [];

        for (let fila of filas) {
          let columnas = fila.querySelectorAll("th, td");
          let filaCSV = Array.from(columnas).map(c => `"${c.innerText}"`).join(",");
          csv.push(filaCSV);
        }

        let contenido = csv.join("\n");
        let blob = new Blob([contenido], { type: "text/csv;charset=utf-8;" });
        let enlace = document.createElement("a");
        enlace.href = URL.createObjectURL(blob);
        enlace.download = "sensor_historial_" + new Date().toISOString().split('T')[0] + ".csv";
        enlace.click();
      }

      function limpiarHistorial() {
        if (confirm("¿Estás seguro de que quieres limpiar todo el historial?")) {
          let tabla = document.getElementById("tablaHistorial");
          let filas = tabla.querySelectorAll("tr:not(:first-child)");
          filas.forEach(fila => fila.remove());
          contador = 1;
          
          // Limpiar gráfica
          datosGrafica.labels = [];
          datosGrafica.datasets[0].data = [];
          grafica.update();
        }
      }
    </script>
  </body>
</html>
)rawliteral";

// 📏 Función para medir distancia
long medirDistancia() {
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);
  
  long duracion = pulseIn(echoPin, HIGH);
  long distancia = duracion * 0.034 / 2;
  return distancia;
}

// ⚙️ Configuración inicial
void setup() {
  Serial.begin(115200);
  Serial.println("🚀 Iniciando Sistema ESP32...");

  // Configurar pines
  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);
  pinMode(ledPin, OUTPUT);
  pinMode(servoLedPin, OUTPUT);
  pinMode(botonPin, INPUT_PULLUP);  // Botón físico opcional
  
  // Estados iniciales
  digitalWrite(ledPin, LOW);
  digitalWrite(servoLedPin, LOW);

  // Conectar a Wi-Fi
  WiFi.begin(ssid, password);
  Serial.print("🔗 Conectando a WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  
  Serial.println("\n✅ Conectado exitosamente!");
  Serial.print("📡 IP del ESP32: ");
  Serial.println(WiFi.localIP());
  Serial.println("🌐 Accede desde tu navegador a la IP mostrada arriba");

  // Configurar rutas del servidor web
  server.on("/", []() { 
    server.send(200, "text/html", htmlPage); 
  });

  server.on("/encender", []() {
    sistemaActivo = true;
    long distancia = medirDistancia();
    String respuesta = "✅ Sistema activado\n📏 Distancia inicial: " + String(distancia) + " cm";
    server.send(200, "text/plain", respuesta);
    Serial.println("🟢 Sistema activado por interfaz web");
  });

  server.on("/apagar", []() {
    sistemaActivo = false;
    digitalWrite(ledPin, LOW);
    digitalWrite(servoLedPin, LOW);
    server.send(200, "text/plain", "🛑 Sistema desactivado\n💤 Todos los actuadores apagados");
    Serial.println("🔴 Sistema desactivado por interfaz web");
  });

  server.on("/medir", []() {
    if (sistemaActivo) {
      long distancia = medirDistancia();
      String resultado = "📏 Distancia: " + String(distancia) + " cm\n";
      
      bool botonPresionado = digitalRead(botonPin) == LOW;
      
      // Lógica de control (30cm es la distancia crítica)
      if (distancia > 0 && distancia <= 30) {
        digitalWrite(ledPin, HIGH);
        digitalWrite(servoLedPin, HIGH);
        resultado += "🚨 ¡Distancia peligrosa! LED encendido.\n🔄 Servo activado (posición 90°)";
      } else {
        digitalWrite(ledPin, LOW);
        digitalWrite(servoLedPin, LOW);
        resultado += "✅ Zona segura. LED apagado.\n🔄 Servo en reposo (posición 0°)";
      }
      
      if (botonPresionado) {
        resultado += "\n🔘 Botón físico presionado";
      }

      server.send(200, "text/plain", resultado);
      
      // Log en consola
      Serial.print("📊 Distancia: ");
      Serial.print(distancia);
      Serial.print("cm | LED: ");
      Serial.print(distancia <= 30 ? "ON" : "OFF");
      Serial.print(" | Servo: ");
      Serial.println(distancia <= 30 ? "ACTIVO" : "REPOSO");
      
    } else {
      digitalWrite(ledPin, LOW);
      digitalWrite(servoLedPin, LOW);
      server.send(200, "text/plain", "💤 Sistema inactivo");
    }
  });

  server.begin();
  Serial.println("🌐 Servidor web iniciado correctamente");
  Serial.println("📱 Listo para recibir conexiones!");
}

// 🔄 Bucle principal
void loop() {
  server.handleClient();
  
  // Control opcional con botón físico
  static bool ultimoEstadoBoton = HIGH;
  bool estadoBotonActual = digitalRead(botonPin);
  
  if (ultimoEstadoBoton == HIGH && estadoBotonActual == LOW) {
    // Botón presionado - toggle del sistema
    sistemaActivo = !sistemaActivo;
    
    if (!sistemaActivo) {
      digitalWrite(ledPin, LOW);
      digitalWrite(servoLedPin, LOW);
    }
    
    Serial.print("🔘 Botón físico presionado - Sistema: ");
    Serial.println(sistemaActivo ? "ACTIVADO" : "DESACTIVADO");
    
    delay(50); // Debounce
  }
  ultimoEstadoBoton = estadoBotonActual;
  
  delay(10); // Pequeña pausa para estabilidad
}