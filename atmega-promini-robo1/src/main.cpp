#include <Arduino.h>

// MOTOR outputs (must be pwm-capable pins)
#define PIN_MOTOR_1A 10
#define PIN_MOTOR_1B 11
#define PIN_MOTOR_2A 5
#define PIN_MOTOR_2B 6

// RC channel pins
#define PIN_STR 2
#define INT_STR 0
#define PIN_THR 3
#define INT_THR 1

// RC channel data
volatile uint32_t strLastPulseStart;
volatile uint32_t strLastPulseWidth;
volatile uint32_t thrLastPulseStart;
volatile uint32_t thrLastPulseWidth;

void strInterrupt() {
  uint32_t now = micros();
  uint8_t state = digitalRead(PIN_STR);

  if (state == 1) {
    strLastPulseStart = now;
  } else {
    strLastPulseWidth = now - strLastPulseStart;
  }
}

void thrInterrupt() {
  uint32_t now = micros();
  uint8_t state = digitalRead(PIN_THR);

  if (state == 1) {
    thrLastPulseStart = now;
  } else {
    thrLastPulseWidth = now - thrLastPulseStart;
  }
}

void testMotors()
{
  // motor test
  analogWrite(PIN_MOTOR_1A, 31);
  analogWrite(PIN_MOTOR_2A, 31);
  delay(50);
  analogWrite(PIN_MOTOR_1A, 0);
  analogWrite(PIN_MOTOR_2A, 0);
  delay(50);
  analogWrite(PIN_MOTOR_1B, 31);
  analogWrite(PIN_MOTOR_2B, 31);
  delay(50);
  analogWrite(PIN_MOTOR_1B, 0);
  analogWrite(PIN_MOTOR_2B, 0);
}

void setup() {
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, 0);

  Serial.begin(115200);
  Serial.println("Initializing");

  // make sure motors are stopped
  pinMode(PIN_MOTOR_1A, OUTPUT);
  pinMode(PIN_MOTOR_1B, OUTPUT);
  pinMode(PIN_MOTOR_2A, OUTPUT);
  pinMode(PIN_MOTOR_2B, OUTPUT);
  analogWrite(PIN_MOTOR_1A, 0);
  analogWrite(PIN_MOTOR_1B, 0);
  analogWrite(PIN_MOTOR_2A, 0);
  analogWrite(PIN_MOTOR_2B, 0);

  pinMode(PIN_STR, INPUT);
  attachInterrupt(INT_STR, strInterrupt, CHANGE);

  pinMode(PIN_THR, INPUT);
  attachInterrupt(INT_THR, thrInterrupt, CHANGE);

  testMotors();

  digitalWrite(LED_BUILTIN, 1);
  Serial.println("Running");
}

void loop() {
  uint32_t now = micros();

  #if 0
  static char buffer[64];
  snprintf(buffer, sizeof(buffer), "STR=%ld THR=%ld", strLastPulseWidth, thrLastPulseWidth);
  Serial.println(buffer);
  #endif

  bool validStr = strLastPulseWidth >= 1000 && strLastPulseWidth <= 2000
    && now - strLastPulseStart <= 1000000;
  bool validThr = thrLastPulseWidth >= 1000 && thrLastPulseWidth <= 2000
    && now - thrLastPulseStart <= 1000000;

  if (!validStr || !validThr) {
    // no signal, stop
    analogWrite(PIN_MOTOR_1A, 0);
    analogWrite(PIN_MOTOR_1B, 0);
    analogWrite(PIN_MOTOR_2A, 0);
    analogWrite(PIN_MOTOR_2B, 0);

    digitalWrite(LED_BUILTIN, 0);

  } else {
    // good signal
    uint8_t motor1a = 0;
    uint8_t motor1b = 0;
    uint8_t motor2a = 0;
    uint8_t motor2b = 0;

    int16_t thrPercent = 0;
    if (thrLastPulseWidth > 1600 || thrLastPulseWidth < 1400) {
      thrPercent = ((int16_t)(thrLastPulseWidth) - 1500) / 5;
    }

    int16_t strPercent = 0;
    if (strLastPulseWidth > 1600 || strLastPulseWidth < 1400) {
      strPercent = ((int16_t)(strLastPulseWidth) - 1500) / 5;
    }

    int16_t thr1Percent = 0;
    int16_t thr2Percent = 0;
    if (thrPercent >= 0) { 
      thr1Percent = thrPercent + strPercent;
      thr2Percent = thrPercent - strPercent;
    } else {
      thr1Percent = thrPercent - strPercent;
      thr2Percent = thrPercent + strPercent;
    }

    if (thr1Percent > 100) thr1Percent = 100;
    if (thr1Percent < -100) thr1Percent = -100;
    if (thr2Percent > 100) thr2Percent = 100;
    if (thr2Percent < -100) thr2Percent = -100;
    
    if (thr1Percent > 0) {
      motor1a = thr1Percent * 2;
    } else if (thr1Percent < 0) {
      motor1b = -thr1Percent * 2;
    }

    if (thr2Percent > 0) {
      motor2a = thr2Percent * 2;
    } else if (thr2Percent < 0) {
      motor2b = -thr2Percent * 2;
    }

    analogWrite(PIN_MOTOR_1A, motor1a);
    analogWrite(PIN_MOTOR_1B, motor1b);
    analogWrite(PIN_MOTOR_2A, motor2a);
    analogWrite(PIN_MOTOR_2B, motor2b);
    
    digitalWrite(LED_BUILTIN, 1);
  }

  delay(100);
}
