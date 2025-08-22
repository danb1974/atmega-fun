#include <Arduino.h>

// must be pwm-capable pins
#define MOTOR_1A 10
#define MOTOR_1B 11
#define MOTOR_2A 5
#define MOTOR_2B 6

void setup() {
  // make sure motors are stopped
  pinMode(MOTOR_1A, OUTPUT);
  pinMode(MOTOR_1B, OUTPUT);
  pinMode(MOTOR_2A, OUTPUT);
  pinMode(MOTOR_2B, OUTPUT);
  analogWrite(MOTOR_1A, 0);
  analogWrite(MOTOR_1B, 0);
  analogWrite(MOTOR_2A, 0);
  analogWrite(MOTOR_2B, 0);

  //
}

void loop() {
  analogWrite(MOTOR_1A, 127);
  analogWrite(MOTOR_2A, 127);
  delay(1000);
  analogWrite(MOTOR_1A, 0);
  analogWrite(MOTOR_2A, 0);
  delay(1000);
}
