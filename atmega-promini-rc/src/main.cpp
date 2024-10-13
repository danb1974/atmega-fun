#include <Arduino.h>

#define PIN_STR_LED 10
#define PIN_THR_LED 11

#define PIN_STR_INPUT 2
#define PIN_THR_INPUT 3

//#define DEBUG

//

volatile unsigned long str_up_micros = 0;
volatile unsigned long str_pulse_width = 0;
volatile unsigned long thr_up_micros = 0;
volatile unsigned long thr_pulse_width = 0;

void str_interrupt() {
  unsigned long now = micros();

  int state = digitalRead(PIN_STR_INPUT);
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
    // invalied
    str_pulse_width = 0;
  }
}

void thr_interrupt() {
  unsigned long now = micros();

  int state = digitalRead(PIN_THR_INPUT);
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
    // invalied
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
}

void loop() {
  unsigned int str_led_value = str_pulse_width > 0 ? (str_pulse_width - 1000) / 4 : 0;
  analogWrite(PIN_STR_LED, str_led_value);

  unsigned int thr_led_value = thr_pulse_width > 0 ? (thr_pulse_width - 1000) / 4 : 0;
  analogWrite(PIN_THR_LED, thr_led_value);

#ifdef DEBUG
  Serial.print("STR: ");
  Serial.print(str_pulse_width);
  Serial.print("us / ");
  Serial.print(str_led_value);
  Serial.println("led\n");
#endif

#ifdef DEBUG
  delay(100);
#else
  delay(10);
#endif
}
