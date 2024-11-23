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

static uint8_t servoAngle = 180;
static uint32_t sonicDistance = 255;

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

#define SONIC_PULSE_BUFFER_SIZE 16
static uint8_t sonicPulsePos = 0;
static uint32_t sonicPulseBuffer[SONIC_PULSE_BUFFER_SIZE];
static uint8_t loopCounter = 0;

void loop() {
  loopCounter++;

  // Sonic sensor probe
  digitalWrite(SONIC_TRIGGER, 1);
  delayMicroseconds(10);
  digitalWrite(SONIC_TRIGGER, 0);

  // Sonic sensor monitor output
  if (sonicPulseWidth != 0) {
    uint32_t curSonicDistance = sonicPulseWidth * 340 / 2000;
    sonicPulseBuffer[sonicPulsePos] = curSonicDistance;
    sonicPulsePos = (sonicPulsePos + 1) % SONIC_PULSE_BUFFER_SIZE;

    uint32_t curAvgSonicDistance = 0;
    for (uint8_t i = 0; i < SONIC_PULSE_BUFFER_SIZE; i++) {
      curAvgSonicDistance += sonicPulseBuffer[i];
    }
    curAvgSonicDistance /= SONIC_PULSE_BUFFER_SIZE;
#ifdef DEBUG
    Serial.print("Sonic pulse ");
    Serial.print(sonicPulseWidth);
    Serial.print("us ");
    Serial.print(curSonicDistance);
    Serial.print("mm avg ");
    Serial.print(curAvgSonicDistance);
    Serial.println("mm");
#endif
    sonicDistance = curAvgSonicDistance;

    uint32_t newServoAngle = sonicDistance / 1;
    if (newServoAngle > 180) {
      newServoAngle = 180;
    }
    servoAngle = newServoAngle;

  } else {
    digitalWrite(LED_BUILTIN, 1);
    delay(50);
    digitalWrite(LED_BUILTIN, 0);
  }

  uint8_t proximityAlert = 0;
  if (sonicDistance < 40) {
    proximityAlert = 1;
  }

  servo.write(servoAngle);
  digitalWrite(LED_1, (loopCounter % 64) > 32 ? proximityAlert : 0);
  analogWrite(LED_2, sonicDistance <= 255 ? sonicDistance : 255);

#ifdef DEBUG
  delay(10);
#else
  delay(5);
#endif
}
