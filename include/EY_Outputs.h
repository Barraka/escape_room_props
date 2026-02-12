#pragma once

#include "EY_Types.h"  // For OutputDef, OutputPinState

// ============================================================
// Output System API (Maglocks, Relays, etc.)
// ============================================================

// Initialize all output pins (call once in setup)
// Sets all pins to INACTIVE (fail-safe: maglock unlocked)
void EY_Outputs_Begin();

// Activate all outputs (lock maglocks) — called on GM "arm" command
void EY_Outputs_Arm();

// Deactivate all outputs (unlock maglocks) — called on solve/force_solve
void EY_Outputs_Release();

// Re-arm all outputs (re-lock maglocks) — called on reset
void EY_Outputs_Reset();

// Get individual output state
OutputPinState EY_Outputs_GetState(uint8_t index);

// Get output count
uint8_t EY_Outputs_GetCount();
