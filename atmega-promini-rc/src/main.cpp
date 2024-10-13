#include <Arduino.h>

//#define DEBUG

// inputs from RC receiver (must be INT - not PCINT - capable pins)
#define PIN_STR_INPUT 2   // steering channel on INT0
#define PIN_THR_INPUT 3   // throttle channel on INT1

// leds for visualising input level (must be pwm-capable pins)
#define PIN_STR_LED 10
#define PIN_THR_LED 11

//

volatile uint32_t str_up_micros = 0;   // last up event
volatile uint32_t str_pulse_width = 0; // last pulse width
volatile uint32_t thr_up_micros = 0;
volatile uint32_t thr_pulse_width = 0;

void str_interrupt() {
  uint32_t now = micros();

  uint8_t state = digitalRead(PIN_STR_INPUT);
  if (state == 1) {
    str_up_micros = now;
    return;
  }
  
  if (str_up_micros == 0) {
    return;
  }

  // state == 0
  str_pulse_width = now - str_up_micros;
  if (str_pulse_width < 1000 || str_pulse_width > 2000) {
    // invalid
    str_pulse_width = 0;
  }
}

void thr_interrupt() {
  uint32_t now = micros();

  uint8_t state = digitalRead(PIN_THR_INPUT);
  if (state == 1) {
    thr_up_micros = now;
    return;
  }
  
  if (thr_up_micros == 0) {
    return;
  }

  // state == 0
  thr_pulse_width = now - thr_up_micros;
  if (thr_pulse_width < 1000 || thr_pulse_width > 2000) {
    // invalid
    thr_pulse_width = 0;
  }
}

void setup() {
#ifdef DEBUG
  Serial.begin(9600);
  Serial.println("Initializing...");
#endif

  pinMode(PIN_STR_LED, OUTPUT);

  pinMode(PIN_STR_INPUT, INPUT);
  pinMode(PIN_THR_INPUT, INPUT);

  attachInterrupt(digitalPinToInterrupt(PIN_STR_INPUT), str_interrupt, CHANGE);
  attachInterrupt(digitalPinToInterrupt(PIN_THR_INPUT), thr_interrupt, CHANGE);

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

void loop() {
  uint8_t str_led_value = pulseWidthToLedValue(str_pulse_width);
  uint8_t thr_led_value = pulseWidthToLedValue(thr_pulse_width);

  analogWrite(PIN_STR_LED, str_led_value);
  analogWrite(PIN_THR_LED, pulseWidthToLedValue(thr_pulse_width));

  printPulseData("STR", str_pulse_width, str_led_value);
  printPulseData("THR", thr_pulse_width, thr_led_value);

#ifdef DEBUG
  delay(100);
#else
  delay(10);
#endif
}
