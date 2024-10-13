#include <Arduino.h>

//#define DEBUG

// channel count and indexes (order does not actually matter but keep same as on receiver)
#define CHN_COUNT 2
#define CHN_STR 0
#define CHN_THR 1

// inputs from RC receiver (must be INT - not PCINT - capable pins)
#define PIN_STR_INPUT 2   // steering channel on INT0
#define PIN_THR_INPUT 3   // throttle channel on INT1

// leds for visualising input level (must be pwm-capable pins)
#define PIN_STR_LED 10
#define PIN_THR_LED 11

//

volatile uint32_t chnLastPulseStart[CHN_COUNT];
volatile uint32_t chnLastPulseWidth[CHN_COUNT];

void handleChnEvent(uint32_t now, uint8_t chnIndex, uint8_t chnState) {
  if (chnState == 1) {
    // up transition
    chnLastPulseStart[chnIndex] = now;
    return;
  }
  
  if (chnLastPulseStart[chnIndex] == 0) {
    // no up transition yet
    return;
  }

  // down transition
  uint16_t pulseWidth = now - chnLastPulseStart[chnIndex];
  if (pulseWidth < 1000 || pulseWidth > 2000) {
    // invalid
    pulseWidth = 0;
  }
  chnLastPulseWidth[chnIndex] = pulseWidth;
}

void strInterrupt() {
  uint32_t now = micros();
  uint8_t state = digitalRead(PIN_STR_INPUT);
  handleChnEvent(now, CHN_STR, state);
}

void thrInterrupt() {
  uint32_t now = micros();
  uint8_t state = digitalRead(PIN_THR_INPUT);
  handleChnEvent(now, CHN_THR, state);
}

void setup() {
#ifdef DEBUG
  Serial.begin(9600);
  Serial.println("Initializing...");
#endif

  pinMode(LED_BUILTIN, OUTPUT);

  pinMode(PIN_STR_LED, OUTPUT);
  pinMode(PIN_THR_LED, OUTPUT);

  pinMode(PIN_STR_INPUT, INPUT);
  pinMode(PIN_THR_INPUT, INPUT);

  attachInterrupt(digitalPinToInterrupt(PIN_STR_INPUT), strInterrupt, CHANGE);
  attachInterrupt(digitalPinToInterrupt(PIN_THR_INPUT), thrInterrupt, CHANGE);

#ifdef DEBUG
  Serial.println("Interrupts set, ready to roll");
#endif
}

void printPulseData(const char *label, uint16_t pulseWidth, uint8_t ledValue) {
#ifdef DEBUG
  Serial.print(label);
  Serial.print(": ");
  Serial.print(pulseWidth);
  Serial.print("us pulse width, led output ");
  Serial.println(ledValue);
#endif
}

uint8_t pulseWidthToLedValue(uint16_t pulseWidth) {
  return pulseWidth > 0 ? (pulseWidth - 1000) / 4 : 0;
}

uint8_t intLedState = 0;

void loop() {
  uint8_t ledValues[CHN_COUNT];
  for (uint8_t i = 0; i < CHN_COUNT; i++) {
    ledValues[i] = pulseWidthToLedValue(chnLastPulseWidth[i]);
  }

  analogWrite(PIN_STR_LED, ledValues[CHN_STR]);
  analogWrite(PIN_THR_LED, ledValues[CHN_THR]);

  printPulseData("STR", chnLastPulseWidth[CHN_STR], ledValues[CHN_STR]);
  printPulseData("THR", chnLastPulseWidth[CHN_THR], ledValues[CHN_THR]);

  digitalWrite(LED_BUILTIN, intLedState++ & 0x10 ? 1 : 0); // slow down the blink

#ifdef DEBUG
  delay(100);
#else
  delay(10);
#endif
}
