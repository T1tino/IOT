// Pin donde está conectado el OUT del módulo de fotoresistencia
const int sensorPin = 34;

void setup() {
  Serial.begin(115200);  // Inicia comunicación Serial
  delay(1000);           // Espera un poco para que se estabilice
  Serial.println("Lectura de sensor de luz iniciada...");
}

void loop() {
  int valorLuz = analogRead(sensorPin); // Lee el valor del sensor
  Serial.print("Valor de luz: ");
  Serial.println(valorLuz);             // Muestra el valor en Serial

  delay(1000); // Espera 1 segundo
}

