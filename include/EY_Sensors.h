#pragma once

#include "EY_Types.h"  // For SensorDef, SensorState, PresentWhen, SolveMode

// ============================================================
// Sensor System API
// ============================================================

// Initialize all sensors (call once in setup)
void EY_Sensors_Begin();

// Poll all sensors, publish events on transitions, return true if solve condition met
// Call every loop iteration (non-blocking)
bool EY_Sensors_Tick();

// Reset all sensor states (call on prop reset)
void EY_Sensors_Reset();

// Check if solve condition is currently met (without side effects)
bool EY_Sensors_IsSolved();

// Get individual sensor state (for debugging/status)
const SensorState* EY_Sensors_GetState(uint8_t index);

// Get sensor count
uint8_t EY_Sensors_GetCount();

// Force-trigger a sensor remotely (for GM override)
// Returns true if sensor found and triggered
bool EY_Sensors_ForceTrigger(const char* sensorId);

// SEQUENCE mode: how many sequence steps have been completed so far.
// Returns 0 in other solve modes. Used by status publisher to mark
// completed steps as "triggered" for the GM dashboard.
uint8_t EY_Sensors_GetSequenceIndex();

// True if the most recent EY_Sensors_Tick() saw at least one sensor change
// present-state (press or release). Used by main.cpp to republish status
// so the GM dashboard sees per-sensor triggers in real time.
bool EY_Sensors_StateChangedThisTick();
