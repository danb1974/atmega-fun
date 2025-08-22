#include <Arduino.h>

#define MOTOR_1A 10
#define MOTOR_1B 11

void setup() {
  pinMode(MOTOR_1A, OUTPUT);
  pinMode(MOTOR_1B, OUTPUT);
  analogWrite(MOTOR_1A, 0);
  analogWrite(MOTOR_1B, 0);
}

void loop() {
  analogWrite(MOTOR_1A, 127);
  delay(1000);
  analogWrite(MOTOR_1A, 0);
  delay(1000);
}
