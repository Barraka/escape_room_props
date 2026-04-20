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

#endif // HAS_SIMON
