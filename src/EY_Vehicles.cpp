#include "EY_Config.h"  // Must be first — brings in PROP_CONFIG which may define HAS_VEHICLES

#ifdef HAS_VEHICLES

#include "EY_Vehicles.h"
#include "EY_Mqtt.h"
#include <Arduino.h>

static const char* POS_NAME[3] = { "LEFT", "CENTRE", "RIGHT" };

// ---- Per-switch state ----
struct VehicleSwitch {
  uint8_t position;        // last debounced position (VEH_LEFT/CENTRE/RIGHT, or 0xFF = invalid)
  uint8_t rawPosition;     // last raw reading
  unsigned long lastChangeMs;
};

static VehicleSwitch s_sw[VEHICLE_COUNT];
static bool s_active = false;
static uint8_t s_correctCount = 0;

// Read the live position of switch i from its two GPIOs.
static uint8_t readPosition(uint8_t i) {
  bool leftLow  = (digitalRead(VEHICLE_PINS[i][0]) == LOW);
  bool rightLow = (digitalRead(VEHICLE_PINS[i][1]) == LOW);

  if (leftLow && !rightLow)  return VEH_LEFT;
  if (!leftLow && rightLow)  return VEH_RIGHT;
  if (!leftLow && !rightLow) return VEH_CENTRE;
  return 0xFF;  // both LOW — wiring fault, never matches a target
}

static void recomputeCorrect() {
  s_correctCount = 0;
  for (uint8_t i = 0; i < VEHICLE_COUNT; i++) {
    if (s_sw[i].position == VEHICLE_TARGET[i]) s_correctCount++;
  }
}

// ---- Public API ----

void EY_Vehicles_Begin() {
  for (uint8_t i = 0; i < VEHICLE_COUNT; i++) {
    pinMode(VEHICLE_PINS[i][0], INPUT_PULLUP);
    pinMode(VEHICLE_PINS[i][1], INPUT_PULLUP);
    s_sw[i] = {};
    s_sw[i].position    = readPosition(i);
    s_sw[i].rawPosition = s_sw[i].position;
    s_sw[i].lastChangeMs = millis();
  }
  recomputeCorrect();
  s_active = false;

  Serial.print("[Vehicles] ");
  Serial.print(VEHICLE_COUNT);
  Serial.println(" switches initialised");

  // Auto-activate on boot (no Room Controller gating needed — combination
  // puzzle is always live, like Simon).
  EY_Vehicles_Activate();
}

void EY_Vehicles_Activate() {
  s_active = true;
  unsigned long now = millis();
  for (uint8_t i = 0; i < VEHICLE_COUNT; i++) {
    s_sw[i].position    = readPosition(i);
    s_sw[i].rawPosition = s_sw[i].position;
    s_sw[i].lastChangeMs = now;
  }
  recomputeCorrect();
  Serial.println("[Vehicles] Activated");
}

bool EY_Vehicles_Tick() {
  if (!s_active) return false;

  unsigned long now = millis();

  for (uint8_t i = 0; i < VEHICLE_COUNT; i++) {
    VehicleSwitch& sw = s_sw[i];

    uint8_t raw = readPosition(i);

    // Debounce: only accept a position after it's been stable for DEBOUNCE_MS
    if (raw != sw.rawPosition) {
      sw.rawPosition = raw;
      sw.lastChangeMs = now;
    }
    if (now - sw.lastChangeMs < VEHICLE_DEBOUNCE_MS) continue;

    if (raw != sw.position) {
      sw.position = raw;
      Serial.print("[Vehicles] Switch ");
      Serial.print(i + 1);
      Serial.print(" -> ");
      Serial.print(raw == 0xFF ? "INVALID" : POS_NAME[raw]);
      Serial.print(raw != 0xFF && raw == VEHICLE_TARGET[i] ? " (correct)" : "");
      Serial.println();
      recomputeCorrect();
    }
  }

  return (s_correctCount >= VEHICLE_COUNT);
}

void EY_Vehicles_Reset() {
  // Combination puzzle has no latched per-switch state to clear — just
  // re-read the live positions and re-activate.
  EY_Vehicles_Activate();
  Serial.println("[Vehicles] Reset");
}

void EY_Vehicles_ForceSolve() {
  s_active = true;
  s_correctCount = VEHICLE_COUNT;
  for (uint8_t i = 0; i < VEHICLE_COUNT; i++) {
    s_sw[i].position = VEHICLE_TARGET[i];
  }
  Serial.println("[Vehicles] Force solved");
}

uint8_t EY_Vehicles_GetProgress() {
  return (uint8_t)((s_correctCount * 100) / VEHICLE_COUNT);
}

uint8_t EY_Vehicles_GetCorrectCount() {
  return s_correctCount;
}

#ifdef VEHICLES_TEST_MODE
// ---- Dev wiring test: print every switch's position whenever it changes ----
static uint8_t s_testLast[VEHICLE_COUNT];

void EY_Vehicles_TestBegin() {
  for (uint8_t i = 0; i < VEHICLE_COUNT; i++) {
    pinMode(VEHICLE_PINS[i][0], INPUT_PULLUP);
    pinMode(VEHICLE_PINS[i][1], INPUT_PULLUP);
    s_testLast[i] = 0xFE;  // force a first-print for every switch
  }
  Serial.println("[Vehicles TEST] Flip each switch — its position prints here.");
  Serial.println("[Vehicles TEST] Use this to map switches and discover the combination.");
}

void EY_Vehicles_TestTick() {
  for (uint8_t i = 0; i < VEHICLE_COUNT; i++) {
    uint8_t pos = readPosition(i);
    if (pos != s_testLast[i]) {
      s_testLast[i] = pos;
      Serial.print("[Vehicles TEST] Switch ");
      Serial.print(i + 1);
      Serial.print(" (pins ");
      Serial.print(VEHICLE_PINS[i][0]);
      Serial.print("/");
      Serial.print(VEHICLE_PINS[i][1]);
      Serial.print(") = ");
      Serial.println(pos == 0xFF ? "INVALID (both LOW)" : POS_NAME[pos]);
    }
  }
  delay(20);
}
#endif // VEHICLES_TEST_MODE

#endif // HAS_VEHICLES
