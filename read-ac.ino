// Uses: https://dl.espressif.com/dl/package_esp32_index.json
// Uses: https://github.com/ThingPulse/esp8266-oled-ssd1306
// TTGO LoRa32-OLED V1

#include <SSD1306.h>
#include <OLEDDisplayUi.h>

#define DISPLAY_ADDRESS 0x3c
#define DISPLAY_SDA 21
#define DISPLAY_SCL 22
#define DISPLAY_RST 16

#define ANALOG_PIN 4

#define LOW_HIGH_DIFF 30
#define PULSE_TIME (1000*1000/13000)

#define MAX_SAMPLES 1000
#define SAMPLE_TIME 100
#define SAMPLE_SPEED (1000*1000 / SAMPLE_TIME / MAX_SAMPLES)

int samples[MAX_SAMPLES];

SSD1306 display(DISPLAY_ADDRESS, DISPLAY_SDA, DISPLAY_SCL);

void setup() {
  Serial.begin(115200);
  Serial.println("Hello World");

  pinMode(DISPLAY_RST, OUTPUT); 
  digitalWrite(DISPLAY_RST, LOW); // low to reset OLED
  delay(50); 
  digitalWrite(DISPLAY_RST, HIGH); 

  analogSetCycles(1);

  display.init();
  display.displayOn();
}

int pulse() {
  int finishTime = micros() + SAMPLE_TIME;
  int value = analogRead(ANALOG_PIN);
  while(micros() < finishTime);
  return value;
}

void loop() {
  for (int i = 0; i < MAX_SAMPLES; ++i) {
    samples[i] = pulse();
  }

  for (int i = 0; i < MAX_SAMPLES; ++i) {
    Serial.println(samples[i]);
    delayMicroseconds(SAMPLE_TIME * SAMPLE_SPEED);
  }

  delay(500);
}
