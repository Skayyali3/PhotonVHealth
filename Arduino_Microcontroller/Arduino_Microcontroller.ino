/*
 * PhotonVHealth - Solar Panel Efficiency Monitoring System's Arduino Microcontroller
 * Copyright (C) 2026 Saif Kayyali
 * GNU GPLv3
 */

#include <Wire.h>
#include <Adafruit_INA219.h>

Adafruit_INA219 ina219;

float smoothLight() {
  float sum = 0;
  for (int i = 0; i < 10; i++) {
    sum += analogRead(A0);
    delay(5);
  }
  return sum / 10.0;
}

void setup() {
  Serial.begin(9600);
  ina219.begin();
}

void loop() {
  float light = smoothLight();

  float tempRaw = analogRead(A1);
  float voltage = tempRaw * (5.0 / 1023.0);
  float temperatureC = voltage * 100.0;

  float busVoltage = ina219.getBusVoltage_V();
  float current_mA = ina219.getCurrent_mA();
  float power_mW = ina219.getPower_mW();


  Serial.print(light);
  Serial.print(",");
  Serial.print(temperatureC);
  Serial.print(",");
  Serial.println(power_mW);

  delay(2000);
}