#pragma once

#include <Arduino.h>

// ============================================================
// Bobine (ceiling film-reel) Module API
// ============================================================
// Drives N MOSFET-switched puck lights arranged on a ceiling
// "film reel". Each puck backlights an opaque acrylic disc
// with a digit sticker behind it. When the puzzle is solved,
// pucks light up one at a time in a fixed sequence to reveal
// the digits of a code, then pause, then loop forever until
// the puzzle is reset.
//
// Config (from prop header, under #ifdef HAS_BOBINE):
//   BOBINE_PUCK_PINS[]     array of GPIO pins driving each puck
//   BOBINE_PUCK_COUNT      number of pucks (size of array)
//   BOBINE_ACTIVE_HIGH     true if GPIO HIGH lights the puck
//   BOBINE_SEQUENCE[]      order of puck indices to light
//   BOBINE_SEQUENCE_LENGTH size of BOBINE_SEQUENCE[]
//   BOBINE_ON_MS           per-puck on-time
//   BOBINE_GAP_MS          off-time between pucks
//   BOBINE_PAUSE_MS        off-time after full sequence

// Initialize pins (all pucks off). Call once in setup().
void EY_Bobine_Begin();

// Advance state machine. Non-blocking. Call every loop iteration.
void EY_Bobine_Tick();

// Begin the light sequence. Safe to call if already running (no-op).
void EY_Bobine_Start();

// Stop the sequence immediately and turn all pucks off.
void EY_Bobine_Stop();

// Alias of Stop — called on puzzle reset.
void EY_Bobine_Reset();

// Stop the sequence and light every puck solid (final "all digits
// revealed" state shown when the players enter the correct code).
void EY_Bobine_RevealAll();
