#include <Arduino.h>

// channel count and indexes (order does not actually matter but keep same as on receiver)
#define CHN_COUNT 2
#define CHN_THR 0
#define CHN_AUX 1

// input pins from RX
#define PIN_THR 2
#define PIN_AUX 3

// output pins to lights
#define PIN_BRAKE 10
#define PIN_HAZARD 11

// inputs from RC receiver (must be INT - not PCINT - capable pins)
uint8_t chnInputPins[CHN_COUNT] = {
  PIN_THR,  // throttle channel on INT0 pin
  PIN_AUX,  // aux channel on INT1 pin
};

// leds for visualising input level (must be pwm-capable pins)
uint8_t chnOutputLeds[CHN_COUNT] = {
  PIN_BRAKE,  // brake lights
  PIN_HAZARD, // hazard lights
};

//

static volatile uint32_t chnLastPulseStart[CHN_COUNT];
static volatile uint32_t chnLastPulseWidth[CHN_COUNT];

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

void auxInterrupt() {
  uint32_t now = micros();
  uint8_t state = digitalRead(chnInputPins[CHN_AUX]);
  handleChnEvent(now, CHN_AUX, state);
}

void thrInterrupt() {
  uint32_t now = micros();
  uint8_t state = digitalRead(chnInputPins[CHN_THR]);
  handleChnEvent(now, CHN_THR, state);
}

//

void setup() {
  for (uint8_t chnIndex = 0; chnIndex < CHN_COUNT; chnIndex++) {
    pinMode(chnInputPins[chnIndex], INPUT);
    pinMode(chnOutputLeds[chnIndex], OUTPUT);
  }
  
  attachInterrupt(digitalPinToInterrupt(chnInputPins[CHN_AUX]), auxInterrupt, CHANGE);
  attachInterrupt(digitalPinToInterrupt(chnInputPins[CHN_THR]), thrInterrupt, CHANGE);

  // ready
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, 1);
}

void processThr() {
  uint32_t pulseWidth = chnLastPulseWidth[CHN_THR];
  if (pulseWidth >= 1450 && pulseWidth <= 1550) {
    // brake on if no command
    analogWrite(chnOutputLeds[CHN_THR], 255);
  } else {
    // brake off (normal tai lights) if command
    analogWrite(chnOutputLeds[CHN_THR], 15);
  }
}

void processAux3P(bool blinkState) {
  uint32_t pulseWidth = chnLastPulseWidth[CHN_AUX];
  if (pulseWidth > 1750) {
    digitalWrite(chnOutputLeds[CHN_AUX], 1);
  } else if (pulseWidth < 1250) {
    digitalWrite(chnOutputLeds[CHN_AUX], blinkState);
  } else {
    digitalWrite(chnOutputLeds[CHN_AUX], 0);
  }
}

void processAux2P(bool blinkState) {
  uint32_t pulseWidth = chnLastPulseWidth[CHN_AUX];
  if (pulseWidth > 1750) {
    digitalWrite(chnOutputLeds[CHN_AUX], blinkState);
  } else if (pulseWidth < 1250) {
    digitalWrite(chnOutputLeds[CHN_AUX], 0);
  } else {
    digitalWrite(chnOutputLeds[CHN_AUX], 0);
  }
}

void loop() {
  static uint8_t pulse = 0;
  static uint8_t blinkState = 0;

  processThr();
  processAux2P(blinkState);

  pulse++;
  blinkState = (pulse / 5) & 0x01;

  delay(100);
}
