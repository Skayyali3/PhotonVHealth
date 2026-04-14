#define KY018_PIN 34

float smooth_light() {
  float sum = 0;
  for (int i = 0; i < 50; i++) {
    sum += analogRead(KY018_PIN);
  }
  return sum / 50.0;
}

void setup() {
  Serial.begin(115200);
  delay(1000);

  Serial.println("KY-018 LDR Test Starting...");
}

void loop() {
    float lightVal = smooth_light();
    float adjustedLight = 4095 - lightVal;
    float percentageLight = (adjustedLight / 4095.0) * 100.0;
    
    Serial.print("Light Intensity: "); Serial.print(percentageLight); Serial.println("%");
    Serial.print("Amount of Light: "); Serial.print(adjustedLight); Serial.println(" a.u.");
    delay(3000);
}