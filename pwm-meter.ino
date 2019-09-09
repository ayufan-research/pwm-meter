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

SSD1306 display(DISPLAY_ADDRESS, DISPLAY_SDA, DISPLAY_SCL);

void setup() {
  Serial.begin(115200);
  Serial.println("Hello World");

  pinMode(DISPLAY_RST, OUTPUT); 
  digitalWrite(DISPLAY_RST, LOW); // low to reset OLED
  delay(50); 
  digitalWrite(DISPLAY_RST, HIGH); 

  display.init();
  display.displayOn();
}

bool detectLowHigh(int *low, int *high, int deadlineUs) {
  *low = 4096;
  *high = -1;
  int avg = 0;
  int cycles = 0;

  while(micros() < deadlineUs) {
    int value = analogRead(ANALOG_PIN);
    *low = min(*low, value);
    *high = max(*high, value);
    avg += value;
    cycles++;
  }

  if (*low >= *high || cycles == 0) {
    Serial.print("low=");
    Serial.print(*low);
    Serial.print(" high=");
    Serial.print(*high);
    Serial.print(" cycles=");
    Serial.println(cycles);
    Serial.println("source not detected");
    display.drawString(0, 0, "source not detected");
    return false;
  }

  if (*high - *low < LOW_HIGH_DIFF) {
    Serial.print("low=");
    Serial.print(*low);
    Serial.print(" high=");
    Serial.print(*high);
    Serial.print(" cycles=");
    Serial.println(cycles);
    Serial.println("too little of difference");
    display.drawString(0, 0, "too little of difference");
    return false;
  }

  int range = *high - *low;
  avg /= cycles;

  *low = avg - range / 2;
  *high = avg + range / 2;

  return true;
}

int getPulse(int *currentTimeUs, int deadlineUs, int maxDeadlineUs) {
    int value = -1;
 
    while(true) {
      *currentTimeUs = micros();
      if (maxDeadlineUs < *currentTimeUs) {
        value = -1;
        break;
      }
      if (deadlineUs < *currentTimeUs) {
        break;
      }
      value = analogRead(ANALOG_PIN);
    }

    return value;
}

bool measure() {
  int low = 4095;
  int high = 0;
  int ignored = 0;
  char buffer[256];
  int samples[MAX_SAMPLES];
  int sampleCount = 0;

  if (!detectLowHigh(&low, &high, micros() + 100*1000)) {
    return false;
  }

  int range = high - low;

  int lowPulse = low + range * 2 / 5;
  int highPulse = high - range * 2 / 5;

  int pulseStart = 0;
  int pulseState = -1;
  int pulseCycles = 0;

  int lowTotal = 0;
  int lowCount = 0;
  int highTotal = 0;
  int highCount = 0;

  sprintf(buffer, "low=%d high=%d", low, high);
  display.drawString(0, 0, buffer);

  int startTimeUs = micros();
  int pulsesDeadlineUs = startTimeUs + 1000*1000;
  int nextTimeUs = startTimeUs + PULSE_TIME; 

  while(true) {
    int currentTimeUs;
    int value = getPulse(&currentTimeUs, nextTimeUs, pulsesDeadlineUs);
    nextTimeUs = nextTimeUs + PULSE_TIME;

    if (value < 0) {
      break;
    }

    if (sampleCount < MAX_SAMPLES) {
      samples[sampleCount++] = value;
    }

    ++pulseCycles;

    if (value < lowPulse) {
      if (pulseState == 1) {
        highTotal += currentTimeUs - pulseStart;
        highCount++;
      }

      if (pulseState != 0) {
        pulseStart = currentTimeUs;
        pulseState = 0;
      }
    } else if (value > highPulse) {
      if (pulseState == 0) {
        lowTotal += currentTimeUs - pulseStart;
        lowCount++;
      }
 
      if (pulseState != 1) {
        pulseStart = currentTimeUs;
        pulseState = 1;
      }
    } else {
      ++ignored;
    }
  }

  int endTimeUs = micros();

  Serial.print("low=");
  Serial.print(low);
  Serial.print(" high=");
  Serial.println(high);

  if(lowCount > 0) {
    Serial.print("lowAvgDutyCycleUs=");
    Serial.println(lowTotal / lowCount);
  }
  Serial.print("lowTotal=");
  Serial.print(lowTotal);
  Serial.print(" lowCount=");
  Serial.println(lowCount);

  if(highCount > 0) {
    Serial.print("highAvgDutyCycleUs=");
    Serial.println(highTotal / highCount);
  }
  Serial.print("highTotal=");
  Serial.print(highTotal);
  Serial.print(" highCount=");
  Serial.println(highCount);

  Serial.print("frequency=");
  Serial.print(highCount);
  if (lowCount > 0 && highCount > 0) {
    int lowDutyUs = lowTotal / lowCount;
    int highDutyUs = highTotal / highCount;
    Serial.print(" dutyCycle255=");
    Serial.print(255 * highDutyUs / (lowDutyUs + highDutyUs));
    Serial.print(" dutyCycle100=");
    Serial.print(100 * highDutyUs / (lowDutyUs + highDutyUs));
  }
  Serial.print(" execTime=");
  Serial.println(endTimeUs - startTimeUs);

  Serial.print("pulseCycles=");
  Serial.print(pulseCycles);
  Serial.print(" ignored=");
  Serial.print(ignored);
  Serial.print(" sampleCount=");
  Serial.println(sampleCount);
  for (int i = 0; i < sampleCount; ++i) {
    Serial.println(samples[i]);
  }
  Serial.println();

  sprintf(buffer, "freq=%5d execTime=%7d", highCount, endTimeUs - startTimeUs);
  display.drawString(0, 10, buffer);
  if (lowCount > 0 && highCount > 0) {
    sprintf(buffer, "lowDC=%4d highDC=%4d", lowTotal / lowCount, highTotal / highCount);
    display.drawString(0, 20, buffer);
  }
}

void loop() {
  display.clear();
  
  measure();
  
  Serial.print("LOOP  ");
  Serial.println(millis());

  display.display();
  Serial.println();
}
