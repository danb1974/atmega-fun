#include <Arduino.h>

#define PIN_LED LED_BUILTIN

// ms between state changes
// if clock is correct, should change state each second
#define BLINK_PERIOD 1000

//

static uint8_t ledState = LOW;

void initLed() {
  pinMode(PIN_LED, OUTPUT);
}

void updateLedState() {
  digitalWrite(PIN_LED, ledState);
}

void toggleLedState() {
  ledState = ledState == LOW ? HIGH : LOW;
}

//

void setup() {
  initLed();
}

void loop() {
  updateLedState();
  toggleLedState();

  delay(BLINK_PERIOD);
}
