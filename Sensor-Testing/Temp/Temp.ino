#define LM35_PIN 35

float smooth_temp() {
  long sum = 0;
  int samples = 50; 

  // Dummy read to settle the ADC
  analogReadMilliVolts(LM35_PIN); 
  delay(10);

  for (int i = 0; i < samples; i++) {
    sum += analogReadMilliVolts(LM35_PIN);
    delay(2); 
  }

  float average = (float)sum / samples;

  float currentTemp = average / 10.0;

  return currentTemp;
}

void setup() {
  Serial.begin(115200);
  delay(1000);

  Serial.println("LM35 Temperature Test Starting...");
}

void loop() {
  float tempC = smooth_temp();

  Serial.print("Temp: ");
  Serial.print(tempC);
  Serial.println(" °C");

  delay(2000);
}