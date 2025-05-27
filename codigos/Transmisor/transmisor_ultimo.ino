#include <SPI.h>
#include <TFT_eSPI.h>
#include "DHT.h"

// ----------------------------
// Configuración de la Pantalla TFT y Sensores
// ----------------------------

TFT_eSPI tft = TFT_eSPI();

#define SCREEN_WIDTH 320
#define SCREEN_HEIGHT 240
#define FONT_SIZE 2

// Sensores
#define DHTPIN 18
#define DHTTYPE DHT22
DHT dht(DHTPIN, DHTTYPE);

#define HUMEDAD_SUELO_PIN 35
const int seco = 4095;
const int mojado = 1200;
#define LDR_PIN 4

// Configuración HC-12
#define HC12_RX_PIN 16 
#define HC12_TX_PIN 17  
#define HC12_SET_PIN 19  // Pin para modo comando del HC-12

// ----------------------------
// Variables para Lecturas
// ----------------------------
float humidity, temperature;
int humedadSuelo, porcentajeLuz;
unsigned long lastDHTReadTime = 0;
const unsigned long DHT_READ_INTERVAL = 2000; // 2 segundos entre lecturas

void setup() {
    Serial.begin(115200);
    Serial2.begin(9600, SERIAL_8N1, HC12_RX_PIN, HC12_TX_PIN); 
    
    // Configurar pin SET del HC-12
    pinMode(HC12_SET_PIN, OUTPUT);
    digitalWrite(HC12_SET_PIN, HIGH); // Poner en modo transmisión normal

    // Iniciar sensores
    dht.begin();
    
    // Iniciar pantalla TFT
    tft.init();
    tft.setRotation(0);
    tft.fillScreen(TFT_WHITE);
    tft.setTextColor(TFT_BLACK, TFT_WHITE);
    
    // Mostrar mensaje inicial
    displayInitialMessage();
}

void displayInitialMessage() {
    tft.fillScreen(TFT_WHITE);
    int centerX = SCREEN_WIDTH / 2;
    tft.drawCentreString("Sistema de Monitoreo", centerX, 50, FONT_SIZE);
    tft.drawCentreString("Inicializando...", centerX, 90, FONT_SIZE);
    tft.drawCentreString("Verifique sensores", centerX, 130, FONT_SIZE);
    delay(2000);
}

void loop() {
    unsigned long currentTime = millis();
    
    // Lectura del DHT22 con intervalo controlado
    if (currentTime - lastDHTReadTime >= DHT_READ_INTERVAL) {
        lastDHTReadTime = currentTime;
        
        // Leer sensores
        humidity = dht.readHumidity();
        temperature = dht.readTemperature();
        
        // Mostrar en pantalla TFT
        tft.fillScreen(TFT_WHITE);
        tft.setTextColor(TFT_BLACK, TFT_WHITE);

        int centerX = SCREEN_WIDTH / 2;
        int textY = 30;

        // Verificar si la lectura de DHT fue exitosa
        if (isnan(humidity) || isnan(temperature)) {
            displayError("Error en sensor DHT22");
            Serial.println("Error al leer del sensor DHT22");
            return;
        }

        // Si llegamos aquí, las lecturas son válidas
        readSoilMoisture();
        readLightLevel();
        
        // Mostrar datos en pantalla
        displaySensorData();
        
        // Enviar datos por HC-12
        sendDataViaHC12();
    }
}

void readSoilMoisture() {
    int lecturaSuelo = analogRead(HUMEDAD_SUELO_PIN);
    humedadSuelo = map(lecturaSuelo, seco, mojado, 0, 100);
    humedadSuelo = constrain(humedadSuelo, 0, 100);
}

void readLightLevel() {
    int valorLDR = analogRead(LDR_PIN);
    porcentajeLuz = map(valorLDR, 4095, 0, 0, 100);
    porcentajeLuz = constrain(porcentajeLuz, 0, 100);
}

void displaySensorData() {
    tft.fillScreen(TFT_WHITE);

    int offsetX = -40;  // Ajuste para mover todo a la izquierda (valor negativo)
    int centerX = (SCREEN_WIDTH / 2) + offsetX;  // Centro ajustado
    int centerY = SCREEN_HEIGHT / 2;

    // Dibujar la cara
    tft.drawCircle(centerX, centerY, 60, TFT_BLACK); // contorno de la cara

    // Ojos (ajustados con offsetX)
    tft.fillCircle(centerX - 20, centerY - 25, 5, TFT_BLACK); // ojo izquierdo
    tft.fillCircle(centerX + 20, centerY - 25, 5, TFT_BLACK); // ojo derecho

    // Determinar expresión (todas las coordenadas X usan centerX ajustado)
    if (humedadSuelo < 30) {
        // Cara triste
        tft.drawLine(centerX - 20, centerY + 30, centerX, centerY + 20, TFT_BLACK);
        tft.drawLine(centerX, centerY + 20, centerX + 20, centerY + 30, TFT_BLACK);
    } else if (temperature > 35) {
        // Calor
        tft.drawLine(centerX - 20, centerY + 25, centerX + 20, centerY + 25, TFT_BLACK);
        tft.fillRect(centerX - 5, centerY + 30, 10, 10, TFT_RED); // lengua
    } else if (temperature < 10) {
        // Frío
        tft.drawLine(centerX - 20, centerY + 25, centerX, centerY + 30, TFT_BLACK);
        tft.drawLine(centerX, centerY + 30, centerX + 20, centerY + 25, TFT_BLACK);
        tft.drawLine(centerX - 15, centerY + 35, centerX + 15, centerY + 35, TFT_BLUE);
    } else if (porcentajeLuz < 20) {
        // Dormido
        tft.drawLine(centerX - 25, centerY - 25, centerX - 15, centerY - 25, TFT_BLACK);
        tft.drawLine(centerX + 15, centerY - 25, centerX + 25, centerY - 25, TFT_BLACK);
        tft.drawLine(centerX - 15, centerY + 25, centerX + 15, centerY + 25, TFT_BLACK);
        tft.setTextColor(TFT_BLUE, TFT_WHITE);
        tft.setTextSize(2);
        tft.drawString("Zz", centerX + 30 + offsetX, centerY - 50); // Ajustado con offsetX
    } else {
        // Feliz
        tft.drawLine(centerX - 20, centerY + 20, centerX - 10, centerY + 30, TFT_BLACK);
        tft.drawLine(centerX - 10, centerY + 30, centerX + 10, centerY + 30, TFT_BLACK);
        tft.drawLine(centerX + 10, centerY + 30, centerX + 20, centerY + 20, TFT_BLACK);
    }
    // Texto explicativo (ajustado con offsetX)
    tft.setTextColor(TFT_BLACK, TFT_WHITE);
    tft.setTextSize(2);
    tft.setTextDatum(MC_DATUM);
    tft.drawString("Estado de la planta", centerX, 20); // Usa centerX en lugar de SCREEN_WIDTH/2

    // Imprimir en consola
    Serial.print("Temperatura: ");
    Serial.print(temperature, 1);
    Serial.println(" °C");

    Serial.print("Humedad Ambiente: ");
    Serial.print(humidity, 1);
    Serial.println(" %");

    Serial.print("Humedad Suelo: ");
    Serial.print(humedadSuelo);
    Serial.println(" %");

    Serial.print("Nivel de Luz: ");
    Serial.print(porcentajeLuz);
    Serial.println(" %");
    Serial.println("---------------------");
}



void displayError(const String &message) {
    tft.fillScreen(TFT_WHITE);
    int centerX = SCREEN_WIDTH / 2;
    tft.drawCentreString("ERROR", centerX, 50, FONT_SIZE);
    tft.drawCentreString(message, centerX, 90, FONT_SIZE);
    tft.drawCentreString("Verifique conexiones", centerX, 130, FONT_SIZE);
}

void sendDataViaHC12() {
    // Crear cadena de datos para enviar (formato: T=25.5,H=60.2,S=75,L=42)
    String dataStr = "T=" + String(temperature, 1) + ",H=" + String(humidity, 1) + 
                     ",S=" + String(humedadSuelo) + ",L=" + String(porcentajeLuz);
    
    Serial2.println(dataStr); // Enviar por HC-12
    Serial.println("Datos enviados: " + dataStr);
}