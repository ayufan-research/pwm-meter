#include <SSD1306.h>
#include <OLEDDisplayUi.h>

#define DISPLAY_ADDRESS 0x3c
#define DISPLAY_SDA 21
#define DISPLAY_SCL 22
#define DISPLAY_RST 16

#define ANALOG_PIN 4

#define LOW_HIGH_DIFF 30

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

bool measure(int detectDeadline, int pulsesDeadline) {
  int low = 4095;
  int high = 0;
  int cycles = 0;
  int pulseCycles = 0;
  int ignored = 0;
  char buffer[256];

  Serial.println("detect start:");

  while(millis() < detectDeadline) {
    int value = analogRead(ANALOG_PIN);
    low = min(low, value);
    high = max(high, value);
    cycles++;
  }

  Serial.print("low=");
  Serial.println(low);
  Serial.print("high=");
  Serial.println(high);
  Serial.print("cycles=");
  Serial.println(cycles);

  if (low >= high) {
    Serial.println("source not detected");
    display.drawString(0, 0, "source not detected");
    return false;
  }

  if (high - low < LOW_HIGH_DIFF) {
    Serial.println("too little of difference");
    display.drawString(0, 0, "too little of difference");
    return false;
  }

  int range = high - low;

  int lowPulse = low + range * 2 / 5;
  int highPulse = high - range * 2 / 5;

  int pulseStart = 0;
  int pulseState = -1;

  int lowTotal = 0;
  int lowCount = 0;
  int highTotal = 0;
  int highCount = 0;

  sprintf(buffer, "low=%d high=%d", low, high);
  display.drawString(0, 0, buffer);

  int startTime = millis();

  while(true) {
    int currentTime = millis();
    int currentTimeMicros = micros();
    if (pulsesDeadline < currentTime) {
      break;
    }
 
    int value = analogRead(ANALOG_PIN);

    ++pulseCycles;
     
    if (value < lowPulse) {
      if (pulseState == 1) {
        highTotal += currentTimeMicros - pulseStart;
        highCount++;
      }
 
      if (pulseState != 0) {
        pulseStart = currentTimeMicros;
        pulseState = 0;
      }
    } else if (value > highPulse) {
      if (pulseState == 0) {
        lowTotal += currentTimeMicros - pulseStart;
        lowCount++;
      }
 
      if (pulseState != 1) {
        pulseStart = currentTimeMicros;
        pulseState = 1;
      }
    } else {
      ++ignored;
      continue;
    }
  }

  int endTime = millis();

  if(lowCount > 0) {
    Serial.print("lowAvgDutyCycleUs=");
    Serial.println(lowTotal / lowCount);
    Serial.print("lowDutyCycle=");
    Serial.println((endTime - startTime) * 1000 / lowCount);
  }
  Serial.print("lowTotal=");
  Serial.println(lowTotal);
  Serial.print("lowCount=");
  Serial.println(lowCount);

  if(highCount > 0) {
    Serial.print("highAvgDutyCycleUs=");
    Serial.println(highTotal / highCount);
    Serial.print("highDutyCycleUs=");
    Serial.println((endTime - startTime) * 1000 / highCount);
  }
  Serial.print("highTotal=");
  Serial.println(highTotal);
  Serial.print("highCount=");
  Serial.println(highCount);

  Serial.println("frequency=");
  Serial.println(1000 * ((lowCount+highCount)/2) / (endTime - startTime));

  sprintf(buffer, "freq=%d", 1000 * ((lowCount+highCount)/2) / (endTime - startTime));
  display.drawString(0, 10, buffer);
  if (lowCount > 0 && highCount > 0) {
    sprintf(buffer, "lowDC=%d highDC=%d", lowTotal / lowCount, highTotal / highCount);
    display.drawString(0, 20, buffer);
  }

  Serial.print("pulseCycles=");
  Serial.println(pulseCycles);
  Serial.print("ignored=");
  Serial.println(ignored);

}

void loop() {
  Serial.print("LOOP  ");
  Serial.println(millis());

  display.clear();
  
  measure(millis() + 100, millis() + 1000);
  
  display.display();
  Serial.println();
}
