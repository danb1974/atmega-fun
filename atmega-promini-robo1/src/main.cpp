#include <Arduino.h>
#include <digitalWriteFast.h>
#include <FastLED.h>

// serial led matrix 4x4
#define LED_PIN 9
#define LED_COUNT 16

static CRGB ledStrip[LED_COUNT];

// MOTOR outputs (must be pwm-capable pins)
#define PIN_MOTOR_1A 10
#define PIN_MOTOR_1B 11
#define PIN_MOTOR_2A 5
#define PIN_MOTOR_2B 6

// RC channel pins
#define PIN_STR 2
#define PIN_THR 3

// RC channel data

struct rcInputs_t {
  uint32_t strLastPulseStart = 0;
  uint32_t strLastPulseWidth = 0;
  uint32_t thrLastPulseStart = 0;
  uint32_t thrLastPulseWidth = 0;
};

volatile static rcInputs_t rcInputs;

void strInterrupt() {
  uint8_t state = digitalReadFast(PIN_STR);
  uint32_t now = micros();

  if (state == 1) {
    rcInputs.strLastPulseStart = now;
  } else {
    rcInputs.strLastPulseWidth = now - rcInputs.strLastPulseStart;
  }
}

void thrInterrupt() {
  uint8_t state = digitalReadFast(PIN_THR);
  uint32_t now = micros();

  if (state == 1) {
    rcInputs.thrLastPulseStart = now;
  } else {
    rcInputs.thrLastPulseWidth = now - rcInputs.thrLastPulseStart;
  }
}

void testMotors()
{
  // motor test
  analogWrite(PIN_MOTOR_1A, 31);
  analogWrite(PIN_MOTOR_2A, 31);
  delay(100);
  analogWrite(PIN_MOTOR_1A, 0);
  analogWrite(PIN_MOTOR_2A, 0);
  delay(100);
  analogWrite(PIN_MOTOR_1B, 31);
  analogWrite(PIN_MOTOR_2B, 31);
  delay(100);
  analogWrite(PIN_MOTOR_1B, 0);
  analogWrite(PIN_MOTOR_2B, 0);
}

void setup() {
  pinMode(LED_BUILTIN, OUTPUT);
  analogWrite(LED_BUILTIN, 0);

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

  FastLED.addLeds<WS2812, LED_PIN, GRB>(ledStrip, LED_COUNT);
  FastLED.setBrightness(3);
  for (int32_t i = 0xff0000; i != 0; i >>= 8) {
    FastLED.showColor(CRGB(i));
    delay(100);
  }
  FastLED.showColor(CRGB(0));

  pinModeFast(PIN_STR, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(PIN_STR), strInterrupt, CHANGE);

  pinModeFast(PIN_THR, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(PIN_THR), thrInterrupt, CHANGE);

  testMotors();

  analogWrite(LED_BUILTIN, 255);
  Serial.println("Running");
}

void loop() {
  static rcInputs_t rcInputsCopy;
  static uint8_t badConsecutivePulses = 0;

  #if 0
  static char buffer[64];
  snprintf(buffer, sizeof(buffer), "STR=%ld THR=%ld", strLastPulseWidth, thrLastPulseWidth);
  Serial.println(buffer);
  #endif

  uint32_t now = micros();

  noInterrupts();
  memcpy(&rcInputsCopy, (void *)&rcInputs, sizeof(rcInputs_t));
  interrupts();

  bool validStr = rcInputsCopy.strLastPulseWidth >= 1000U && rcInputsCopy.strLastPulseWidth <= 2000U;
  bool freshStr = now - rcInputsCopy.strLastPulseStart <= 1000000U;
  bool validThr = rcInputsCopy.thrLastPulseWidth >= 1000U && rcInputsCopy.thrLastPulseWidth <= 2000U;
  bool freshThr = now - rcInputsCopy.thrLastPulseStart <= 1000000U;

  bool validPulse = validStr && validThr;
  bool freshPulse = freshStr && freshThr;

  if (validPulse && freshPulse) {
    badConsecutivePulses = 0;
  } else {
    if (badConsecutivePulses < 255) {
      badConsecutivePulses++;
    }
  }

  if (!freshPulse || badConsecutivePulses > 10) {
    //Serial.println("No signal");
    Serial.write("No signal: thr ");
    Serial.write(validThr ? "+" : "-");
    Serial.write("/");
    Serial.write(freshThr ? "+" : "-");
    Serial.write(" str ");
    Serial.write(validStr ? "+" : "-");
    Serial.write("/");
    Serial.write(freshStr ? "+" : "-");
    Serial.write(" bad ");
    Serial.print(badConsecutivePulses);
    Serial.println();

    // no good signal for a while
    analogWrite(PIN_MOTOR_1A, 0);
    analogWrite(PIN_MOTOR_1B, 0);
    analogWrite(PIN_MOTOR_2A, 0);
    analogWrite(PIN_MOTOR_2B, 0);

    analogWrite(LED_BUILTIN, 0);

  } else if (validPulse) {
    // good signal
    uint8_t motor1a = 0;
    uint8_t motor1b = 0;
    uint8_t motor2a = 0;
    uint8_t motor2b = 0;

    int16_t thrPercent = 0;
    if (rcInputsCopy.thrLastPulseWidth > 1600 || rcInputsCopy.thrLastPulseWidth < 1400) {
      thrPercent = ((int16_t)(rcInputsCopy.thrLastPulseWidth) - 1500) / 5;
    }

    int16_t strPercent = 0;
    if (rcInputsCopy.strLastPulseWidth > 1600 || rcInputsCopy.strLastPulseWidth < 1400) {
      strPercent = ((int16_t)(rcInputsCopy.strLastPulseWidth) - 1500) / 5;
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
    
    analogWrite(LED_BUILTIN, 255);

  } else {
    // stale but not bad enough to take action
    Serial.println("Stale signal");
    analogWrite(LED_BUILTIN, 127);
  }

  delay(10);
}
