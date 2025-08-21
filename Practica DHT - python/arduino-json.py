import serial
import serial.tools.list_ports
import json
import pandas as pd
from datetime import datetime
import sys

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
baudios = 115200
nombre_archivo = 'datos_dht11.csv'
datos = []

if not puerto:
    sys.exit("Error: No se pudo detectar el puerto del Arduino.")

# Inicia conexión serial
ser = serial.Serial(puerto, baudios)
print(f"Conectado al puerto {puerto}. Esperando datos...")

try:
    while True:
        linea = ser.readline().decode('utf-8', errors='ignore').strip()
        try:
            json_data = json.loads(linea)
            temp = json_data['temperatura']
            hum = json_data['humedad']
            fecha = datetime.now().strftime('%Y-%m-%d %H:%M:%S')

            print(f"{fecha} | Temp: {temp}°C | Hum: {hum}%")
            datos.append({'fecha': fecha, 'temperatura': temp, 'humedad': hum})

            # Guarda en CSV
            df = pd.DataFrame(datos)
            df.to_csv(nombre_archivo, index=False)

        except json.JSONDecodeError:
            print("Dato no válido recibido. Ignorado.")
except KeyboardInterrupt:
    print("Lectura interrumpida. Archivo guardado.")
    ser.close()
