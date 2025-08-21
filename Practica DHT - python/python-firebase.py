import serial.tools.list_ports
import serial
import json
import time
import firebase_admin
from firebase_admin import credentials, db

# 1. Cargar credenciales del archivo .json que descargaste
cred = credentials.Certificate("monitoreoambientaltijuan-7c502-firebase-adminsdk.json")

# 2. Conectarse a Firebase con la URL de tu base de datos
firebase_admin.initialize_app(cred, {
    'databaseURL': 'https://monitoreoambientaltijuan-7c502-default-rtdb.firebaseio.com/'
})

# Detectar el puerto usb
def detectar_puerto():
    puertos = list(serial.tools.list_ports.comports())
    for p in puertos:
        if 'Arduino' in p.description or 'USB Serial' in p.description or 'ttyUSB' in p.device:
            return p.device
    if not puertos:
        print("No se encontraron puertos disponibles.")
    else:
        print("No se encontró un puerto que parezca un Arduino. Verifica la conexión.")
        for p in puertos:
            print(f"Encontrado: {p.device} - {p.description}")
    return None

puerto = detectar_puerto()
print(f"Puerto detectado: {puerto}") # Mostrar el puerto seleccionado


# 3. Configurar puerto y velocidad del ESP32
ser = serial.Serial(puerto, 115200)  # Asegúrate de que es el puerto correcto

time.sleep(2)  # Esperar a que la conexión esté lista

while True:
    try:
        # 4. Leer línea desde el ESP32
        linea = ser.readline().decode('utf-8').strip()
        datos = json.loads(linea)

        # 5. Enviar a Firebase
        ref = db.reference('lecturas/dht11')
        ref.push({
            'temperatura': datos['temperatura'],
            'humedad': datos['humedad'],
            'fecha': time.strftime("%Y-%m-%d %H:%M:%S")
        })

        print("Enviado:", datos)

    except KeyboardInterrupt:
        print("Programa detenido.")
        ser.close()
        break
    except Exception as e:
        print("Error:", e)
