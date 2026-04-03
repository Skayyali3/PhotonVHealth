/*
 * PhotonVHealth - Solar Panel Efficiency Monitoring System
 * ESP32 Microcontroller Prototyping
*/

#include <Wire.h>
#include <Adafruit_INA219.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include "secrets.h" // make your own secrets.h file & put in your wifi ssid and pass, make sure not to commit it

#define ONE_WIRE_BUS 4
Adafruit_INA219 ina219;
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensor(&oneWire);

const int lightPin = 34;
const int tempPin  = 35;
const int baselinebuttonpin = 2;

float health = 0;
float lightVal = 0;
float tempVal = 0;
float powerVal = 0;
float efficiency = 0;
float adjustedLight = 0;
float percentageLight = 0;

float baselinePower = 3000;
float baselineLight = 700;
float hardwarePowerMax = 0;

float previousLight = 0;
float filteredTemp = 0;
float previousPower = 0;

unsigned long lastOverheatAlert = 0;
unsigned long lastDustAlert = 0;
unsigned long lastShadeAlert = 0;
unsigned long alertCooldown = 60000;
unsigned long lastDisplayTime = 0;
const unsigned long displayInterval = 2000;

int lastbuttonstate = LOW;
int buttonstate;

float smooth_light() {
  float sum = 0;
  for (int i = 0; i < 20; i++) {
    sum += analogRead(lightPin);
    delay(5);
  }
  return sum / 20.0;
}

float read_temperature() {
  sensor.requestTemperatures();
  float tempC = sensor.getTempCByIndex(0);
  return tempC;
}

void connect_to_wifi() {
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  Serial.print("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("\nConnected!");
  Serial.println(WiFi.localIP());
}

void data_to_server() {
  if (WiFi.status() == WL_CONNECTED) {

    HTTPClient http;
    http.begin("https://my-app.onrender.com/api/data"); // will change to photonvhealth.onrender.com insha'Allah soon

    http.addHeader("Content-Type", "application/json");

    String json = "{";
    json += "\"power\":" + String(powerVal) + ",";
    json += "\"light\":" + String(adjustedLight) + ",";
    json += "\"perecentage\":" + String(percentageLight) + ",";
    json += "\"temp\":" + String(tempVal) + ",";
    json += "\"efficiency\":" + String(efficiency) + ",";
    json += "\"health\":" + String(health);
    json += "}";

    int response = http.POST(json);

    http.end();
  }
}

void setup() {
  Serial.begin(115200);
  ina219.begin();
  sensor.begin();
  connect_to_wifi();
  pinMode(baselinebuttonpin, INPUT_PULLUP);
  filteredTemp = read_temperature();
}

void update_baseline(float p, float l) {
  baselinePower = p;
  baselineLight = l;
  Serial.println("Baseline updated!");
}

void check_alerts() {
  if (previousLight == 0 && previousPower == 0) {
    previousLight = adjustedLight;
    previousPower = powerVal;
    return;
  }

  float current_mA = ina219.getCurrent_mA();

  if (adjustedLight < 150) return;
  if (current_mA < 20) return; // Prevents alerts when battery (or whatever load used is) is full

  float lightchange = previousLight - adjustedLight;
  float powerchange = previousPower - powerVal;

  if (tempVal > 45 && efficiency < 90) {
    if (millis() - lastOverheatAlert > alertCooldown) {
      lastOverheatAlert = millis();
    }
  }

  if (adjustedLight > baselineLight * 0.8 && efficiency < 75) {
    if (millis() - lastDustAlert > alertCooldown) {
        lastDustAlert = millis();
    }
  }

  if (lightchange > 200 && powerchange > (previousPower * 0.2)) {
    if (millis() - lastShadeAlert > alertCooldown) {
      lastShadeAlert = millis();
    }
  }

  previousLight = adjustedLight;
  previousPower = powerVal;
}

void loop() {
  buttonstate = digitalRead(baselinebuttonpin);
  if (buttonstate == LOW && lastbuttonstate == HIGH) {
    update_baseline(ina219.getPower_mW(), 1023 - smooth_light());
  }

  lastbuttonstate = buttonstate;
  if (millis() - lastDisplayTime >= displayInterval) {
    lastDisplayTime = millis();

    lightVal = smooth_light();
    adjustedLight = 1023 - lightVal;
    powerVal = ina219.getPower_mW();

    tempVal = read_temperature();

    filteredTemp = filteredTemp * 0.9 + tempVal * 0.1;
    tempVal = filteredTemp;

    if (baselineLight > 0 && baselinePower > 0) {
      float lightRatio = adjustedLight / baselineLight;
      // Make it realistic
  
      if (lightRatio > 1.2) lightRatio = 1.2;
      if (lightRatio < 0.1) lightRatio = 0.1;

      float baselineExpected = baselinePower * lightRatio;
      float maxExpected = hardwarePowerMax * lightRatio;
      float expectedPower;
      if (hardwarePowerMax > 0) {
        expectedPower = (0.7 * baselineExpected) + (0.3 * maxExpected);
      } else {
        expectedPower = baselineExpected;
      }
      // good light, low temp & actually higher power than baseline needed to auto-renew baseline
      if (powerVal > baselinePower * 1.1 && adjustedLight > 600 && tempVal < 35) {
          update_baseline(powerVal, adjustedLight);
          expectedPower = powerVal; 
      }
      
      if (expectedPower > 0.01) {
        efficiency = (powerVal / expectedPower) * 100.0;
      }
    }

    percentageLight = (adjustedLight / 4095.0) * 100.0;

    if (hardwarePowerMax > 0) {
      health = (baselinePower / hardwarePowerMax) * 100.0;
    }
    if (health > 100) {
      health = 100;
    }

    data_to_server();

    Serial.print("Health: "); Serial.print(health); Serial.println("%");
    Serial.print("Light Intensity: "); Serial.print(percentageLight); Serial.println("%");
    Serial.print("Amount of Light: "); Serial.print(adjustedLight); Serial.println(" a.u.");
    Serial.print("Temp: "); Serial.print(tempVal); Serial.println("°C");
    Serial.print("Power: "); Serial.print(powerVal); Serial.println(" mW");
    Serial.print("Eff: "); Serial.print(efficiency); Serial.println("%");

    check_alerts();
  }
}