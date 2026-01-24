#pragma once

#include <Arduino.h>

// ============================================================
// Shared Type Definitions
// ============================================================
// This header contains types used across multiple modules.
// It has no dependencies on other project headers.

// When is the sensor considered "present"?
enum class PresentWhen : uint8_t {
  HIGH_LEVEL,  // present when digitalRead() == HIGH
  LOW_LEVEL,   // present when digitalRead() == LOW (e.g., reed switch to GND)
};

// How do we determine "solved"?
enum class SolveMode : uint8_t {
  ANY,   // Solved when ANY sensor becomes present
  ALL,   // Solved when ALL sensors are present simultaneously
  // Future extensions:
  // SEQUENCE,  // Sensors must trigger in defined order
  // CUSTOM,    // Prop-specific logic via callback
};

// Sensor definition (compile-time configuration)
struct SensorDef {
  const char* id;            // Stable identifier, e.g., "rfid1", "magnet_left"
  uint8_t pin;               // GPIO pin number
  PresentWhen presentWhen;   // Polarity rule
  const char* actionEvent;   // Event action string, e.g., "rfid_present"
  bool needsArming;          // Must see "not present" at least once before "present" counts
};

// Sensor runtime state
struct SensorState {
  bool armed;      // Only relevant if needsArming == true
  bool present;    // Current computed presence
  bool eventSent;  // One-shot event sent this session (reset clears this)
};
