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

const int lightpin = A0;
const int temppin  = A1;
const int baselinebuttonpin = 2;

float lightval = 0;
float tempval = 0;
float powerval = 0;
float efficiency = 0;
float adjustedlight = 0;

float baselinepower = 3000;
float baselinelight = 700;

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
  analogReference(DEFAULT);
  analogRead(lightpin); // Dummy read to settle the ADC
  delay(50);
  for (int i = 0; i < 20; i++) {
    sum += analogRead(lightpin);
    delay(5);
  }
  return sum / 20.0;
}

float smoothtemp() {
    float sum = 0;
    analogReference(INTERNAL);
    analogRead(temppin); // Dummy read to settle the ADC
    delay(50);
    for (int i = 0; i < 20; i++) {
        sum += analogRead(temppin);
        delay(5);
    }
    return (sum / 20.0) * (1.1 / 1023.0) * 100.0;
}

void setup() {
  Serial.begin(9600);
  BTSerial.begin(9600);
  BTSerial.println("PhotonVHealth is Online via Bluetooth");
  ina219.begin();
  pinMode(baselinebuttonpin, INPUT_PULLUP);
  filteredtemp = smoothtemp();
}

void updatebaseline() {
  baselinepower = powerval;
  baselinelight = 1023 - lightval;
  BTSerial.println("BASELINE UPDATED");
  Serial.println("Baseline updated!");
}

void checkalerts() {
  if (previouslight == 0 && previouspower == 0) {
    previouslight = adjustedlight;
    previouspower = powerval;
    return;
  }

  float current_mA = ina219.getCurrent_mA();

  if (adjustedlight < 150) return;
  if (current_mA < 10) return; // Prevents alerts when battery (or whatever load used is) is full

  float lightchange = previouslight - adjustedlight;
  float powerchange = previouspower - powerval;

  if (tempval > 45 && efficiency < 90) {
    if (millis() - lastoverheatalert > alertcooldown) {
      BTSerial.println("ALERT: Panel OVERHEATING!");
      lastoverheatalert = millis();
    }
  }

  if (adjustedlight > baselinelight * 0.8 && efficiency < 75) {
    if (millis() - lastdustalert > alertcooldown) {
        BTSerial.println("ALERT: Dust suspected, clean the panel.");
        lastdustalert = millis();
    }
  }

  if (lightchange > 200 && powerchange > (previouspower * 0.2)) {
    if (millis() - lastshadealert > alertcooldown) {
      BTSerial.println("ALERT: Sudden SHADE detected.");
      lastshadealert = millis();
    }
  }

  previouslight = adjustedlight;
  previouspower = powerval;
}

void loop() {
  buttonstate = digitalRead(baselinebuttonpin);
  if (buttonstate == LOW && lastbuttonstate == HIGH) {
    updatebaseline();
  }

  lastbuttonstate = buttonstate;
  if (millis() - lastdisplaytime >= displayinterval) {
    lastdisplaytime = millis();

    lightval = smoothlight();
    adjustedlight = 1023 - lightval;
    powerval = ina219.getPower_mW();

    if (baselinelight > 0 && baselinepower > 0) {
      float expectedpower = baselinepower * (adjustedlight / baselinelight);
      if (expectedpower > 0.01) {
        efficiency = (powerval / expectedpower) * 100.0;
      }
    }

    tempval = smoothtemp();
    filteredtemp = filteredtemp * 0.9 + tempval * 0.1;
    tempval = filteredtemp;

    float lightpercent = (adjustedlight / 1023.0) * 100.0;

    Serial.print("Light Intensity: "); Serial.print(lightpercent); Serial.println("%");
    Serial.print("Amount of Light: "); Serial.print(lightval); Serial.println(" a.u.");
    Serial.print("Temp: "); Serial.print(tempval); Serial.println("°C");
    Serial.print("Power: "); Serial.print(powerval); Serial.println(" mW");
    Serial.print("Eff: "); Serial.print(efficiency); Serial.println("%");

    checkalerts();
  }
}