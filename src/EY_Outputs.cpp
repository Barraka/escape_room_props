#include "EY_Outputs.h"
#include "EY_Config.h"

#include <Arduino.h>

// Runtime state array (handle zero-output case for props without outputs)
static OutputPinState s_states[OUTPUT_COUNT > 0 ? OUTPUT_COUNT : 1];

// Write a pin respecting activeLow polarity
static void writePin(uint8_t index, bool activate) {
  bool level = activate;
  if (OUTPUTS[index].activeLow) level = !level;
  digitalWrite(OUTPUTS[index].pin, level ? HIGH : LOW);
}

void EY_Outputs_Begin() {
  for (uint8_t i = 0; i < OUTPUT_COUNT; i++) {
    pinMode(OUTPUTS[i].pin, OUTPUT);
    s_states[i] = OutputPinState::INACTIVE;
    writePin(i, false);  // Fail-safe: deactivated on boot
    Serial.print("[Outputs] Pin ");
    Serial.print(OUTPUTS[i].pin);
    Serial.print(" (");
    Serial.print(OUTPUTS[i].id);
    Serial.println(") → INACTIVE");
  }
}

void EY_Outputs_Arm() {
  for (uint8_t i = 0; i < OUTPUT_COUNT; i++) {
    s_states[i] = OutputPinState::ARMED;
    writePin(i, true);  // Activate (lock maglock)
    Serial.print("[Outputs] ");
    Serial.print(OUTPUTS[i].id);
    Serial.println(" → ARMED");
  }
}

void EY_Outputs_Release() {
  for (uint8_t i = 0; i < OUTPUT_COUNT; i++) {
    s_states[i] = OutputPinState::RELEASED;
    writePin(i, false);  // Deactivate (unlock maglock)
    Serial.print("[Outputs] ");
    Serial.print(OUTPUTS[i].id);
    Serial.println(" → RELEASED");
  }
}

void EY_Outputs_Reset() {
  // Reset re-arms outputs (re-lock maglocks for next session)
  for (uint8_t i = 0; i < OUTPUT_COUNT; i++) {
    s_states[i] = OutputPinState::ARMED;
    writePin(i, true);  // Re-activate (re-lock maglock)
    Serial.print("[Outputs] ");
    Serial.print(OUTPUTS[i].id);
    Serial.println(" → ARMED (reset)");
  }
}

OutputPinState EY_Outputs_GetState(uint8_t index) {
  if (index >= OUTPUT_COUNT) return OutputPinState::INACTIVE;
  return s_states[index];
}

uint8_t EY_Outputs_GetCount() {
  return OUTPUT_COUNT;
}
