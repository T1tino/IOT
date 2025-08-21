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

// ========== CONFIGURACIÓN DE PINES ==========
// Pines según la nomenclatura de tu ESP32 DevKit v1
const int pinTrig = 5;       // D5 - Sensor ultrasónico - Trigger
const int pinEcho = 18;      // D18 - Sensor ultrasónico - Echo
const int pinLED = 13;       // D13 - LED de advertencia (rojo)
const int pinLEDServo = 25;  // D25 - LED que simula servo (amarillo/verde)
const int pinBoton = 19;     // D19 - Botón físico de activación

// ========== OBJETOS Y VARIABLES ==========
WebServer server(80);

bool sistemaActivo = false;
long distanciaActual = 0;
const int UMBRAL_PELIGRO = 30; // cm
unsigned long ultimaLectura = 0;
const unsigned long INTERVALO_LECTURA = 200; // ms

// ========== VARIABLES ADICIONALES PARA EL BOTÓN ==========
bool estadoPrevioBoton = HIGH;  // Para detectar cambios
unsigned long ultimoDebounce = 0;
const unsigned long DEBOUNCE_DELAY = 50;  // 50ms de debounce

// ========== DECLARACIONES DE FUNCIONES (PROTOTIPOS) ==========
void configurarPines();
void conectarWiFi();
void configurarServidor();
void mostrarInformacion();
void actualizarSensor();
long medirDistancia();

// ========== PÁGINA WEB COMPLETA ==========
const char* paginaWeb = R"rawliteral(
<!DOCTYPE html>
<html lang="es">
<head>
    <meta charset="utf-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>🚗 Sistema de Estacionamiento</title>
    <script src="https://cdnjs.cloudflare.com/ajax/libs/Chart.js/3.9.1/chart.min.js"></script>
    <style>
        * { margin: 0; padding: 0; box-sizing: border-box; }

        body {
            font-family: 'Segoe UI', Tahoma, Geneva, Verdana, sans-serif;
            background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
            min-height: 100vh;
            color: #333;
        }

        .container {
            max-width: 500px;
            margin: 0 auto;
            padding: 20px;
            background: rgba(255,255,255,0.95);
            border-radius: 15px;
            margin-top: 20px;
            box-shadow: 0 8px 32px rgba(0,0,0,0.1);
        }

        h1 {
            text-align: center;
            color: #4a5568;
            margin-bottom: 30px;
            font-size: 24px;
        }

        .status-card {
            background: #f7fafc;
            padding: 20px;
            border-radius: 10px;
            margin-bottom: 20px;
            border-left: 4px solid #4299e1;
        }

        .btn {
            width: 100%;
            padding: 15px;
            font-size: 18px;
            border: none;
            border-radius: 8px;
            cursor: pointer;
            transition: all 0.3s;
            margin-bottom: 15px;
            font-weight: bold;
        }

        .btn-primary { background: #4299e1; color: white; }
        .btn-success { background: #48bb78; color: white; }
        .btn-danger { background: #f56565; color: white; }
        .btn-secondary { background: #718096; color: white; }

        .btn:hover { transform: translateY(-2px); box-shadow: 0 4px 12px rgba(0,0,0,0.15); }

        .metric {
            display: flex;
            justify-content: space-between;
            align-items: center;
            margin: 10px 0;
            font-size: 16px;
        }

        .metric-value {
            font-weight: bold;
            font-size: 20px;
        }

        .distance-safe { color: #48bb78; }
        .distance-warning { color: #ed8936; }
        .distance-danger { color: #f56565; }

        .chart-container {
            margin: 20px 0;
            height: 250px;
        }

        table {
            width: 100%;
            border-collapse: collapse;
            margin-top: 20px;
            font-size: 14px;
        }

        th, td {
            padding: 8px;
            text-align: center;
            border: 1px solid #e2e8f0;
        }

        th { background: #edf2f7; font-weight: bold; }

        .peligro { background: #fed7d7 !important; }
        .seguro { background: #c6f6d5 !important; }

        @media (max-width: 480px) {
            .container { margin: 10px; padding: 15px; }
            h1 { font-size: 20px; }
            .btn { padding: 12px; font-size: 16px; }
        }
    </style>
</head>
<body>
    <div class="container">
        <h1>🚗 Sistema de Estacionamiento</h1>

        <div class="status-card">
            <div class="metric">
                <span>Estado del Sistema:</span>
                <span id="estadoSistema" class="metric-value">⚪ Inactivo</span>
            </div>
            <div class="metric">
                <span>Distancia Actual:</span>
                <span id="distanciaActual" class="metric-value distance-safe">-- cm</span>
            </div>
            <div class="metric">
                <span>LED Advertencia:</span>
                <span id="estadoLED" class="metric-value">🔘 Apagado</span>
            </div>
            <div class="metric">
                <span>LED Servo:</span>
                <span id="estadoLEDServo" class="metric-value">🔘 Apagado</span>
            </div>
        </div>

        <button id="btnSistema" class="btn btn-primary" onclick="toggleSistema()">
            🔴 Activar Sistema
        </button>

        <div class="chart-container">
            <canvas id="grafica"></canvas>
        </div>

        <button class="btn btn-secondary" onclick="exportarDatos()">
            💾 Exportar Historial
        </button>

        <button class="btn btn-secondary" onclick="limpiarHistorial()">
            🗑️ Limpiar Historial
        </button>

        <table id="tablaHistorial">
            <thead>
                <tr><th>#</th><th>Distancia</th><th>Estado</th><th>Hora</th></tr>
            </thead>
            <tbody></tbody>
        </table>
    </div>

    <script>
        let sistemaEncendido = false;
        let intervaloLectura;
        let contadorLecturas = 1;
        let grafica;

        // Configuración de la gráfica
        const datosGrafica = {
            labels: [],
            datasets: [{
                label: 'Distancia (cm)',
                data: [],
                borderColor: '#4299e1',
                backgroundColor: 'rgba(66, 153, 225, 0.1)',
                borderWidth: 2,
                fill: true,
                tension: 0.4
            }]
        };

        // Inicializar gráfica
        window.onload = function() {
            const ctx = document.getElementById('grafica').getContext('2d');
            grafica = new Chart(ctx, {
                type: 'line',
                data: datosGrafica,
                options: {
                    responsive: true,
                    maintainAspectRatio: false,
                    plugins: {
                        legend: { display: true, position: 'top' }
                    },
                    scales: {
                        y: {
                            beginAtZero: true,
                            max: 100,
                            title: { display: true, text: 'Distancia (cm)' }
                        },
                        x: {
                            title: { display: true, text: 'Tiempo' }
                        }
                    }
                }
            });
            
            // Iniciar verificación periódica del estado
            setInterval(verificarEstado, 1000);
        };

        // Verificar estado del sistema periódicamente
        async function verificarEstado() {
            try {
                const respuesta = await fetch('/datos');
                const datos = await respuesta.json();
                
                // Sincronizar estado local con el ESP32
                if (sistemaEncendido !== datos.activo) {
                    sistemaEncendido = datos.activo;
                    actualizarInterfaz();
                    
                    if (sistemaEncendido) {
                        iniciarLecturas();
                    } else {
                        detenerLecturas();
                    }
                }
                
                if (sistemaEncendido) {
                    actualizarDatos(datos);
                }
            } catch (error) {
                console.error('Error verificando estado:', error);
            }
        }

        // Función principal para activar/desactivar sistema
        async function toggleSistema() {
            sistemaEncendido = !sistemaEncendido;

            try {
                const respuesta = await fetch(`/${sistemaEncendido ? 'activar' : 'desactivar'}`);
                const datos = await respuesta.json();

                actualizarInterfaz();

                if (sistemaEncendido) {
                    iniciarLecturas();
                } else {
                    detenerLecturas();
                }
            } catch (error) {
                console.error('Error:', error);
                alert('Error de conexión con el ESP32');
            }
        }

        // Actualizar elementos de la interfaz
        function actualizarInterfaz() {
            const btnSistema = document.getElementById('btnSistema');
            const estadoSistema = document.getElementById('estadoSistema');

            if (sistemaEncendido) {
                btnSistema.textContent = '🟢 Sistema Activo - Desactivar';
                btnSistema.className = 'btn btn-danger';
                estadoSistema.textContent = '🟢 Activo';
                estadoSistema.style.color = '#48bb78';
            } else {
                btnSistema.textContent = '🔴 Activar Sistema';
                btnSistema.className = 'btn btn-primary';
                estadoSistema.textContent = '⚪ Inactivo';
                estadoSistema.style.color = '#718096';

                document.getElementById('distanciaActual').textContent = '-- cm';
                document.getElementById('estadoLED').textContent = '🔘 Apagado';
                document.getElementById('estadoLEDServo').textContent = '🔘 Reposo';
            }
        }

        // Iniciar lecturas periódicas
        function iniciarLecturas() {
            intervaloLectura = setInterval(async () => {
                try {
                    const respuesta = await fetch('/datos');
                    const datos = await respuesta.json();

                    actualizarDatos(datos);
                } catch (error) {
                    console.error('Error en lectura:', error);
                }
            }, 500);
        }

        // Detener lecturas
        function detenerLecturas() {
            if (intervaloLectura) {
                clearInterval(intervaloLectura);
            }
        }

        // Actualizar datos en tiempo real
        function actualizarDatos(datos) {
            const distanciaElement = document.getElementById('distanciaActual');
            const ledElement = document.getElementById('estadoLED');
            const ledServoElement = document.getElementById('estadoLEDServo');

            // Actualizar distancia con colores
            distanciaElement.textContent = `${datos.distancia} cm`;

            if (datos.distancia <= 15) {
                distanciaElement.className = 'metric-value distance-danger';
            } else if (datos.distancia <= 30) {
                distanciaElement.className = 'metric-value distance-warning';
            } else {
                distanciaElement.className = 'metric-value distance-safe';
            }

            // Actualizar estado de los LEDs
            ledElement.textContent = datos.ledAdvertencia ? '🔴 Encendido' : '🔘 Apagado';
            ledElement.style.color = datos.ledAdvertencia ? '#f56565' : '#718096';

            ledServoElement.textContent = datos.ledServo ? '🟡 Activo' : '🔘 Reposo';
            ledServoElement.style.color = datos.ledServo ? '#ed8936' : '#718096';

            // Agregar a la tabla
            agregarATabla(datos.distancia, datos.peligro);

            // Actualizar gráfica
            actualizarGrafica(datos.distancia);
        }

        // Agregar fila a la tabla
        function agregarATabla(distancia, esPeligro) {
            const tbody = document.querySelector('#tablaHistorial tbody');
            const fila = tbody.insertRow(0); // Insertar al inicio

            const hora = new Date().toLocaleTimeString();
            const estado = esPeligro ? 'Peligro' : 'Seguro';

            fila.innerHTML = `
                <td>${contadorLecturas++}</td>
                <td>${distancia} cm</td>
                <td>${estado}</td>
                <td>${hora}</td>
            `;

            fila.className = esPeligro ? 'peligro' : 'seguro';

            // Mantener máximo 50 filas
            if (tbody.rows.length > 50) {
                tbody.deleteRow(50);
            }
        }

        // Actualizar gráfica
        function actualizarGrafica(distancia) {
            const hora = new Date().toLocaleTimeString();

            datosGrafica.labels.push(hora);
            datosGrafica.datasets[0].data.push(distancia);

            // Mantener últimos 20 puntos
            if (datosGrafica.labels.length > 20) {
                datosGrafica.labels.shift();
                datosGrafica.datasets[0].data.shift();
            }

            grafica.update('none'); // Actualización sin animación para mejor rendimiento
        }

        // Exportar datos a CSV
        function exportarDatos() {
            const tabla = document.getElementById('tablaHistorial');
            const filas = tabla.querySelectorAll('tr');
            let csv = 'Numero,Distancia,Estado,Hora\n';

            for (let i = 1; i < filas.length; i++) { // Omitir encabezado
                const celdas = filas[i].querySelectorAll('td');
                const fila = Array.from(celdas).map(celda => celda.textContent).join(',');
                csv += fila + '\n';
            }

            const blob = new Blob([csv], { type: 'text/csv;charset=utf-8;' });
            const enlace = document.createElement('a');
            const url = URL.createObjectURL(blob);

            enlace.setAttribute('href', url);
            enlace.setAttribute('download', `historial_estacionamiento_${new Date().toISOString().slice(0,10)}.csv`);
            enlace.click();
        }

        // Limpiar historial
        function limpiarHistorial() {
            if (confirm('¿Seguro que quieres limpiar el historial?')) {
                document.querySelector('#tablaHistorial tbody').innerHTML = '';
                datosGrafica.labels = [];
                datosGrafica.datasets[0].data = [];
                grafica.update();
                contadorLecturas = 1;
            }
        }
    </script>
</body>
</html>
)rawliteral";

// ========== FUNCIONES PRINCIPALES ==========

void setup() {
    Serial.begin(115200);
    delay(1000);

    // Configurar pines
    configurarPines();

    // Conectar WiFi
    conectarWiFi();

    // Configurar servidor web
    configurarServidor();

    Serial.println("=== Sistema de Estacionamiento Iniciado ===");
    mostrarInformacion();
}

void loop() {
    server.handleClient();

    // ========== LECTURA DEL BOTÓN FÍSICO ==========
    bool estadoActualBoton = digitalRead(pinBoton);
    
    // Detectar cuando se presiona el botón (cambio de HIGH a LOW)
    if (estadoPrevioBoton == HIGH && estadoActualBoton == LOW) {
        delay(50); // Debounce simple
        
        // Verificar que el botón sigue presionado después del debounce
        if (digitalRead(pinBoton) == LOW) {
            // Alternar el estado del sistema
            sistemaActivo = !sistemaActivo;
            
            Serial.println("====================================");
            if (sistemaActivo) {
                Serial.println("🟢 SISTEMA ACTIVADO por botón físico");
            } else {
                Serial.println("🔴 SISTEMA DESACTIVADO por botón físico");
                digitalWrite(pinLED, LOW);
                digitalWrite(pinLEDServo, LOW);
                distanciaActual = 0;
            }
            Serial.println("====================================");
        }
    }
    
    estadoPrevioBoton = estadoActualBoton;
    
    // ========== DEBUG DEL BOTÓN (opcional) ==========
    static unsigned long ultimoPrintDebug = 0;
    if (millis() - ultimoPrintDebug > 2000) { // Cada 2 segundos
        bool estadoBoton = digitalRead(pinBoton);
        Serial.print("Debug - Botón: ");
        Serial.print(estadoBoton == HIGH ? "NO presionado" : "PRESIONADO");
        Serial.print(" | Sistema: ");
        Serial.println(sistemaActivo ? "ACTIVO" : "INACTIVO");
        ultimoPrintDebug = millis();
    }
    
    // ========== OPERACIÓN DEL SENSOR ==========
    if (sistemaActivo) {
        if (millis() - ultimaLectura >= INTERVALO_LECTURA) {
            actualizarSensor();
            ultimaLectura = millis();
        }
    } else {
        // Sistema inactivo - asegurar que todo esté apagado
        digitalWrite(pinLED, LOW);
        digitalWrite(pinLEDServo, LOW);
        distanciaActual = 0;
    }

    delay(10); // Pequeña pausa para estabilidad
}

// ========== CONFIGURACIÓN INICIAL ==========

void configurarPines() {
    pinMode(pinTrig, OUTPUT);
    pinMode(pinEcho, INPUT);
    pinMode(pinLED, OUTPUT);
    pinMode(pinLEDServo, OUTPUT);  // LED que simula servo
    pinMode(pinBoton, INPUT_PULLUP);

    // Asegurar estado inicial
    digitalWrite(pinTrig, LOW);
    digitalWrite(pinLED, LOW);
    digitalWrite(pinLEDServo, LOW);

    Serial.println("✅ Pines configurados correctamente");
}

void conectarWiFi() {
    WiFi.begin(ssid, password);
    Serial.print("Conectando a WiFi");

    int intentos = 0;
    while (WiFi.status() != WL_CONNECTED && intentos < 20) {
        delay(500);
        Serial.print(".");
        intentos++;
    }

    if (WiFi.status() == WL_CONNECTED) {
        Serial.println("\n✅ WiFi conectado exitosamente");
    } else {
        Serial.println("\n❌ Error: No se pudo conectar a WiFi");
        Serial.println("Verifica SSID y contraseña en el código");
    }
}

void configurarServidor() {
    // Página principal
    server.on("/", HTTP_GET, []() {
        server.send(200, "text/html", paginaWeb);
    });

    // Activar sistema
    server.on("/activar", HTTP_GET, []() {
        sistemaActivo = true;
        server.send(200, "application/json", "{\"estado\":\"activado\"}");
        Serial.println("Sistema activado desde web");
    });

    // Desactivar sistema
    server.on("/desactivar", HTTP_GET, []() {
        sistemaActivo = false;
        digitalWrite(pinLED, LOW);
        digitalWrite(pinLEDServo, LOW);
        server.send(200, "application/json", "{\"estado\":\"desactivado\"}");
        Serial.println("Sistema desactivado desde web");
    });

    // Obtener datos actuales
    server.on("/datos", HTTP_GET, []() {
        String json = "{";
        json += "\"distancia\":" + String(distanciaActual) + ",";
        json += "\"ledAdvertencia\":" + String(digitalRead(pinLED) ? "true" : "false") + ",";
        json += "\"ledServo\":" + String(digitalRead(pinLEDServo) ? "true" : "false") + ",";
        json += "\"peligro\":" + String(distanciaActual > 0 && distanciaActual <= UMBRAL_PELIGRO ? "true" : "false") + ",";
        json += "\"activo\":" + String(sistemaActivo ? "true" : "false");
        json += "}";

        server.send(200, "application/json", json);
    });

    // Estado del sistema
    server.on("/estado", HTTP_GET, []() {
        String json = "{\"memoria_libre\":" + String(ESP.getFreeHeap()) + ",";
        json += "\"uptime\":" + String(millis()) + "}";
        server.send(200, "application/json", json);
    });

    server.begin();
    Serial.println("✅ Servidor web iniciado");
}

void mostrarInformacion() {
    Serial.println("\n========== INFORMACIÓN DEL SISTEMA ==========");
    Serial.print("📶 Red WiFi: "); Serial.println(ssid);
    Serial.print("🌐 IP del ESP32: "); Serial.println(WiFi.localIP());
    Serial.print("💾 Memoria libre: "); Serial.println(ESP.getFreeHeap());
    Serial.println("\n========== CONFIGURACIÓN DE PINES ==========");
    Serial.printf("📍 Trigger: D5 (GPIO %d)\n", pinTrig);
    Serial.printf("📍 Echo: D18 (GPIO %d)\n", pinEcho);
    Serial.printf("🔴 LED Advertencia: D13 (GPIO %d)\n", pinLED);
    Serial.printf("🟡 LED Servo: D25 (GPIO %d)\n", pinLEDServo);
    Serial.printf("🔘 Botón: D19 (GPIO %d)\n", pinBoton);
    Serial.println("\n============================================");
    Serial.print("🌐 Accede desde tu navegador: http://");
    Serial.println(WiFi.localIP());
    Serial.println("============================================\n");
}

// ========== SENSOR Y CONTROL ==========

void actualizarSensor() {
    long distancia = medirDistancia();

    // Validar lectura
    if (distancia == 0 || distancia > 400) {
        Serial.println("⚠️ Lectura de sensor inválida");
        return;
    }

    distanciaActual = distancia;

    // Control de dispositivos según distancia
    if (distancia <= UMBRAL_PELIGRO) {
        // PELIGRO: Activar alertas
        digitalWrite(pinLED, HIGH);         // LED rojo encendido
        digitalWrite(pinLEDServo, HIGH);    // LED amarillo/verde encendido (simula servo activo)

        Serial.printf("🚨 PELIGRO: %ld cm - LEDs encendidos (rojo + servo)\n", distancia);
    } else {
        // SEGURO: Desactivar alertas
        digitalWrite(pinLED, LOW);          // LED rojo apagado
        digitalWrite(pinLEDServo, LOW);     // LED servo apagado

        Serial.printf("✅ SEGURO: %ld cm - LEDs apagados\n", distancia);
    }
}

long medirDistancia() {
    // Generar pulso trigger
    digitalWrite(pinTrig, LOW);
    delayMicroseconds(2);
    digitalWrite(pinTrig, HIGH);
    delayMicroseconds(10);
    digitalWrite(pinTrig, LOW);

    // Medir tiempo de respuesta
    long duracion = pulseIn(pinEcho, HIGH, 30000); // Timeout de 30ms

    if (duracion == 0) {
        return 0; // Error en la medición
    }

    // Calcular distancia en cm
    long distancia = duracion * 0.034 / 2;

    return distancia;
}