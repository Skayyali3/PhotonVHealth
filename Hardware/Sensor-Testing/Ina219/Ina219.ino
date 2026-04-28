#include <Wire.h>
#include <Adafruit_INA219.h>

Adafruit_INA219 ina219;

void setup() {
    Serial.begin(115200);
    ina219.begin();
    Serial.println("INA219 Sensor Testing Initialized...");
}

void loop() {
    float currentVal = ina219.getCurrent_mA();
    float powerVal = ina219.getPower_mW();
    float voltageVal = ina219.getBusVoltage_V();
    Serial.print("Electrical Power: "); Serial.print(powerVal); Serial.println(" mW");
    Serial.print("Voltage: "); Serial.print(voltageVal); Serial.println(" V");
    Serial.print("Current Flowing Through: "); Serial.print(currentVal); Serial.println(" A");
    delay(3000);
}