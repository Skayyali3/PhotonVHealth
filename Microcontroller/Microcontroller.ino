/*
 * PhotonVHealth - Solar Panel Efficiency Monitoring System
 * Bluetooth MVP (Future insha'Allah is using an ESP32)
 * Arduino UNO Microcontroller
*/

#include <Wire.h>
#include <Adafruit_INA219.h>
#include "BluetoothSerial.h"
BluetoothSerial SerialBT;
Adafruit_INA219 ina219;

const int lightPin = 34;
const int tempPin  = 35;
const int baselinebuttonpin = 2;

float lightval = 0;
float tempval = 0;
float powerval = 0;
float efficiency = 0;
float adjustedlight = 0;

float baselinepower = 3000;
float baselinelight = 700;
float maxhardwarepower = 0;

float previouslight = 0;
float filteredtemp = 0;
float previouspower = 0;

unsigned long lastoverheatalert = 0;
unsigned long lastdustalert = 0;
unsigned long lastshadealert = 0;
unsigned long alertcooldown = 60000;
unsigned long lastdisplaytime = 0;
const unsigned long displayinterval = 2000;

int lastbuttonstate = LOW;
int buttonstate;

float smoothlight() {
  float sum = 0;
  for (int i = 0; i < 20; i++) {
    sum += analogRead(lightPin);
    delay(5);
  }
  return sum / 20.0;
}

float smoothtemp() {
    float sum = 0;
    for (int i = 0; i < 20; i++) {
        sum += analogRead(tempPin);
        delay(5);
    }
    return (sum / 20.0) * (3.3 / 4095.0) * 100.0;
}

void setup() {
  Serial.begin(115200);
  SerialBT.begin("PhotonVHealth");
  SerialBT.println("PhotonVHealth is Online via Bluetooth");
  ina219.begin();
  pinMode(baselinebuttonpin, INPUT_PULLUP);
  filteredtemp = smoothtemp();
}

void updatebaseline(float p, float l) {
  baselinepower = p;
  baselinelight = l;
  SerialBT.println("BASELINE UPDATED");
  SerialBT.print(baselinepower); SerialBT.print(" mW, ");
  SerialBT.print(baselinelight); SerialBT.println(" a.u.");
  Serial.println("Baseline updated!");
}

void handlebluetoothinput() {
  if (SerialBT.available() > 0) {
    float input = SerialBT.parseFloat();
    if (input > 0) {
      maxhardwarepower = input;
      SerialBT.print("Max Rating Set To: "); SerialBT.print(maxhardwarepower); SerialBT.println("mW");
    }
  }
}

void checkalerts() {
  if (previouslight == 0 && previouspower == 0) {
    previouslight = adjustedlight;
    previouspower = powerval;
    return;
  }

  float current_mA = ina219.getCurrent_mA();

  if (adjustedlight < 150) return;
  if (current_mA < 20) return; // Prevents alerts when battery (or whatever load used is) is full

  float lightchange = previouslight - adjustedlight;
  float powerchange = previouspower - powerval;

  if (tempval > 45 && efficiency < 90) {
    if (millis() - lastoverheatalert > alertcooldown) {
      SerialBT.println("ALERT: Panel OVERHEATING!");
      lastoverheatalert = millis();
    }
  }

  if (adjustedlight > baselinelight * 0.8 && efficiency < 75) {
    if (millis() - lastdustalert > alertcooldown) {
        SerialBT.println("ALERT: Dust suspected, clean the panel.");
        lastdustalert = millis();
    }
  }

  if (lightchange > 200 && powerchange > (previouspower * 0.2)) {
    if (millis() - lastshadealert > alertcooldown) {
      SerialBT.println("ALERT: Sudden SHADE detected.");
      lastshadealert = millis();
    }
  }

  previouslight = adjustedlight;
  previouspower = powerval;
}

void loop() {
  handlebluetoothinput();

  buttonstate = digitalRead(baselinebuttonpin);
  if (buttonstate == LOW && lastbuttonstate == HIGH) {
    updatebaseline(INA219.getPower_mW(), 1023 - smoothlight());
  }

  lastbuttonstate = buttonstate;
  if (millis() - lastdisplaytime >= displayinterval) {
    lastdisplaytime = millis();

    lightval = smoothlight();
    adjustedlight = 1023 - lightval;
    powerval = ina219.getPower_mW();

    tempval = smoothtemp();
    filteredtemp = filteredtemp * 0.9 + tempval * 0.1;
    tempval = filteredtemp;

    if (baselinelight > 0 && baselinepower > 0) {
      float lightratio = adjustedlight / baselinelight;
      // Make it realistic
  
      if (lightratio > 1.2) lightratio = 1.2;
      if (lightratio < 0.1) lightratio = 0.1;

      float baselineExpected = baselinepower * lightratio;
      float maxExpected = maxhardwarepower * lightratio;
      float expectedpower;
      if (maxhardwarepower > 0) {
        expectedpower = (0.7 * baselineExpected) + (0.3 * maxExpected);
      } else {
        expectedpower = baselineExpected;
      }
      // good light, low temp & actually higher power than baseline needed to auto-renew baseline
      if (powerval > baselinepower * 1.1 && adjustedlight > 600 && tempval < 35) {
          updatebaseline(powerval, adjustedlight);
          expectedpower = powerval; 
      }
      
      if (expectedpower > 0.01) {
        efficiency = (powerval / expectedpower) * 100.0;
      }
    }

    float lightpercent = (adjustedlight / 4095.0) * 100.0;
    float health = 0;

    if (maxhardwarepower > 0) {
      health = (baselinepower / maxhardwarepower) * 100.0;
    }
    if (health > 100) {
      health = 100;
    }

    Serial.print("Health: ");
    Serial.print(health);
    Serial.println("%");
    Serial.print("Light Intensity: "); Serial.print(lightpercent); Serial.println("%");
    Serial.print("Amount of Light: "); Serial.print(adjustedlight); Serial.println(" a.u.");
    Serial.print("Temp: "); Serial.print(tempval); Serial.println("°C");
    Serial.print("Power: "); Serial.print(powerval); Serial.println(" mW");
    Serial.print("Eff: "); Serial.print(efficiency); Serial.println("%");

    checkalerts();
  }
}