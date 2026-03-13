#pragma once
// Wiegand keypad reader module
// Supports 4-bit (key-by-key), 8-bit (key + complement), and 26-bit (full code) frames.
// Feature guard: #define HAS_WIEGAND in prop config

#include <Arduino.h>

void EY_Wiegand_Begin(uint8_t pinD0, uint8_t pinD1);
void EY_Wiegand_Tick();   // Call every loop — checks for completed frames
void EY_Wiegand_Reset();  // Clear accumulated code buffer
