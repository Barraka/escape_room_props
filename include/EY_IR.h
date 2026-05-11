#pragma once
// IR receiver module (NEC 32-bit decoding via IRremoteESP8266).
// Feature guard: #define HAS_IR in prop config.
//
// Maps received NEC codes to digits (or a clear key) using the
// IR_KEY_MAP table defined in the prop header, then publishes MQTT
// events so the Room Controller can validate the code.

#include <Arduino.h>

void EY_IR_Begin(uint8_t pin);
void EY_IR_Tick();
void EY_IR_Reset();
