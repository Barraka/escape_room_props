#pragma once
// Wiegand keypad reader module
// Supports 4-bit (key-by-key), 8-bit (key + complement), and 26-bit (full code) frames.
// Feature guard: #define HAS_WIEGAND in prop config

#include <Arduino.h>

typedef void (*WiegandCodeCallback)(const char* code);

void EY_Wiegand_Begin(uint8_t pinD0, uint8_t pinD1);
void EY_Wiegand_Tick();   // Call every loop — checks for completed frames
void EY_Wiegand_Reset();  // Clear accumulated code buffer

// Register a callback fired when a non-empty code is submitted with '#'.
// Called in addition to the MQTT "code_entered" event. Pass NULL to unregister.
void EY_Wiegand_OnCode(WiegandCodeCallback cb);
