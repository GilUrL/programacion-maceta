#include <WiFi.h>
#include <HTTPClient.h>

// Pines para los LEDs de estado
#define LED_CONNECTED 18    // LED verde para conexión estable
#define LED_DISCONNECTED 19 // LED rojo para pérdida de conexión

// Configuración HC-12 (usando Serial2 hardware del ESP32)
#define HC12_RX_PIN 16 
#define HC12_TX_PIN 17  
#define HC12_SET_PIN 5  // Pin para modo comando del HC-12

// Variables para el control de conexión
unsigned long lastMessageTime = 0;
const unsigned long CONNECTION_TIMEOUT = 5000; // 5 segundos sin datos = desconexión

// Variables para almacenar los datos recibidos
float temperature = 0;
float humidity = 0;
int soilMoisture = 0;
int lightLevel = 0;

// Datos WiFi
const char* ssid = "TP-Link_6032";
const char* password = "98204496";

void setup() {
    Serial.begin(115200);
    Serial2.begin(9600, SERIAL_8N1, HC12_RX_PIN, HC12_TX_PIN);

    pinMode(LED_CONNECTED, OUTPUT);
    pinMode(LED_DISCONNECTED, OUTPUT);
    digitalWrite(LED_CONNECTED, LOW);
    digitalWrite(LED_DISCONNECTED, HIGH);

    pinMode(HC12_SET_PIN, OUTPUT);
    digitalWrite(HC12_SET_PIN, HIGH);

    // Conectar al WiFi
    WiFi.begin(ssid, password);
    Serial.print("Conectando a WiFi");
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    Serial.println("\nWiFi conectado");
    Serial.println(WiFi.localIP());

    Serial.println("Receptor HC-12 inicializado");
    Serial.println("Esperando datos...");
}

void loop() {
    if (Serial2.available()) {
        String receivedData = Serial2.readStringUntil('\n');
        receivedData.trim();

        if (receivedData.length() > 0) {
            processReceivedData(receivedData);
            lastMessageTime = millis();
            updateConnectionStatus(true);
            printReceivedData();
        }
    }

    if (millis() - lastMessageTime > CONNECTION_TIMEOUT) {
        updateConnectionStatus(false);
    }
}

void processReceivedData(String data) {
    int tempIndex = data.indexOf("T=");
    int humIndex = data.indexOf("H=");
    int soilIndex = data.indexOf("S=");
    int lightIndex = data.indexOf("L=");

    if (tempIndex != -1 && humIndex != -1 && soilIndex != -1 && lightIndex != -1) {
        temperature = data.substring(tempIndex + 2, data.indexOf(',', tempIndex)).toFloat();
        humidity = data.substring(humIndex + 2, data.indexOf(',', humIndex)).toFloat();
        soilMoisture = data.substring(soilIndex + 2, data.indexOf(',', soilIndex)).toInt();
        lightLevel = data.substring(lightIndex + 2).toInt();
    }
}

void updateConnectionStatus(bool connected) {
    if (connected) {
        digitalWrite(LED_CONNECTED, HIGH);
        digitalWrite(LED_DISCONNECTED, LOW);
    } else {
        digitalWrite(LED_CONNECTED, LOW);
        digitalWrite(LED_DISCONNECTED, HIGH);
        Serial.println("¡Alerta! Pérdida de conexión con el emisor");
    }
}

void printReceivedData() {
    Serial.println("\n--- Datos Recibidos ---");
    Serial.print("Temperatura: ");
    Serial.print(temperature, 1);
    Serial.println(" °C");

    Serial.print("Humedad Ambiente: ");
    Serial.print(humidity, 1);
    Serial.println(" %");

    Serial.print("Humedad del Suelo: ");
    Serial.print(soilMoisture);
    Serial.println(" %");

    Serial.print("Nivel de Luz: ");
    Serial.print(lightLevel);
    Serial.println(" %");
    Serial.println("----------------------");

    sendDataToAPI();
}

void sendDataToAPI() {
    if (WiFi.status() == WL_CONNECTED) {
        HTTPClient http;

        http.begin("https://plantatech.ultrasoftware.pro/api/registrar_lecturas");
        http.addHeader("Content-Type", "application/json");

        String jsonPayload = "{";
        jsonPayload += "\"correo\":\"gilurbina09@gmail.com\",";
        jsonPayload += "\"nivel_luz\":\"" + String(lightLevel) + "\",";
        jsonPayload += "\"humedad_aire\":\"" + String(humidity) + "\",";
        jsonPayload += "\"temperatura\":\"" + String(temperature) + "\",";
        jsonPayload += "\"humedad_suelo\":\"" + String(soilMoisture) + "\"";
        jsonPayload += "}";

        int httpResponseCode = http.POST(jsonPayload);

        if (httpResponseCode > 0) {
            String response = http.getString();
            Serial.println("Respuesta del servidor:");
            Serial.println(response);
        } else {
            Serial.print("Error al enviar: ");
            Serial.println(httpResponseCode);
        }

        http.end();
    } else {
        Serial.println("WiFi no conectado, no se puede enviar a la API.");
    }
}
