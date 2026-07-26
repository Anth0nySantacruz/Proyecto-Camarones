#include <Arduino.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ILI9341.h>
#include <DallasTemperature.h>
#include <OneWire.h>
#include <SPI.h>
#include <math.h>
#include "apwifieeprommode.h"
#include <EEPROM.h>
#include "webmonitor.h"

// ===================== PINES =====================
constexpr uint8_t PIN_TFT_CS = 5;
constexpr uint8_t PIN_TFT_DC = 16;
constexpr uint8_t PIN_TFT_RST = 17;
constexpr uint8_t PIN_TFT_LED = 32;

constexpr uint8_t PIN_DS18B20 = 4;
constexpr uint8_t PIN_OXYGEN_DISSOLVED = 15;

constexpr uint8_t PIN_PELTIER = 27;
constexpr uint8_t PIN_AIR_PUMP = 26;

constexpr unsigned long READ_INTERVAL_MS = 1000;

// ===================== OBJETOS =====================
Adafruit_ILI9341 tft(PIN_TFT_CS, PIN_TFT_DC, PIN_TFT_RST);
OneWire oneWire(PIN_DS18B20);
DallasTemperature tempSensor(&oneWire);

// ===================== WEB SERVER  =====================
extern WebServer server;

// ===================== VARIABLES COMPARTIDAS =====================
float temperatura = 0;
float oxigeno = 0;

bool bombaEstado = false;
bool peltierEstado = false;

bool peltierState = false;
bool pumpState = false;

unsigned long lastRead = 0;

// ===================== PANTALLA =====================
void drawStaticScreen()
{
  tft.fillScreen(ILI9341_BLACK);

  tft.setTextColor(ILI9341_CYAN);
  tft.setTextSize(2);
  tft.setCursor(18, 18);
  tft.print("PROTOTIPO BALDE");

  tft.drawFastHLine(18, 48, 284, ILI9341_DARKGREY);

  // Título temperatura
  tft.setTextColor(ILI9341_WHITE);
  tft.setCursor(18, 70);
  tft.print("Temperatura");

  // Título oxígeno al lado
  tft.setCursor(180, 70);
  tft.print("O2");

  // Footer
  tft.setTextSize(1);
  tft.setTextColor(ILI9341_LIGHTGREY);
  tft.setCursor(18, 225);
  tft.print("Estado del sistema");
}

// ===================== TEMPERATURA =====================
void drawTemperature(float temperatureC)
{

  tft.fillRect(18, 95, 140, 60, ILI9341_BLACK);

  tft.setCursor(18, 95);
  tft.setTextSize(4);

  if (isnan(temperatureC))
  {
    tft.setTextColor(ILI9341_RED);
    tft.print("ERR");
    return;
  }

  tft.setTextColor(ILI9341_YELLOW);
  tft.print(temperatureC, 1);
}

// ===================== OXÍGENO =====================
void drawOxygen(float oxygenMgL)
{

  tft.fillRect(180, 95, 120, 60, ILI9341_BLACK);

  tft.setCursor(180, 95);
  tft.setTextSize(4);

  tft.setTextColor(ILI9341_GREEN);
  tft.print(oxygenMgL, 1);
}

// ===================== ESTADOS =====================
void drawActuatorStatus(bool peltierOn, bool pumpOn)
{

  tft.fillRect(18, 190, 286, 30, ILI9341_BLACK);

  tft.setTextSize(2);

  // Celda
  tft.setCursor(18, 190);
  if (peltierOn)
  {
    tft.setTextColor(ILI9341_GREEN);
    tft.print("Celda: ON");
  }
  else
  {
    tft.setTextColor(ILI9341_RED);
    tft.print("Celda: OFF");
  }

  // Bomba
  tft.setCursor(170, 190);
  if (pumpOn)
  {
    tft.setTextColor(ILI9341_GREEN);
    tft.print("Bomba: ON");
  }
  else
  {
    tft.setTextColor(ILI9341_RED);
    tft.print("Bomba: OFF");
  }
}

// ===================== SENSORES =====================
float readTemperatureC()
{
  tempSensor.requestTemperatures();
  float value = tempSensor.getTempCByIndex(0);
  if (value == DEVICE_DISCONNECTED_C)
    return NAN;
  return value;
}

float readOxygenMgL()
{
  int raw = analogRead(PIN_OXYGEN_DISSOLVED);
  return (raw * 12.0f) / 4095.0f;
}

// ===================== SETUP =====================
void setup()
{

  Serial.begin(115200);
  delay(500);

  EEPROM.begin(512);

  pinMode(PIN_PELTIER, OUTPUT);
  pinMode(PIN_AIR_PUMP, OUTPUT);

  digitalWrite(PIN_PELTIER, LOW);
  digitalWrite(PIN_AIR_PUMP, LOW);

  pinMode(PIN_TFT_LED, OUTPUT);
  digitalWrite(PIN_TFT_LED, HIGH);

  SPI.begin(18, 19, 23);

  tft.begin();
  tft.setRotation(1);

  tempSensor.begin();

  drawStaticScreen();

  // ===================== WIFI + AP =====================
  intentoconexion("Proyecto Camarones", "123456789");

  // ===================== WEB =====================
  iniciarWebMonitor();

  server.begin();

  //  drawTemperature(NAN);
  //  drawOxygen(0);
  //  drawActuatorStatus(false, false);

  Serial.println("Sistema iniciado");
}

// ===================== LOOP =====================
void loop() {

  // AP + web server
  loopAP();
  server.handleClient();

  if (millis() - lastRead >= READ_INTERVAL_MS) {

    lastRead = millis();

    // ---------- TEMPERATURA ----------
    float t = readTemperatureC();
    temperatura = t;

    drawTemperature(t);

    if (!isnan(t)) {

      if (t > 24.0f) {
        digitalWrite(PIN_PELTIER, HIGH);
        peltierState = true;
      }
      else if (t < 22.0f) {
        digitalWrite(PIN_PELTIER, LOW);
        peltierState = false;
      }
    }

    // ---------- OXÍGENO ----------
    float o2 = readOxygenMgL();
    oxigeno = o2;

    drawOxygen(o2);

    if (o2 < 5.0f) {
      digitalWrite(PIN_AIR_PUMP, HIGH);
      pumpState = true;
    }
    else if (o2 >= 7.0f) {
      digitalWrite(PIN_AIR_PUMP, LOW);
      pumpState = false;
    }

    // ---------- ESTADOS ----------
    bombaEstado = pumpState;
    peltierEstado = peltierState;

    drawActuatorStatus(peltierState, pumpState);

    Serial.printf("T: %.2f C | O2: %.2f mg/L\n", t, o2);
  }
}