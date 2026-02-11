#include "EY_Sensors.h"
#include "EY_Config.h"
#include "EY_Mqtt.h"

// ------------------------------------------------------------
// Runtime state for each sensor
// ------------------------------------------------------------
static SensorState s_states[SENSOR_COUNT];

// ------------------------------------------------------------
// Internal helpers
// ------------------------------------------------------------

static bool readPresent(const SensorDef& def) {
  int val = digitalRead(def.pin);
  return (def.presentWhen == PresentWhen::HIGH_LEVEL) ? (val == HIGH) : (val == LOW);
}

static bool evaluateSolveCondition() {
  switch (SOLVE_MODE) {
    case SolveMode::ANY:
      for (uint8_t i = 0; i < SENSOR_COUNT; i++) {
        if (s_states[i].present) return true;
      }
      return false;

    case SolveMode::ALL:
      for (uint8_t i = 0; i < SENSOR_COUNT; i++) {
        if (!s_states[i].present) return false;
      }
      return (SENSOR_COUNT > 0);  // true only if all present (and at least one sensor exists)

    default:
      return false;
  }
}

// ------------------------------------------------------------
// Public API
// ------------------------------------------------------------

void EY_Sensors_Begin() {
  for (uint8_t i = 0; i < SENSOR_COUNT; i++) {
    pinMode(SENSORS[i].pin, INPUT_PULLUP);

    // Initialize state
    s_states[i].armed = !SENSORS[i].needsArming;  // Pre-armed if arming not required
    s_states[i].present = false;
    s_states[i].eventSent = false;
    s_states[i].forceLocked = false;
    s_states[i].lastRaw = readPresent(SENSORS[i]);
    s_states[i].lastChangeMs = millis();
  }
}

bool EY_Sensors_Tick() {
  bool anyTransition = false;

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
      Serial.print("[Sensor] ");
      Serial.print(def.id);
      Serial.println(" -> PRESENT");

      // Publish one-shot event (once per reset)
      if (!state.eventSent) {
        EY_PublishEvent(def.actionEvent, EY_MQTT::SRC_PLAYER);
        state.eventSent = true;
      }
    }

    // Detect transition to not present (for debugging)
    if (!state.present && wasPresent) {
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
    s_states[i].eventSent = false;
    s_states[i].forceLocked = false;
    s_states[i].lastRaw = readPresent(SENSORS[i]);
    s_states[i].lastChangeMs = millis();
  }
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

bool EY_Sensors_ForceTrigger(const char* sensorId) {
  if (!sensorId) return false;

  for (uint8_t i = 0; i < SENSOR_COUNT; i++) {
    if (strcmp(SENSORS[i].id, sensorId) == 0) {
      SensorState& state = s_states[i];

      // Force the sensor to triggered state (locked until reset)
      state.armed = true;
      state.present = true;
      state.forceLocked = true;

      Serial.print("[Sensor] ");
      Serial.print(sensorId);
      Serial.println(" -> FORCE TRIGGERED (GM)");

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
