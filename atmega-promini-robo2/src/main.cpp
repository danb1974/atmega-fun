#define FASTLED_ALLOW_INTERRUPTS 1

#include <Arduino.h>
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

static rcInputs_t rcInputs;

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

  pinMode(PIN_STR, INPUT);
  pinMode(PIN_THR, INPUT);

  testMotors();

  analogWrite(LED_BUILTIN, 255);
  Serial.println("Running");
}

void loop() {
  uint32_t now = micros();

  uint32_t strPulseWidth = pulseIn(PIN_STR, HIGH, 25000);
  if (strPulseWidth > 0) {
    rcInputs.strLastPulseWidth = strPulseWidth;
    rcInputs.strLastPulseStart = now;
  }
  uint32_t thrPulseWidth = pulseIn(PIN_THR, HIGH, 25000);
  if (thrPulseWidth > 0) {
    rcInputs.thrLastPulseWidth = thrPulseWidth;
    rcInputs.thrLastPulseStart = now;
  }

  bool validStr = rcInputs.strLastPulseWidth >= 1000U && rcInputs.strLastPulseWidth <= 2000U;
  bool freshStr = now - rcInputs.strLastPulseStart <= 100000U;
  bool validThr = rcInputs.thrLastPulseWidth >= 1000U && rcInputs.thrLastPulseWidth <= 2000U;
  bool freshThr = now - rcInputs.thrLastPulseStart <= 100000U;

  bool validPulse = validStr && validThr;
  bool freshPulse = freshStr && freshThr;

  static uint8_t badConsecutivePulses = 0;
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

    FastLED.showColor(CHSV(0, 0, 0));

  } else if (validPulse) {
    // good signal
    uint8_t motor1a = 0;
    uint8_t motor1b = 0;
    uint8_t motor2a = 0;
    uint8_t motor2b = 0;

    int16_t thrPercent = 0;
    if (rcInputs.thrLastPulseWidth > 1600) {
      thrPercent = ((int16_t)(rcInputs.thrLastPulseWidth - 100) - 1500) / 3;
      if (thrPercent > 100) {
        thrPercent = 100;
      }
    }
    if (rcInputs.thrLastPulseWidth < 1400) {
      thrPercent = ((int16_t)(rcInputs.thrLastPulseWidth + 100) - 1500) / 3;
      if (thrPercent < -100) {
        thrPercent = -100;
      }
    }

    int16_t strPercent = 0;
    if (rcInputs.strLastPulseWidth > 1600) {
      strPercent = ((int16_t)(rcInputs.strLastPulseWidth - 100) - 1500) / 3;
      if (strPercent > 100) {
        strPercent = 100;
      }
    }
    if (rcInputs.strLastPulseWidth < 1400) {
      strPercent = ((int16_t)(rcInputs.strLastPulseWidth + 100) - 1500) / 3;
      if (strPercent < -100) {
        strPercent = -100;
      }
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

    #if 0
    // hsv: 0 = red, 96 = green
    static uint8_t bar1O[4] = {15, 8, 7, 0};
    static uint8_t bar1I[4] = {14, 9, 6, 1};
    static uint8_t bar2I[4] = {13, 10, 5, 2};
    static uint8_t bar2O[4] = {12, 11, 4, 3};

    uint8_t hsv1V = floor(abs(thr1Percent) * 96U / 100U);
    uint8_t hsv2V = floor(abs(thr2Percent) * 96U / 100U);
    uint8_t hsv1L = floor(min(abs(thr1Percent), 99U) / 25U);
    uint8_t hsv2L = floor(min(abs(thr2Percent), 99U) / 25U);

    for (uint8_t i = 0; i < LED_COUNT; i++) {
      ledStrip[i] = CHSV(0, 0, 0);
    }
    if (thr1Percent != 0) {
      for (uint8_t l = 0; l <= hsv1L; l++) {
        uint8_t i = (thr1Percent > 0) ? l : 3 - l;
        ledStrip[bar1I[i]] = CHSV(hsv1V, 255, 255);
        ledStrip[bar1O[i]] = CHSV(hsv1V, 255, 255);
      }
    }
    if (thr2Percent != 0) {
      for (uint8_t l = 0; l <= hsv2L; l++) {
        uint8_t i = (thr2Percent > 0) ? l : 3 - l;
        ledStrip[bar2I[i]] = CHSV(hsv2V, 255, 255);
        ledStrip[bar2O[i]] = CHSV(hsv2V, 255, 255);
      }
    }
    #else
    static uint8_t bar1[8] = {14, 15, 8, 9, 6, 7, 0, 1};
    static uint8_t bar2[8] = {13, 12, 11, 10, 5, 4, 3, 2};

    uint8_t hsv1V = floor(abs(thr1Percent) * 96U / 100U);
    uint8_t hsv2V = floor(abs(thr2Percent) * 96U / 100U);
    uint8_t hsv1L = floor(min(abs(thr1Percent), 95U) / 12U);
    uint8_t hsv2L = floor(min(abs(thr2Percent), 95U) / 12U);

    for (uint8_t i = 0; i < LED_COUNT; i++) {
      ledStrip[i] = CHSV(0, 0, 0);
    }
    if (thr1Percent != 0) {
      for (uint8_t l = 0; l <= hsv1L; l++) {
        uint8_t i = (thr1Percent > 0) ? l : 7 - l;
        ledStrip[bar1[i]] = CHSV(hsv1V, 255, 255);
      }
    }
    if (thr2Percent != 0) {
      for (uint8_t l = 0; l <= hsv2L; l++) {
        uint8_t i = (thr2Percent > 0) ? l : 7 - l;
        ledStrip[bar2[i]] = CHSV(hsv2V, 255, 255);
      }
    }
    #endif
    FastLED.show();
    
    analogWrite(LED_BUILTIN, 255);

  } else {
    // stale but not bad enough to take action
    Serial.println("Stale signal");
    FastLED.showColor(CHSV(0, 0, 0));
    analogWrite(LED_BUILTIN, 127);
  }

  delay(10);
}
