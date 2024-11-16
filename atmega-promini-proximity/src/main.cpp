#include <Arduino.h>
#include <EnableInterrupt.h>
#include <Servo.h>

//#define DEBUG

#define SONIC_TRIGGER 7
#define SONIC_ECHO 8

#define SERVO 9
#define LED_1 10
#define LED_2 11

//

Servo servo;

volatile uint32_t sonicEchoStart = 0;
volatile uint32_t sonicEchoStop = 0;
volatile uint32_t sonicPulseWidth = 0;

void sonicInterrupt() {
  uint32_t now = micros();

  uint8_t echoPin = digitalRead(SONIC_ECHO);
  if (echoPin == 1) {
    sonicEchoStart = now;
    //sonicWidth = 0;
  } else {
    sonicEchoStop = now;
    if (sonicEchoStart != 0) {
      sonicPulseWidth = sonicEchoStop - sonicEchoStart;
    }
  }
}

//

uint8_t servoAngle = 180;
uint32_t sonicDistance = 255;

void setup() {
#ifdef DEBUG
  Serial.begin(9600);
  Serial.println("Initializing...");
#endif

  pinMode(LED_BUILTIN, OUTPUT);

  pinMode(SERVO, OUTPUT);
  pinMode(LED_1, OUTPUT);
  pinMode(LED_2, OUTPUT);

  // 

  servo.attach(SERVO);
  // sweep at init
#if 0
  for (int i = 90; i >= 0; i--) {
    servo.write(i);
    delay(5);
  }
  for (int i = 0; i <= 180; i++) {
    servo.write(i);
    delay(5);
  }
  for (int i = 180; i > 90; i--) {
    servo.write(i);
    delay(5);
  }
#endif
  servo.write(servoAngle);

  //

  pinMode(SONIC_TRIGGER, OUTPUT);
  pinMode(SONIC_ECHO, INPUT);

  enableInterrupt(SONIC_ECHO, sonicInterrupt, CHANGE);

#ifdef DEBUG
  Serial.println("Interrupts set, ready to roll");
#endif
}

void loop() {
  // Sonic sensor probe
  digitalWrite(SONIC_TRIGGER, 1);
  delayMicroseconds(10);
  digitalWrite(SONIC_TRIGGER, 0);

  // Sonic sensor monitor output
  if (sonicPulseWidth != 0) {
    uint32_t newSonicDistance = sonicPulseWidth * 340 / 2000;
#ifdef DEBUG
    Serial.print("Sonic pulse width ");
    Serial.print(sonicWidth);
    Serial.print("us ");
    Serial.print(sonicDistance);
    Serial.println("mm");
#endif
    if (newSonicDistance > sonicDistance) {
      sonicDistance += (newSonicDistance - sonicDistance) / 4;
    } else if (newSonicDistance < sonicDistance) {
      sonicDistance -= (sonicDistance - newSonicDistance) / 4;
    }

    uint32_t newServoAngle = sonicDistance / 2;
    if (newServoAngle > 180) {
      newServoAngle = 180;
    }
    if (newServoAngle > servoAngle) {
      servoAngle++;
    } else if (newServoAngle < servoAngle) {
      servoAngle--;
    }
  } else {
    digitalWrite(LED_BUILTIN, 1);
    delay(50);
    digitalWrite(LED_BUILTIN, 0);
  }

  uint8_t proximityAlert = 0;
  if (sonicDistance < 100) {
    proximityAlert = 1;
  }

  servo.write(servoAngle);
  digitalWrite(LED_1, proximityAlert);
  analogWrite(LED_2, sonicDistance <= 255 ? sonicDistance : 255);

#ifdef DEBUG
  delay(1000);
#else
  delay(10);
#endif
}
