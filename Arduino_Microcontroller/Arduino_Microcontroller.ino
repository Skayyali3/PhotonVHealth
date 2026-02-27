/*
 * PhotonVHealth - Solar Panel Efficiency Monitoring System
 * Bluetooth MVP (Future insha'Allah is using an ESP32)
 * Arduino UNO Microcontroller
*/

#include <Wire.h>
#include <Adafruit_INA219.h>
#include <SoftwareSerial.h>

SoftwareSerial BTSerial(10, 11);
Adafruit_INA219 ina219;

const int lightPin = A0;
const int tempPin  = A1;
const int baselineButtonPin = 2;

float lightVal = 0;
float tempVal = 0;
float powerVal = 0;
float efficiency = 0;
float adjustedLight = 0;

float baselinePower = 3000;
float baselineLight = 700;

float previousLight = 0;
float filteredTemp = 0;
float previousPower = 0;

unsigned long lastOverheatAlert = 0;
unsigned long lastDustAlert = 0;
unsigned long lastShadeAlert = 0;
unsigned long alertCooldown = 60000;

int lastButtonState = LOW;
int buttonState;

float smoothLight() {
  float sum = 0;
  for (int i = 0; i < 20; i++) {
    sum += analogRead(lightPin);
    delay(5);
  }
  return sum / 20.0;
}

float smoothTemp() {
    float sum = 0;
    for (int i = 0; i < 20; i++) {
        sum += analogRead(tempPin);
        delay(5);
    }
    return (sum / 20.0);
}

void setup() {
  Serial.begin(9600);
  BTSerial.begin(9600);
  BTSerial.println("PhotonVHealth Online");
  ina219.begin();
  pinMode(baselineButtonPin, INPUT_PULLUP);
}

void updateBaseline() {
  baselinePower = powerVal;
  baselineLight = 1023 - lightVal;
  BTSerial.println("BASELINE UPDATED");
  Serial.println("Baseline updated!");
}

void checkAlerts() {
  if (previousLight == 0 && previousPower == 0) {
    previousLight = adjustedLight;
    previousPower = powerVal;
    return;
  }

  float current_mA = ina219.getCurrent_mA();

  if (adjustedLight < 150) return;
  if (current_mA < 10) return; // Prevents alerts when battery (or whatever load used is) is full

  float lightChange = previousLight - adjustedLight;
  float powerChange = previousPower - powerVal;

  if (tempVal > 45 && efficiency < 90) {
    if (millis() - lastOverheatAlert > alertCooldown) {
      BTSerial.println("ALERT: Panel OVERHEATING!");
      lastOverheatAlert = millis();
    }
  }

  if (adjustedLight > baselineLight * 0.8 && efficiency < 75) {
    if (millis() - lastDustAlert > alertCooldown) {
        BTSerial.println("ALERT: Dust suspected, clean panel.");
        lastDustAlert = millis();
    }
  }

  if (lightChange > 200 && powerChange > (previousPower * 0.2)) {
    if (millis() - lastShadeAlert > alertCooldown) {
      BTSerial.println("ALERT: Sudden SHADE detected.");
      lastShadeAlert = millis();
    }
  }

  previousLight = adjustedLight;
  previousPower = powerVal;
}

void loop() {
  lightVal = smoothLight();
  adjustedLight = 1023 - lightVal;
  powerVal = ina219.getPower_mW();

  if (baselineLight > 0 && baselinePower > 0) {
    float expectedPower = baselinePower * (adjustedLight / baselineLight);
    if (expectedPower > 0.01) {
      efficiency = (powerVal / expectedPower) * 100.0;
    }
  }

  float tempRaw = smoothTemp();
  float voltage = tempRaw * (5 / 1023.0);
  tempVal = voltage * 100.0;
  filteredTemp = filteredTemp * 0.9 + tempVal * 0.1;
  tempVal = filteredTemp;

  // Debug via USB
  Serial.print("Amount of Light: "); Serial.println(lightVal);
  Serial.print("Temp: "); Serial.print(tempVal); Serial.println(" °C");
  Serial.print("Power: "); Serial.print(powerVal); Serial.println(" mW");
  Serial.print("Eff: "); Serial.print(efficiency); Serial.println("%");

  checkAlerts();

  buttonState = digitalRead(baselineButtonPin);
  if (buttonState == LOW && lastButtonState == HIGH) {
    updateBaseline();
  }
  lastButtonState = buttonState;

  delay(2000);
}