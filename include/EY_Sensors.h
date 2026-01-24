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
