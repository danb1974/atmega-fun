#include <Arduino.h>

#define PIN_LED 13        // really???
#define BLINK_PERIOD 500  // ms between state changes

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
