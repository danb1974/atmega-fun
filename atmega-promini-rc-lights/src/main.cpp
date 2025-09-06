#include <Arduino.h>


// RC channel pins
#define PIN_THR 2
#define INT_THR 0
#define PIN_AUX 3
#define INT_AUX 1

// output pins to lights (pwm capable)
#define PIN_BRAKE 10
#define PIN_HAZARD 11

// RC channel data
volatile uint32_t thrLastPulseStart = 0;
volatile uint32_t thrLastPulseEnd = 0;
volatile uint32_t thrLastPulseWidth = 0;
volatile uint32_t auxLastPulseStart = 0;
volatile uint32_t auxLastPulseEnd = 0;
volatile uint32_t auxLastPulseWidth = 0;

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

void thrInterrupt() {
  uint32_t now = micros();
  uint8_t state = digitalRead(PIN_THR);

  if (state == 1) {
    thrLastPulseStart = now;
    return;
  }

  thrLastPulseEnd = now;
  thrLastPulseWidth = now - thrLastPulseStart;
}

void auxInterrupt() {
  uint32_t now = micros();
  uint8_t state = digitalRead(PIN_AUX);

  if (state == 1) {
    auxLastPulseStart = now;
    return;
  }
  
  thrLastPulseEnd = now;
  auxLastPulseWidth = now - auxLastPulseStart;
}

//

void setup() {
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, 0);

  pinMode(PIN_THR, INPUT);
  pinMode(PIN_AUX, INPUT);
  pinMode(PIN_BRAKE, OUTPUT);
  pinMode(PIN_HAZARD, OUTPUT);

  attachInterrupt(INT_THR, thrInterrupt, CHANGE);
  attachInterrupt(INT_AUX, auxInterrupt, CHANGE);

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

void processThr(bool blinkPulse) {
  static THR_STATES lastThrState = NEUTRAL;
  static uint32_t lastThrStateTs = micros();
  static uint32_t lastThrPulseWidth = 0;
  static uint32_t lastThrPulseWidthTs = micros();
  static bool brakeLight = true;
  static bool throttleMoved = false;

  uint32_t now = micros();
  uint32_t pulseWidth = thrLastPulseWidth;

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
    digitalWrite(PIN_BRAKE, blinkPulse);
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

void processAux2P(bool blinkPulse) {
  uint32_t pulseWidth = auxLastPulseWidth;

  if (pulseWidth > AUX_CHN_HIGH_THRES) {
    digitalWrite(PIN_HAZARD, blinkPulse);
  } else if (pulseWidth < AUX_CHN_LOW_THRES) {
    digitalWrite(PIN_HAZARD, 0);
  } else {
    digitalWrite(PIN_HAZARD, 0);
  }
}

void loop() {
  static uint8_t blinkPulse = 0;
  static uint8_t errorPulse = 0;
  static bool blinkPattern[] = {1, 1, 1, 0, 0, 0, 0, 0, 0, 0};
  static bool errorPattern[] = {0, 1};

  uint32_t now = micros();
  uint32_t pulse = now >> 16;

  blinkPulse = blinkPattern[pulse % sizeof(blinkPattern)];
  errorPulse = errorPattern[pulse % sizeof(errorPattern)];

  bool validThr = thrLastPulseWidth >= MIN_VALID_PULSE && thrLastPulseWidth <= MAX_VALID_PULSE
    //&& now - thrLastPulseStart < 3000000;
    && thrLastPulseStart > 0;
  if (validThr) {
    processThr(blinkPulse);
  } else {
    digitalWrite(PIN_BRAKE, 1 - errorPulse);
  }

  bool validAux = auxLastPulseWidth >= MIN_VALID_PULSE && auxLastPulseWidth <= MAX_VALID_PULSE
    //&& now - auxLastPulseStart < 3000000;
    && auxLastPulseStart > 0;
  if (validAux) {
    processAux2P(blinkPulse);
  } else {
    digitalWrite(PIN_HAZARD, errorPulse);
  }

  // TODO figure out what delay we want (ms)
  delay(20);
}
