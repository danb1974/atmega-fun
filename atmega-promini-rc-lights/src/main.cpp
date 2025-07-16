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

static volatile uint32_t _chnLastPulseStart[CHN_COUNT];
static volatile uint32_t _chnLastPulseEnd[CHN_COUNT];
static volatile uint32_t chnLastPulseWidth[CHN_COUNT];

void handleChnEvent(uint32_t now, uint8_t chnIndex, uint8_t chnState) {
  if (chnState == 1) {
    // up transition, record start
    _chnLastPulseStart[chnIndex] = now;
    return;
  }
  
  if (_chnLastPulseStart[chnIndex] == 0) {
    // no up transition yet
    return;
  }

  // down transition, compute width
  _chnLastPulseEnd[chnIndex] = now;
  uint16_t pulseWidth = _chnLastPulseEnd[chnIndex] - _chnLastPulseStart[chnIndex];
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

enum THR_STATES {
  ACCEL,
  NEUTRAL,
  BRAKE
};

// Brake light rules
// - brake from neutral && neutral short : ON
// - any to neutral : ON
// - any to accel: OFF
// - in accell (optional)
//   - if higher: OFF
//   - if lower: ON

void processThr() {
  static THR_STATES lastThrState = NEUTRAL;
  static uint32_t lastThrStateTs = micros();
  static uint32_t lastThrPulseWidth = 0;
  static uint32_t lastThrPulseWidthTs = micros();
  static bool brakeLight = true;

  uint32_t now = micros();
  uint32_t pulseWidth = chnLastPulseWidth[CHN_THR];

  THR_STATES thrState = NEUTRAL;
  if (pulseWidth <= 1450) {
    thrState = BRAKE;
  } else if (pulseWidth >= 1550) {
    thrState = ACCEL;
  }

  if (thrState != lastThrState) {
    // handle transitions

    if (thrState == ACCEL) {
      // transition to accel; accel variation will be handled later
      brakeLight = false;
    } else if (thrState == NEUTRAL) {
      brakeLight = true;
    } else if (thrState == BRAKE) {
      if (lastThrState == NEUTRAL && now - lastThrStateTs < 50000U) { // micros
        brakeLight = true;
      } else if (lastThrState == ACCEL) {
        brakeLight = true;
      } else {
        brakeLight = false;
      }
    }
  }

  if (pulseWidth != lastThrPulseWidth) {
    // brake on accell going down
    if (thrState == ACCEL && lastThrState == ACCEL) {
      if (pulseWidth < lastThrPulseWidth - 10) {
        brakeLight = true;
      } else {
        brakeLight = false;
      }
    }
  }

  if (thrState != lastThrState) {
    lastThrStateTs = now; //_chnLastPulseStart[CHN_THR];
    lastThrState = thrState;
  }

  if (pulseWidth != lastThrPulseWidth) {
    lastThrPulseWidthTs = now;
    lastThrPulseWidth = pulseWidth;
  }

  if (brakeLight) {
    analogWrite(chnOutputLeds[CHN_THR], 255);
  } else {
    analogWrite(chnOutputLeds[CHN_THR], 31);
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
  static uint8_t blinkState = 0;
  static uint8_t blinkPattern[] = {1, 1, 1, 0, 0, 0, 0, 0, 0, 0};

  uint32_t now = millis();
  uint32_t pulse = now / 80;

  processThr();

  blinkState = blinkPattern[pulse % sizeof(blinkPattern)];
  processAux2P(blinkState);

  // TODO
  delay(3);
}
