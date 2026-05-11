#pragma once
// PIR motion sensor module (HC-SR501)
// Publishes a "motion_detected" event on rising edge.
// Feature guard: #define HAS_PIR in prop config

#include <Arduino.h>

void EY_PIR_Begin(uint8_t pin);
void EY_PIR_Tick();
