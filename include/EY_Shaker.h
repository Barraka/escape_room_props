#pragma once

#include <Arduino.h>

// ============================================================
// Shaker Module API
// ============================================================
// Receives ESP-NOW packets from an ESP32-C3 + MPU6050 transmitter
// inside the shaker. Accumulates active time, decays when idle,
// solves at target.

// Initialize the shaker module (call once in setup)
void EY_Shaker_Begin(uint8_t rxPin);

// Poll edge counts, accumulate/decay shake time.
// Returns true when accumulated time reaches SHAKE_TARGET_MS (solved).
// Call every loop iteration (non-blocking).
bool EY_Shaker_Tick();

// Reset accumulated shake time to zero (call on prop reset)
void EY_Shaker_Reset();

// Get current shake progress as 0-100 percentage
uint8_t EY_Shaker_GetProgress();
