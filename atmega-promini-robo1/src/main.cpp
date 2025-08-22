#include <Arduino.h>

// MOTOR outputs (must be pwm-capable pins)
#define PIN_MOTOR_1A 10
#define PIN_MOTOR_1B 11
#define PIN_MOTOR_2A 5
#define PIN_MOTOR_2B 6

// RC channel count and indexes (order does not actually matter but keep same as on receiver)
#define CHN_COUNT 2
#define CHN_STR 0
#define CHN_THR 1
#define PIN_STR 2
#define PIN_THR 3

// inputs from RC receiver (must be INT - not PCINT - capable pins)
uint8_t chnInputPins[CHN_COUNT] = {
  PIN_STR,  // steering channel on INT0 pin
  PIN_THR,  // throttle channel on INT1 pin
};

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
  if (pulseWidth < 1000 - 100 || pulseWidth > 2000 + 100) {
    // completely invalid
    pulseWidth = 0;
  } else {
    // slightly off
    if (pulseWidth < 1000) pulseWidth = 1000;
    if (pulseWidth > 2000) pulseWidth = 2000;
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

void setup() {
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, 0);

  // make sure motors are stopped
  pinMode(PIN_MOTOR_1A, OUTPUT);
  pinMode(PIN_MOTOR_1B, OUTPUT);
  pinMode(PIN_MOTOR_2A, OUTPUT);
  pinMode(PIN_MOTOR_2B, OUTPUT);
  analogWrite(PIN_MOTOR_1A, 0);
  analogWrite(PIN_MOTOR_1B, 0);
  analogWrite(PIN_MOTOR_2A, 0);
  analogWrite(PIN_MOTOR_2B, 0);

  for (uint8_t chnIndex = 0; chnIndex < CHN_COUNT; chnIndex++) {
    pinMode(chnInputPins[chnIndex], INPUT);
  }
  attachInterrupt(digitalPinToInterrupt(chnInputPins[CHN_STR]), strInterrupt, CHANGE);
  attachInterrupt(digitalPinToInterrupt(chnInputPins[CHN_THR]), thrInterrupt, CHANGE);

  //
  digitalWrite(LED_BUILTIN, 1);
}

void loop() {
  #if 1
  // motor test
  analogWrite(PIN_MOTOR_1A, 127);
  analogWrite(PIN_MOTOR_2A, 127);
  delay(1000);
  analogWrite(PIN_MOTOR_1A, 0);
  analogWrite(PIN_MOTOR_2A, 0);
  delay(1000);
  #endif

  //
}
