/*
 * PhotonVHealth - Solar Panel Efficiency Monitoring System
 * ESP32 Microcontroller Prototyping
*/

#include <Wire.h>
#include <Adafruit_INA219.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <WiFiManager.h>
#include <ArduinoJson.h>

Adafruit_INA219 ina219;

const int lightPin = 34;
const int tempPin = 35;

float lightVal = 0;
float tempVal = 0;
float powerVal = 0;
float voltageVal = 0;
float efficiency = 0;
float adjustedLight = 0;
float percentageLight = 0;
float health = 0;

float baselinePower = 0;
float baselineLight = 0;

float previousLight = 0;
float previousPower = 0;
float filteredTemp = 0;

unsigned long lastOverheatAlert = 0;
unsigned long lastDustAlert = 0;
unsigned long lastShadeAlert = 0;
const unsigned long alertCooldown = 60000;

unsigned long lastDataSend = 0;
unsigned long lastCommandCheck = 0;
const unsigned long dataInterval = 2000;
const unsigned long commandInterval = 30000;

String deviceId = "";

float smooth_light() {
  float sum = 0;
  for (int i = 0; i < 50; i++) {
    sum += analogRead(lightPin);
  }
  return sum / 50.0;
}

float smooth_temp() {
  long sum = 0;
  const int samples = 50;

  analogReadMilliVolts(tempPin);  // dummy read to settle ADC
  delay(10);

  for (int i = 0; i < samples; i++) {
    sum += analogReadMilliVolts(tempPin);
    delay(2);
  }

  float average = (float)sum / samples;
  float currentTemp = average / 10.0;

  if (currentTemp <= 0.0 || currentTemp >= 110.0) {
    Serial.println("LM35 read error, defaulting to previous value.");
    return filteredTemp;
  }
  return currentTemp;
}

void connect_to_wifi() {
  WiFiManager wm;
  if (!wm.autoConnect("PhotonVHealth-Setup")) {
    ESP.restart();
  }
}

void data_to_server() {
  if (WiFi.status() != WL_CONNECTED) return;

  HTTPClient http;
  http.begin("https://photonvhealth.onrender.com/api/data");
  http.addHeader("Content-Type", "application/json");

  String json = "{";
  json += "\"device_id\":\"" + deviceId + "\",";
  json += "\"power\":" + String(powerVal) + ",";
  json += "\"light\":" + String(adjustedLight) + ",";
  json += "\"percentage\":" + String(percentageLight) + ",";
  json += "\"temp\":" + String(tempVal) + ",";
  json += "\"efficiency\":" + String(efficiency);
  json += "}";

  int statusCode = http.POST(json);

  if (statusCode == 200) {
    String body = http.getString();
    StaticJsonDocument<128> doc;
    if (deserializeJson(doc, body) == DeserializationError::Ok) {
      health = doc["health"] | 0.0f;
    }
  } else {
    Serial.print("POST /api/data failed: ");
    Serial.println(statusCode);
  }

  http.end();
}

void check_commands() {
  if (WiFi.status() != WL_CONNECTED) return;

  HTTPClient http;
  http.begin("https://photonvhealth.onrender.com/api/commands/" + deviceId);

  int statusCode = http.GET();

  if (statusCode == 200) {
    String body = http.getString();
    StaticJsonDocument<128> doc;
    if (deserializeJson(doc, body) == DeserializationError::Ok) {
      float newBasePower = doc["baseline_power"] | 0.0f;
      float newBaseLight = doc["baseline_light"] | 0.0f;

      if (newBasePower > 0 && newBaseLight > 0) {
        baselinePower = newBasePower;
        baselineLight = newBaseLight;
        Serial.println("Baseline synced from server.");
        Serial.print("Power Baseline Value: ");
        Serial.println(baselinePower);
        Serial.print("Light Baseline Value: ");
        Serial.println(baselineLight);
      }
    }
  } else {
    Serial.print("GET /api/commands failed: ");
    Serial.println(statusCode);
  }

  http.end();
}

void check_alerts() {
  if (previousLight == 0 && previousPower == 0) {
    previousLight = adjustedLight;
    previousPower = powerVal;
    return;
  }

  float currentMA = ina219.getCurrent_mA();

  if (adjustedLight < 150) return;
  if (currentMA < 20) return;

  float lightChange = previousLight - adjustedLight;
  float powerChange = previousPower - powerVal;

  if (tempVal > 45 && efficiency < 90) {
    if (millis() - lastOverheatAlert > alertCooldown) {
      lastOverheatAlert = millis();
      Serial.println("[ALERT] Overheat detected.");
    }
  }

  if (adjustedLight > baselineLight * 0.8 && efficiency < 75) {
    if (millis() - lastDustAlert > alertCooldown) {
      lastDustAlert = millis();
      Serial.println("[ALERT] Possible dust/soiling detected.");
    }
  }

  if (lightChange > 200 && powerChange > (previousPower * 0.2)) {
    if (millis() - lastShadeAlert > alertCooldown) {
      lastShadeAlert = millis();
      Serial.println("[ALERT] Sudden shading detected.");
    }
  }

  previousLight = adjustedLight;
  previousPower = powerVal;
}

void setup() {
  Serial.begin(115200);
  Serial.println("PhotonVHealth Setup: Initialized...");

  uint64_t chipid = ESP.getEfuseMac();
  char idBuffer[20];
  sprintf(idBuffer, "PVH_%04X%08X", (uint16_t)(chipid >> 32), (uint32_t)chipid);
  deviceId = String(idBuffer);

  Serial.print("Device ID: ");
  Serial.println(deviceId);

  ina219.begin();
  analogReadResolution(12);
  connect_to_wifi();

  filteredTemp = smooth_temp();

  check_commands();
}

void loop() {
  unsigned long now = millis();
  if (now - lastCommandCheck >= commandInterval) {
    lastCommandCheck = now;
    check_commands();
  }

  if (now - lastDataSend >= dataInterval) {
    lastDataSend = now;

    lightVal = smooth_light();
    adjustedLight = 4095 - lightVal;
    powerVal = ina219.getPower_mW();
    voltageVal = ina219.getBusVoltage_V();

    tempVal = smooth_temp();
    filteredTemp = filteredTemp * 0.9 + tempVal * 0.1;
    tempVal = filteredTemp;

    if (baselineLight > 0 && baselinePower > 0) {
      float lightRatio = adjustedLight / baselineLight;
      if (lightRatio > 1.2) lightRatio = 1.2;
      if (lightRatio < 0.1) lightRatio = 0.1;

      float expectedPower = baselinePower * lightRatio;

      if (expectedPower > 0.01) {
        efficiency = (powerVal / expectedPower) * 100.0;
      }
    }

    percentageLight = (adjustedLight / 4095.0) * 100.0;

    data_to_server();

    Serial.println("──────────────────────");
    Serial.print("Light Intensity: ");
    Serial.print(percentageLight);
    Serial.println(" %");
    Serial.print("Light:");
    Serial.print(adjustedLight);
    Serial.println(" a.u.");
    Serial.print("Temp: ");
    Serial.print(tempVal);
    Serial.println(" °C");
    Serial.print("Power: ");
    Serial.print(powerVal);
    Serial.println(" mW");
    Serial.print("Voltage: ");
    Serial.print(voltageVal);
    Serial.println(" V");
    Serial.print("Efficiency: ");
    Serial.print(efficiency);
    Serial.println(" %");
    Serial.print("Health: ");
    Serial.print(health);
    Serial.println(" %");

    check_alerts();
  }
}