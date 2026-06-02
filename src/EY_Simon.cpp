#include "EY_Config.h"  // Must be first — brings in PROP_CONFIG which may define HAS_SIMON

#ifdef HAS_SIMON

#include "EY_Simon.h"
#include "EY_Mqtt.h"
#include <Arduino.h>

// ---- Per-button state ----
struct SimonButton {
  // Game
  bool locked;
  bool ledOn;
  unsigned long nextBlinkAt;
  unsigned long blinkOffAt;

  // Debounce
  bool lastRead;
  unsigned long lastChangeMs;
  bool stablePressed;
};

static SimonButton s_btns[SIMON_COUNT];
static bool s_active = false;
static uint8_t s_lockedCount = 0;

// ---- Public API ----

void EY_Simon_Begin() {
  randomSeed(analogRead(0) ^ micros());

  for (uint8_t i = 0; i < SIMON_COUNT; i++) {
    // Input-only pins (34-39) have no internal pull-up — need external 10kΩ
    if (SIMON_BTN_PINS[i] < 34) {
      pinMode(SIMON_BTN_PINS[i], INPUT_PULLUP);
    } else {
      pinMode(SIMON_BTN_PINS[i], INPUT);
    }

    pinMode(SIMON_LED_PINS[i], OUTPUT);
    digitalWrite(SIMON_LED_PINS[i], LOW);

    s_btns[i] = {};
  }

  s_active = false;
  s_lockedCount = 0;

  // Boot LED test — blink all LEDs 3 times to confirm wiring
  Serial.println("[Simon] LED test...");
  for (int t = 0; t < 3; t++) {
    for (uint8_t i = 0; i < SIMON_COUNT; i++) digitalWrite(SIMON_LED_PINS[i], HIGH);
    delay(300);
    for (uint8_t i = 0; i < SIMON_COUNT; i++) digitalWrite(SIMON_LED_PINS[i], LOW);
    delay(300);
  }
  Serial.println("[Simon] LED test done");

  // Auto-activate on boot
  EY_Simon_Activate();
}

void EY_Simon_Activate() {
  s_active = true;
  s_lockedCount = 0;

  unsigned long now = millis();
  for (uint8_t i = 0; i < SIMON_COUNT; i++) {
    SimonButton& btn = s_btns[i];
    btn.locked = false;
    btn.ledOn = false;
    btn.nextBlinkAt = now + random(SIMON_BLINK_MIN_MS, SIMON_BLINK_MAX_MS);
    btn.blinkOffAt = 0;
    btn.lastRead = false;
    btn.lastChangeMs = now;
    btn.stablePressed = false;

    digitalWrite(SIMON_LED_PINS[i], LOW);
  }

  Serial.println("[Simon] Game activated");
}

bool EY_Simon_Tick() {
  if (!s_active) return false;

  unsigned long now = millis();

  for (uint8_t i = 0; i < SIMON_COUNT; i++) {
    SimonButton& btn = s_btns[i];

    if (btn.locked) continue;

    // ---- LED blinking ----
    if (!btn.ledOn && now >= btn.nextBlinkAt) {
      btn.ledOn = true;
      btn.blinkOffAt = now + SIMON_BLINK_DURATION_MS;
      digitalWrite(SIMON_LED_PINS[i], HIGH);
    }

    if (btn.ledOn && now >= btn.blinkOffAt) {
      btn.ledOn = false;
      btn.nextBlinkAt = now + random(SIMON_BLINK_MIN_MS, SIMON_BLINK_MAX_MS);
      digitalWrite(SIMON_LED_PINS[i], LOW);
    }

    // ---- Button debounce ----
    bool raw = (digitalRead(SIMON_BTN_PINS[i]) == LOW);
    if (raw != btn.lastRead) {
      btn.lastRead = raw;
      btn.lastChangeMs = now;
    }

    if (now - btn.lastChangeMs < SIMON_DEBOUNCE_MS) continue;

    bool wasPressed = btn.stablePressed;
    btn.stablePressed = btn.lastRead;

    // ---- Press detection (rising edge) ----
    if (btn.stablePressed && !wasPressed) {
      if (btn.ledOn) {
        // Correct — lock this button, LED stays on
        btn.locked = true;
        digitalWrite(SIMON_LED_PINS[i], HIGH);
        s_lockedCount++;

        Serial.print("[Simon] Button ");
        Serial.print(i + 1);
        Serial.print(" LOCKED (");
        Serial.print(s_lockedCount);
        Serial.print("/");
        Serial.print(SIMON_COUNT);
        Serial.println(")");

        EY_PublishEvent("button_locked", EY_MQTT::SRC_PLAYER);
      }
      // If LED not on: nothing happens
    }
  }

  return (s_lockedCount >= SIMON_COUNT);
}

void EY_Simon_Reset() {
  s_active = false;
  s_lockedCount = 0;

  for (uint8_t i = 0; i < SIMON_COUNT; i++) {
    s_btns[i] = {};
    digitalWrite(SIMON_LED_PINS[i], LOW);
  }

  Serial.println("[Simon] Reset");

  // Auto-restart the game
  EY_Simon_Activate();
}

void EY_Simon_ForceSolve() {
  s_active = true;
  s_lockedCount = SIMON_COUNT;

  for (uint8_t i = 0; i < SIMON_COUNT; i++) {
    s_btns[i].locked = true;
    s_btns[i].ledOn = true;
    digitalWrite(SIMON_LED_PINS[i], HIGH);
  }

  Serial.println("[Simon] Force solved");
}

uint8_t EY_Simon_GetProgress() {
  return (uint8_t)((s_lockedCount * 100) / SIMON_COUNT);
}

uint8_t EY_Simon_GetLockedCount() {
  return s_lockedCount;
}

#ifdef SIMON_TEST_MODE
// ---- Dev wiring test: all LEDs solid on; press a button → its LED blinks 3x → solid ----
struct SimonTestBtn {
  bool lastRead;
  unsigned long lastChangeMs;
  bool stablePressed;
  bool blinking;
  uint8_t phasesLeft;   // 6 = off,on,off,on,off,on → ends solid on
  bool ledOn;
  unsigned long nextToggleAt;
};
static SimonTestBtn s_test[SIMON_COUNT];
static constexpr unsigned long SIMON_TEST_BLINK_MS = 150;

void EY_Simon_TestBegin() {
  unsigned long now = millis();
  for (uint8_t i = 0; i < SIMON_COUNT; i++) {
    s_test[i] = {};
    s_test[i].ledOn = true;
    s_test[i].lastChangeMs = now;
    digitalWrite(SIMON_LED_PINS[i], HIGH);  // solid on
  }
  Serial.println("[Simon TEST] All LEDs on. Press a button -> 3 blinks -> back on.");
}

void EY_Simon_TestTick() {
  unsigned long now = millis();

  for (uint8_t i = 0; i < SIMON_COUNT; i++) {
    SimonTestBtn& b = s_test[i];

    // ---- non-blocking blink animation ----
    if (b.blinking && now >= b.nextToggleAt) {
      b.phasesLeft--;
      if (b.phasesLeft == 0) {
        b.blinking = false;
        b.ledOn = true;
        digitalWrite(SIMON_LED_PINS[i], HIGH);  // back to solid on
      } else {
        b.ledOn = !b.ledOn;
        digitalWrite(SIMON_LED_PINS[i], b.ledOn ? HIGH : LOW);
        b.nextToggleAt = now + SIMON_TEST_BLINK_MS;
      }
    }

    // ---- debounced press detection ----
    bool raw = (digitalRead(SIMON_BTN_PINS[i]) == LOW);
    if (raw != b.lastRead) {
      b.lastRead = raw;
      b.lastChangeMs = now;
    }
    if (now - b.lastChangeMs < SIMON_DEBOUNCE_MS) continue;

    bool wasPressed = b.stablePressed;
    b.stablePressed = b.lastRead;

    // rising edge → start the 3-blink sequence (ignored if already blinking)
    if (b.stablePressed && !wasPressed && !b.blinking) {
      b.blinking = true;
      b.phasesLeft = 6;
      b.ledOn = false;
      digitalWrite(SIMON_LED_PINS[i], LOW);
      b.nextToggleAt = now + SIMON_TEST_BLINK_MS;
      Serial.print("[Simon TEST] Button ");
      Serial.print(i + 1);
      Serial.println(" pressed");
    }
  }
}
#endif // SIMON_TEST_MODE

#endif // HAS_SIMON
