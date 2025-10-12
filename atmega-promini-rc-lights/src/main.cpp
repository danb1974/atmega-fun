#include <Arduino.h>

// RC channel pins
#define PIN_THR 2
#define PIN_AUX 3

// output pins to lights (pwm capable)
#define PIN_BRAKE 10
#define PIN_HAZARD 11

// RC channel data
struct rcInput_t {
  uint32_t thrLastPulseStart = 0;
  uint32_t thrLastPulseWidth = 0;
  uint32_t auxLastPulseStart = 0;
  uint32_t auxLastPulseWidth = 0;
};

static rcInput_t rcInputs;

// rc pulse width range (us)
#define MIN_VALID_PULSE 1000U - 100U
#define MAX_VALID_PULSE 2000U + 100U

// aux switch thresholds (us)
#define AUX_CHN_LOW_THRES 1250U
#define AUX_CHN_HIGH_THRES 1750U

// brightness values for brake leds
#define POSITION_LIGHT 31U 
#define BRAKE_LIGHT 255U

// when easying off throttle, what decrease should trigger brake
#define THR_BRAKE_TRIGGER_OFFSET 20U

//

void setup() {
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, 0);

  pinMode(PIN_THR, INPUT);
  pinMode(PIN_AUX, INPUT);

  pinMode(PIN_BRAKE, OUTPUT);
  pinMode(PIN_HAZARD, OUTPUT);
  analogWrite(PIN_BRAKE, 0);
  analogWrite(PIN_HAZARD, 0);

  // ready
  digitalWrite(LED_BUILTIN, 1);
}

enum THR_STATES {
  ACCEL,
  NEUTRAL,
  BRAKE
};

// Brake light rules
// - brake from neutral && neutral short : ON
// - any to neutral : ON
// - any to accel: OFF
// - in accell (optional)
//   - if higher: OFF
//   - if lower: ON

void processThr(const uint32_t now, const uint32_t pulseWidth, const bool blinkPulse) {
  static THR_STATES lastThrState = NEUTRAL;
  static uint32_t lastThrStateTs = micros();
  static uint32_t lastThrPulseWidth = 0;
  static uint32_t lastThrPulseWidthTs = micros();
  static bool brakeLight = true;
  static bool throttleMoved = false;

  THR_STATES thrState = NEUTRAL;
  if (pulseWidth <= 1450U) {
    thrState = BRAKE;
  } else if (pulseWidth >= 1550U) {
    thrState = ACCEL;
  }

  if (thrState != NEUTRAL && !throttleMoved) {
    throttleMoved = true;
  }

  if (!throttleMoved) {
    // blink untill throttle moved
    analogWrite(PIN_BRAKE, blinkPulse * 127);
    return;
  }

  // state transition rules
  if (thrState != lastThrState) {
    if (thrState == ACCEL) {
      // transition to accel; accel variation will be handled later
      brakeLight = false;

    } else if (thrState == NEUTRAL) {
      brakeLight = true;

    } else if (thrState == BRAKE) {
      if (lastThrState == NEUTRAL && now - lastThrStateTs < 50000U) { // micros
        brakeLight = true;
      } else if (lastThrState == ACCEL) {
        brakeLight = true;
      } else {
        brakeLight = false;
      }
    }
  }

  // pulse variation rules
  if (pulseWidth != lastThrPulseWidth) {
    // brake on accell going down
    if (thrState == ACCEL && lastThrState == ACCEL) {
      if (pulseWidth < lastThrPulseWidth - THR_BRAKE_TRIGGER_OFFSET) {
        brakeLight = true;
      } else {
        brakeLight = false;
      }
    }
  }

  // after rules are evaluated, update state and pulse width
  if (thrState != lastThrState) {
    lastThrStateTs = now;
    lastThrState = thrState;
  }
  if (pulseWidth != lastThrPulseWidth) {
    lastThrPulseWidthTs = now;
    lastThrPulseWidth = pulseWidth;
  }

  // serves as both position and brake
  uint8_t brakeLightValue = brakeLight ? BRAKE_LIGHT : POSITION_LIGHT;
  analogWrite(PIN_BRAKE, brakeLightValue);
}

void processAux2P(const uint32_t now, const uint32_t pulseWidth, const bool blinkPulse) {
  if (pulseWidth > AUX_CHN_HIGH_THRES) {
    analogWrite(PIN_HAZARD, blinkPulse * 255);
  } else if (pulseWidth < AUX_CHN_LOW_THRES) {
    analogWrite(PIN_HAZARD, 0);
  } else {
    analogWrite(PIN_HAZARD, 0);
  }
}

void loop() {
  static uint8_t badConsecutivePulses = 0;
  static uint8_t blinkPulse = 0;
  static uint8_t errorPulse = 0;
  static bool blinkPattern[] = {1, 1, 1, 0, 0, 0, 0, 0, 0, 0};
  static bool errorPattern[] = {0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1};

  uint32_t now = micros();

  uint32_t thrPulseWidth = pulseIn(PIN_THR, HIGH, 25000);
  if (thrPulseWidth > 0) {
    rcInputs.thrLastPulseWidth = thrPulseWidth;
    rcInputs.thrLastPulseStart = now;
  }
  uint32_t auxPulseWidth = pulseIn(PIN_AUX, HIGH, 25000);
  if (auxPulseWidth > 0) {
    rcInputs.auxLastPulseWidth = auxPulseWidth;
    rcInputs.auxLastPulseStart = now;
  }

  bool validAux = rcInputs.auxLastPulseWidth >= 1000U && rcInputs.auxLastPulseWidth <= 2000U;
  bool freshAux = now - rcInputs.auxLastPulseStart <= 100000U;
  bool validThr = rcInputs.thrLastPulseWidth >= 1000U && rcInputs.thrLastPulseWidth <= 2000U;
  bool freshThr = now - rcInputs.thrLastPulseStart <= 100000U;

  bool validPulse = validAux && validThr;
  bool freshPulse = freshAux && freshThr;

  if (validPulse && freshPulse) {
    badConsecutivePulses = 0;
  } else {
    if (badConsecutivePulses < 255) {
      badConsecutivePulses++;
    }
  }

  uint32_t pulse = now >> 16;
  blinkPulse = blinkPattern[pulse % sizeof(blinkPattern)];
  errorPulse = errorPattern[pulse % sizeof(errorPattern)];

  if (freshPulse && badConsecutivePulses < 10) {
    processThr(now, rcInputs.thrLastPulseWidth, blinkPulse);
    processAux2P(now, rcInputs.auxLastPulseWidth, blinkPulse);
  } else {
    analogWrite(PIN_BRAKE, (1 - errorPulse) * 255);
    analogWrite(PIN_HAZARD, errorPulse * 255);
  }

  // TODO figure out what delay we want (ms)
  delay(10);
}
