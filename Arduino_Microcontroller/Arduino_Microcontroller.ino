/*
 * PhotonVHealth - Solar Panel Efficiency Monitoring System
 * Bluetooth MVP + Hardware Button for Baseline
 * Arduino UNO
 */

#include <Wire.h>
#include <Adafruit_INA219.h>
#include <SoftwareSerial.h>

SoftwareSerial BTSerial(10, 11); // RX, TX for Bluetooth
Adafruit_INA219 ina219;

// Pins
const int lightPin = A0;
const int tempPin  = A1;
const int baselineButtonPin = 2;

// Sensor and efficiency variables
float lightVal = 0;
float tempVal = 0;
float powerVal = 0;
float efficiency = 0;
float adjustedLight = 0;

// Baseline values
float baselinePower = 3000;  // initial baseline power in mW
float baselineLight = 700;   // initial baseline light value

// Previous readings for shade detection
float previousLight = 0;
float previousPower = 0;

// Timing for alerts
unsigned long lastOverheatAlert = 0;
unsigned long lastDustAlert = 0;
unsigned long lastShadeAlert = 0;
unsigned long alertCooldown = 60000; // 1 minute

// Button state
int lastButtonState = LOW;
int buttonState;

float smoothLight() {
  float sum = 0;
  for (int i = 0; i < 10; i++) {
    sum += analogRead(lightPin);
    delay(5);
  }
  return sum / 10.0;
}

void setup() {
  Serial.begin(9600);
  BTSerial.begin(9600);
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

  // Night mode
  if (lightVal < 50) return;

  float lightChange = previousLight - adjustedLight;
  float powerChange = previousPower - powerVal;

  // Overheat alert
  if (tempVal > 45 && efficiency < 90) {
    if (millis() - lastOverheatAlert > alertCooldown) {
      BTSerial.println("ALERT: Panel OVERHEATING!");
      lastOverheatAlert = millis();
    }
  }

  // Dust alert
  if (adjustedLight > baselineLight * 0.8 && efficiency < 75) {
    if (millis() - lastDustAlert > alertCooldown) {
        BTSerial.println("ALERT: Dust suspected, clean panel.");
        lastDustAlert = millis();
    }
  }

  // Shade alert
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

  float tempRaw = analogRead(tempPin);
  float voltage = tempRaw * (5.0 / 1023.0);
  tempVal = voltage * 100.0;

  float busVoltage = ina219.getBusVoltage_V();
  float current_mA = ina219.getCurrent_mA();
  powerVal = ina219.getPower_mW();

  adjustedLight = 1023 - lightVal;

  if (baselineLight > 0 && baselinePower > 0) {
    float expectedPower = baselinePower * (adjustedLight / baselineLight);
    if (expectedPower > 0.01) {
      efficiency = (powerVal / expectedPower) * 100.0;
    }
  }

  // Send data via Bluetooth
  BTSerial.print(lightVal);   BTSerial.print(",");
  BTSerial.print(tempVal);    BTSerial.print(",");
  BTSerial.print(powerVal);   BTSerial.print(",");
  BTSerial.println(efficiency);

  // Debug via USB
  Serial.print("Light: "); Serial.print(lightVal);
  Serial.print(", Temp: "); Serial.print(tempVal); Serial.print(" C");
  Serial.print(", Power: "); Serial.print(powerVal); Serial.print(" mW");
  Serial.print(", Eff: "); Serial.print(efficiency); Serial.println("%");

  checkAlerts();

  buttonState = digitalRead(baselineButtonPin);
  if (buttonState == LOW && lastButtonState == HIGH) {
    updateBaseline();
  }
  lastButtonState = buttonState;

  delay(2000);
}