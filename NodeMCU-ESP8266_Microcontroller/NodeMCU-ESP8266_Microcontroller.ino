/*
 * PhotonVHealth - Solar Panel Efficiency Monitoring System's NodeMCU Microcontroller
 * Copyright (C) 2026 Saif Kayyali
 * GNU GPLv3
 */

#include <ESP8266WiFi.h>
#include <BlynkSimpleEsp8266.h>
#include <secrets.h>

float lightVal = 0, tempVal = 0, powerVal = 0;

float baselinePower = 3000;
float baselineLight = 700;

float efficiency;
float adjustedLight;

float previousLight = 0;
float previousPower = 0;

unsigned long lastAlertTime = 0;
unsigned long alertCooldown = 60000; // 1 minute

void setup() {
  Serial.begin(9600);   // Communication with Arduino
  Serial.setTimeout(50);
  Blynk.begin(WIFI_SSID, WIFI_PASS, BLYNK_TOKEN); // Make your own secrets.h file with these in it and keep it in .gitignore
}

void checkAlerts() {
  if (previousLight == 0 && previousPower == 0) {
    previousLight = lightVal;
    previousPower = powerVal;
    return;
  }

  if (lightVal > 900) { 
    return;  // Night mode
  }

  float lightChange = previousLight - lightVal;
  float powerChange = previousPower - powerVal;

  if (tempVal > 45 && efficiency < 90) {
    if (millis() - lastAlertTime > alertCooldown) {
      Blynk.logEvent("overheat", "Panel overheating.");
      lastAlertTime = millis();
    }
  }

  else if (adjustedLight > baselineLight * 0.8 && efficiency < 75) {
    if (millis() - lastAlertTime > alertCooldown) {
      Blynk.logEvent("dust", "Dust accumulation suspected, make sure panel is clean.");
      lastAlertTime = millis();
    }
  }

  else if (lightChange > 200 && powerChange > (previousPower * 0.2)) {
    if (millis() - lastAlertTime > alertCooldown) {
      Blynk.logEvent("shade", "Sudden shading detected, ignore if false alert.");
      lastAlertTime = millis();
    }
  }

  previousLight = adjustedLight;
  previousPower = powerVal;
}

void loop() {
  Blynk.run();

  if (Serial.available()) {
    String data = Serial.readStringUntil('\n');

    int firstComma = data.indexOf(',');
    int secondComma = data.indexOf(',', firstComma + 1);

    if (firstComma > 0 && secondComma > 0) {
      lightVal = data.substring(0, firstComma).toFloat();
      tempVal = data.substring(firstComma + 1, secondComma).toFloat();
      powerVal = data.substring(secondComma + 1).toFloat();

      float adjustedLight = 1023 - lightVal;

      if (baselineLight > 0 && baselinePower > 0) {
        float expectedPower = baselinePower * (adjustedLight / baselineLight);

        if (expectedPower > 0.01) {
          efficiency = (powerVal / expectedPower) * 100.0;
        }
      }

      Blynk.virtualWrite(V0, lightVal);
      Blynk.virtualWrite(V1, tempVal);
      Blynk.virtualWrite(V2, powerVal);
      Blynk.virtualWrite(V3, efficiency);

      checkAlerts();
    }
  }
}

// Baseline Update via button press

BLYNK_WRITE(V4) {
  int buttonState = param.asInt();
  if (buttonState == 1) {
    baselinePower = powerVal;
    baselineLight = 1023 - lightVal;
  }
}