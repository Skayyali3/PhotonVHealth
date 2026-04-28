#include <OneWire.h>
#include <DallasTemperature.h>

#define ONE_WIRE_BUS 9

OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);

void setup() {
  Serial.begin(9600);
  Serial.println();
  Serial.println("--- DS18B20 Temp Test Initialized ---");

  sensors.begin();

  if (sensors.getDeviceCount() == 0) {
    Serial.println("No DS18B20 sensors found! Check connections.");
  } else {
    Serial.print("Found ");
    Serial.print(sensors.getDeviceCount());
    Serial.println(" sensor(s).");
  }
}

void loop() {
  Serial.println();
  Serial.println("Requesting temperatures...");
  sensors.requestTemperatures();
  Serial.println("DONE");

  float tempC = sensors.getTempCByIndex(0);

  if(tempC == DEVICE_DISCONNECTED_C) {
    Serial.println("Error: Could not read temperature data");
  } else {
    Serial.print("Temperature: ");
    Serial.print(tempC);
    Serial.print(" °C");
  }

  delay(2000);
}