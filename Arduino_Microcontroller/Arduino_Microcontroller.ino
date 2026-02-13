/*
 * PhotonVHealth - Solar Panel Efficiency Monitoring System's Arduino Microcontroller
 * Copyright (C) 2026 Saif Kayyali
 * GNU GPLv3
 */


// Yet to get components so code is improvised for now

float light = 800;
float temp = 28;
float power = 3.0;

void setup() {
  Serial.begin(9600);
}

void loop() {

  light -= 50;     // simulate shade/dust
  temp += 1;       // simulate overheating
  power -= 0.2;    // simulate power drop

  if (light < 200) light = 800;
  if (temp > 50) temp = 28;
  if (power < 1.0) power = 3.0;

  Serial.print(light);
  Serial.print(",");
  Serial.print(temp);
  Serial.print(",");
  Serial.println(power);

  delay(2000);
}
