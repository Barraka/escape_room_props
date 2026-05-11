#pragma once
// 433MHz OOK transmitter module (rc-switch).
// Feature guard: #define HAS_RF433 in prop config.
//
// Protocol/pulse/bits/repeats are read from the prop header
// (RF433_PROTOCOL, RF433_PULSE_LENGTH, RF433_BITS, RF433_REPEATS).

#include <Arduino.h>

void EY_RF433_Begin(uint8_t pin);
void EY_RF433_Send(unsigned long code);
