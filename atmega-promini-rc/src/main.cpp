#include <Arduino.h>
#include<EnableInterrupt.h>

#define DEBUG

// channel count and indexes (order does not actually matter but keep same as on receiver)
#define CHN_COUNT 2
#define CHN_STR 0
#define CHN_THR 1

// inputs from RC receiver (must be INT - not PCINT - capable pins)
uint8_t chnInputPins[CHN_COUNT] = {
  2,  // steering channel on INT0 pin
  3,  // throttle channel on INT1 pin
};

// leds for visualising input level (must be pwm-capable pins)
uint8_t chnOutputLeds[CHN_COUNT] = {
  10, // steering led
  11  // throttle led
};

#define SONIC_TRIGGER 7
#define SONIC_ECHO 8

//

volatile uint32_t chnLastPulseStart[CHN_COUNT];
volatile uint32_t chnLastPulseWidth[CHN_COUNT];

void handleChnEvent(uint32_t now, uint8_t chnIndex, uint8_t chnState) {
  if (chnState == 1) {
    // up transition, record start
    chnLastPulseStart[chnIndex] = now;
    return;
  }
  
  if (chnLastPulseStart[chnIndex] == 0) {
    // no up transition yet
    return;
  }

  // down transition, compute width
  uint16_t pulseWidth = now - chnLastPulseStart[chnIndex];
  if (pulseWidth < 1000 || pulseWidth > 2000) {
    // invalid
    pulseWidth = 0;
  }
  chnLastPulseWidth[chnIndex] = pulseWidth;
}

void strInterrupt() {
  uint32_t now = micros();
  uint8_t state = digitalRead(chnInputPins[CHN_STR]);
  handleChnEvent(now, CHN_STR, state);
}

void thrInterrupt() {
  uint32_t now = micros();
  uint8_t state = digitalRead(chnInputPins[CHN_THR]);
  handleChnEvent(now, CHN_THR, state);
}

void printPulseData(const uint8_t chnIndex, uint16_t pulseWidth, uint8_t ledValue) {
#ifdef DEBUG
  Serial.print("Channel ");
  Serial.print(chnIndex);
  Serial.print(": ");
  Serial.print(pulseWidth);
  Serial.print("us pulse width, led output ");
  Serial.println(ledValue);
#endif
}

uint8_t pulseWidthToLedValue(uint16_t pulseWidth) {
  return pulseWidth > 0 ? (pulseWidth - 1000) / 4 : 0;
}

//

volatile uint32_t sonicStart = 0;
volatile uint32_t sonicStop = 0;
volatile uint32_t sonicWidth = 0;

void sonicInterrupt() {
  uint32_t now = micros();

  uint8_t echoPin = digitalRead(SONIC_ECHO);
  if (echoPin == 1) {
    sonicStart = now;
    sonicWidth = 0;
  } else {
    sonicStop = now;
    if (sonicStart != 0) {
      sonicWidth = sonicStop - sonicStart;
    }
  }
}

//

void setup() {
#ifdef DEBUG
  Serial.begin(9600);
  Serial.println("Initializing...");
#endif

  pinMode(LED_BUILTIN, OUTPUT);

  //

  for (uint8_t chnIndex = 0; chnIndex < CHN_COUNT; chnIndex++) {
    pinMode(chnInputPins[chnIndex], INPUT);
    pinMode(chnOutputLeds[chnIndex], OUTPUT);
  }
  
  //attachInterrupt(digitalPinToInterrupt(chnInputPins[CHN_STR]), strInterrupt, CHANGE);
  //attachInterrupt(digitalPinToInterrupt(chnInputPins[CHN_THR]), thrInterrupt, CHANGE);
  enableInterrupt(chnInputPins[CHN_STR], strInterrupt, CHANGE);
  enableInterrupt(chnInputPins[CHN_THR], thrInterrupt, CHANGE);

  //

  pinMode(SONIC_TRIGGER, OUTPUT);
  pinMode(SONIC_ECHO, INPUT);

  enableInterrupt(SONIC_ECHO, sonicInterrupt, CHANGE);

#ifdef DEBUG
  Serial.println("Interrupts set, ready to roll");
#endif
}

//uint8_t intLedState = 0;

void loop() {
  // RC channel monitor outputs
  for (uint8_t chnIndex = 0; chnIndex < CHN_COUNT; chnIndex++) {
    uint8_t ledValue = pulseWidthToLedValue(chnLastPulseWidth[chnIndex]);

    analogWrite(chnOutputLeds[chnIndex], ledValue);
    printPulseData(chnIndex, chnLastPulseWidth[chnIndex], ledValue);
  }

  // Sonic sensor probe
  digitalWrite(SONIC_TRIGGER, 1);
  delayMicroseconds(10);
  digitalWrite(SONIC_TRIGGER, 0);

  if (sonicWidth != 0) {
    uint32_t sonicDistance = sonicWidth * 340 / 2000;
    Serial.print("Sonic pulse width ");
    Serial.print(sonicWidth);
    Serial.print("us ");
    Serial.print(sonicDistance);
    Serial.println("mm");
  }

  //digitalWrite(LED_BUILTIN, intLedState++ & 0x10 ? 1 : 0); // slow down the blink

#ifdef DEBUG
  delay(1000);
#else
  delay(10);
#endif
}
