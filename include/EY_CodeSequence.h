#pragma once
// Code Sequence module — sequential code validation with per-step LED feedback.
// Player enters codes one at a time; each correct code (in order) lights the
// corresponding LED. Solved when all codes have been entered in order.
// Feature guard: #define HAS_CODE_SEQUENCE in prop config

#include <Arduino.h>

enum class CodeSequenceResult : uint8_t {
  WRONG     = 0,  // Code did not match expected[currentStep]
  CORRECT   = 1,  // Matched — lit LED, advanced to next step
  SOLVED    = 2,  // Matched AND was the final code — puzzle solved
};

void EY_CodeSequence_Begin();

// Submit a code. Compares against expected[currentStep]. Lights the matching
// LED and advances on a correct match. Returns the outcome.
CodeSequenceResult EY_CodeSequence_Submit(const char* code);

// Reset to step 0, all LEDs off.
void EY_CodeSequence_Reset();

// Force the puzzle into solved state (lights all LEDs).
void EY_CodeSequence_ForceSolve();

bool    EY_CodeSequence_IsSolved();
uint8_t EY_CodeSequence_GetCurrentStep();  // 0..CODE_SEQUENCE_COUNT
