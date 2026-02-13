/*
 * PhotonVHealth - Solar Panel Efficiency Monitoring System's NodeMCU Microcontroller
 * Copyright (C) 2026 Saif Kayyali
 * GNU GPLv3
 */

 // Yet to get components so code is improvised for now

#include <ESP8266WiFi.h>
#include <BlynkSimpleEsp8266.h>
#include <secrets.h>

float lightVal = 0, tempVal = 0, powerVal = 0;

float baselinePower = 3.0;
float baselineLight = 700;   // default starting assumption

float efficiency;

float previousLight = 0;
float previousPower = 0;

unsigned long lastAlertTime = 0;
unsigned long alertCooldown = 60000; // 1 minute

void setup() {
  Serial.begin(9600);   // Communication with Arduino
  Blynk.begin(BLYNK_TOKEN, WIFI_SSID, WIFI_PASS); // Make your own secrets.h file with these in it and keep it in .gitignore
}

void checkAlerts() {
  if (lightVal < 100) {
    return;  // Night mode
  }

  if (tempVal > 45 && efficiency < 90) {
    if (millis() - lastAlertTime > alertCooldown) {
      Blynk.logEvent("overheat", "Panel overheating.");
      lastAlertTime = millis();
    }
  }

  else if (lightVal > baselineLight * 0.8 && efficiency < 75) {
    if (millis() - lastAlertTime > alertCooldown) {
      Blynk.logEvent("dust", "Dust accumulation suspected, make sure panel is clean.");
      lastAlertTime = millis();
    }
  }

  else if ((previousLight - lightVal) > 200 && (previousPower - powerVal) > 0.5) {
    if (millis() - lastAlertTime > alertCooldown) {
      Blynk.logEvent("shade", "Sudden shading detected, ignore if false alert.");
      lastAlertTime = millis();
    }
  }

  previousLight = lightVal;
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

      if (baselineLight > 0) {
        float expectedPower = baselinePower * (lightVal / baselineLight);
        efficiency = (powerVal / expectedPower) * 100.0;
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
    baselineLight = lightVal;
  }
}