#include "EY_Sensors.h"
#include "EY_Config.h"
#include "EY_Mqtt.h"

// ------------------------------------------------------------
// Runtime state for each sensor
// ------------------------------------------------------------
static SensorState s_states[SENSOR_COUNT];

// SEQUENCE mode: index of the next sensor expected to be pressed.
// Solved when it reaches s_solveSensorCount (excludes decoratives).
static uint8_t s_sequenceIndex = 0;

// Count of non-decorative sensors (= solve target for SEQUENCE/ALL).
// Computed once in EY_Sensors_Begin.
static uint8_t s_solveSensorCount = 0;

// True if EY_Sensors_Tick observed any sensor present-state change this tick
// (press or release). Used by main.cpp to republish status on demand.
static bool s_anyStateChangeThisTick = false;

// ------------------------------------------------------------
// Internal helpers
// ------------------------------------------------------------

static bool readPresent(const SensorDef& def) {
  int val = digitalRead(def.pin);
  return (def.presentWhen == PresentWhen::HIGH_LEVEL) ? (val == HIGH) : (val == LOW);
}

// Effective presence for solve/status purposes. Latching sensors (momentary-pulse
// readers) report their latched state; everything else reports live presence.
static bool effectivePresent(uint8_t i) {
  return SENSORS[i].latching ? s_states[i].latched : s_states[i].present;
}

static void handleSequencePress(uint8_t i) {
  if (i == s_sequenceIndex) {
    s_sequenceIndex++;
    EY_PublishEvent(SENSORS[i].actionEvent, EY_MQTT::SRC_PLAYER);
    Serial.print("[Sequence] OK ");
    Serial.print(s_sequenceIndex);
    Serial.print("/");
    Serial.println(s_solveSensorCount);
  } else {
    Serial.print("[Sequence] Wrong press (expected idx ");
    Serial.print(s_sequenceIndex);
    Serial.print(", got ");
    Serial.print(i);
    Serial.println(") — sequence reset");
    s_sequenceIndex = 0;
  }
}

static bool evaluateSolveCondition() {
  switch (SOLVE_MODE) {
    case SolveMode::ANY:
      for (uint8_t i = 0; i < SENSOR_COUNT; i++) {
        if (SENSORS[i].decorative) continue;
        if (effectivePresent(i)) return true;
      }
      return false;

    case SolveMode::ALL:
      for (uint8_t i = 0; i < SENSOR_COUNT; i++) {
        if (SENSORS[i].decorative) continue;
        if (!effectivePresent(i)) return false;
      }
      return (s_solveSensorCount > 0);

    case SolveMode::SEQUENCE:
      return (s_solveSensorCount > 0) && (s_sequenceIndex >= s_solveSensorCount);

    default:
      return false;
  }
}

// ------------------------------------------------------------
// Public API
// ------------------------------------------------------------

void EY_Sensors_Begin() {
  s_solveSensorCount = 0;
  for (uint8_t i = 0; i < SENSOR_COUNT; i++) {
    pinMode(SENSORS[i].pin, INPUT_PULLUP);

    // Initialize state
    s_states[i].armed = !SENSORS[i].needsArming;  // Pre-armed if arming not required
    s_states[i].present = false;
    s_states[i].latched = false;
    s_states[i].eventSent = false;
    s_states[i].forceLocked = false;
    s_states[i].lastRaw = readPresent(SENSORS[i]);
    s_states[i].lastChangeMs = millis();

    if (!SENSORS[i].decorative) s_solveSensorCount++;
  }
}

bool EY_Sensors_Tick() {
  bool anyTransition = false;
  s_anyStateChangeThisTick = false;

  for (uint8_t i = 0; i < SENSOR_COUNT; i++) {
    const SensorDef& def = SENSORS[i];
    SensorState& state = s_states[i];

    // Skip sensors that were force-triggered by GM (preserved until reset)
    if (state.present && state.forceLocked) continue;

    bool raw = readPresent(def);

    // Debounce: track raw GPIO changes, only accept after stable for DEBOUNCE_MS
    if (raw != state.lastRaw) {
      state.lastRaw = raw;
      state.lastChangeMs = millis();
    }
    if (millis() - state.lastChangeMs < DEBOUNCE_MS) continue;

    // Arming logic: must see "not present" before "present" counts
    if (def.needsArming && !state.armed) {
      if (!raw) {
        state.armed = true;
        Serial.print("[Sensor] ");
        Serial.print(def.id);
        Serial.println(" armed");
      }
      // Not armed yet, so presence doesn't count
      continue;
    }

    bool wasPresent = state.present;
    state.present = raw;

    // Detect transition to present
    if (state.present && !wasPresent) {
      anyTransition = true;
      s_anyStateChangeThisTick = true;
      if (def.latching) state.latched = true;  // momentary readers: latch until reset
      Serial.print("[Sensor] ");
      Serial.print(def.id);
      Serial.println(" -> PRESENT");

      if (def.decorative) {
        // Decorative: publish on every press (e.g. sound feedback).
        EY_PublishEvent(def.actionEvent, EY_MQTT::SRC_PLAYER);
        // In SEQUENCE mode a decorative button is a "decoy" / wrong button →
        // pressing it resets progress, just like an out-of-order real press,
        // so the dashboard clears the green steps.
        if (SOLVE_MODE == SolveMode::SEQUENCE && s_sequenceIndex > 0) {
          Serial.println("[Sequence] Decoy pressed — sequence reset");
          s_sequenceIndex = 0;
        }
      } else if (SOLVE_MODE == SolveMode::SEQUENCE) {
        handleSequencePress(i);
      } else if (!state.eventSent) {
        // ANY/ALL: publish one-shot event (once per reset)
        EY_PublishEvent(def.actionEvent, EY_MQTT::SRC_PLAYER);
        state.eventSent = true;
      }
    }

    // Detect transition to not present
    if (!state.present && wasPresent) {
      s_anyStateChangeThisTick = true;
      Serial.print("[Sensor] ");
      Serial.print(def.id);
      Serial.println(" -> ABSENT");
    }
  }

  return evaluateSolveCondition();
}

void EY_Sensors_Reset() {
  for (uint8_t i = 0; i < SENSOR_COUNT; i++) {
    s_states[i].armed = !SENSORS[i].needsArming;
    s_states[i].present = false;
    s_states[i].latched = false;
    s_states[i].eventSent = false;
    s_states[i].forceLocked = false;
    s_states[i].lastRaw = readPresent(SENSORS[i]);
    s_states[i].lastChangeMs = millis();
  }
  s_sequenceIndex = 0;
  Serial.println("[Sensor] All sensors reset");
}

bool EY_Sensors_IsSolved() {
  return evaluateSolveCondition();
}

const SensorState* EY_Sensors_GetState(uint8_t index) {
  if (index >= SENSOR_COUNT) return nullptr;
  return &s_states[index];
}

uint8_t EY_Sensors_GetCount() {
  return SENSOR_COUNT;
}

uint8_t EY_Sensors_GetSequenceIndex() {
  return (SOLVE_MODE == SolveMode::SEQUENCE) ? s_sequenceIndex : 0;
}

bool EY_Sensors_StateChangedThisTick() {
  return s_anyStateChangeThisTick;
}

bool EY_Sensors_ForceTrigger(const char* sensorId) {
  if (!sensorId) return false;

  for (uint8_t i = 0; i < SENSOR_COUNT; i++) {
    if (strcmp(SENSORS[i].id, sensorId) == 0) {
      SensorState& state = s_states[i];

      // Force the sensor to triggered state (locked until reset)
      state.armed = true;
      state.present = true;
      state.latched = true;
      state.forceLocked = true;

      Serial.print("[Sensor] ");
      Serial.print(sensorId);
      Serial.println(" -> FORCE TRIGGERED (GM)");

      // In SEQUENCE mode, GM force-trigger advances progress by one step.
      // Force-triggering every sensor reaches SENSOR_COUNT -> solved.
      if (SOLVE_MODE == SolveMode::SEQUENCE && s_sequenceIndex < SENSOR_COUNT) {
        s_sequenceIndex++;
      }

      // Publish event if not already sent
      if (!state.eventSent) {
        EY_PublishEvent(SENSORS[i].actionEvent, EY_MQTT::SRC_GM);
        state.eventSent = true;
      }

      return true;
    }
  }

  Serial.print("[Sensor] Unknown sensorId: ");
  Serial.println(sensorId);
  return false;
}
