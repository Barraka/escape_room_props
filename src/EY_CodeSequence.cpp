#include "EY_Config.h"  // Brings in PROP_CONFIG which may define HAS_CODE_SEQUENCE

#ifdef HAS_CODE_SEQUENCE

#include "EY_CodeSequence.h"
#include <string.h>

static uint8_t s_currentStep = 0;
static bool    s_solved      = false;

static void setLed(uint8_t index, bool on) {
  if (index >= CODE_SEQUENCE_COUNT) return;
  digitalWrite(CODE_SEQUENCE_LED_PINS[index], on ? HIGH : LOW);
}

void EY_CodeSequence_Begin() {
  for (uint8_t i = 0; i < CODE_SEQUENCE_COUNT; i++) {
    pinMode(CODE_SEQUENCE_LED_PINS[i], OUTPUT);
    digitalWrite(CODE_SEQUENCE_LED_PINS[i], LOW);
  }
  s_currentStep = 0;
  s_solved = false;

  Serial.print("[CodeSequence] Ready — ");
  Serial.print(CODE_SEQUENCE_COUNT);
  Serial.print(" codes, LED pins:");
  for (uint8_t i = 0; i < CODE_SEQUENCE_COUNT; i++) {
    Serial.print(" ");
    Serial.print(CODE_SEQUENCE_LED_PINS[i]);
  }
  Serial.println();
}

CodeSequenceResult EY_CodeSequence_Submit(const char* code) {
  if (s_solved) {
    Serial.println("[CodeSequence] Already solved — ignoring submission");
    return CodeSequenceResult::WRONG;
  }
  if (s_currentStep >= CODE_SEQUENCE_COUNT) {
    return CodeSequenceResult::WRONG;
  }

  const char* expected = CODE_SEQUENCE_EXPECTED[s_currentStep];
  if (strcmp(code, expected) != 0) {
    Serial.print("[CodeSequence] WRONG at step ");
    Serial.print(s_currentStep);
    Serial.print(" — got \"");
    Serial.print(code);
    Serial.print("\" expected \"");
    Serial.print(expected);
    Serial.println("\"");
    return CodeSequenceResult::WRONG;
  }

  setLed(s_currentStep, true);
  Serial.print("[CodeSequence] CORRECT at step ");
  Serial.print(s_currentStep);
  Serial.print(" — LED ");
  Serial.print(s_currentStep);
  Serial.println(" ON");

  s_currentStep++;

  if (s_currentStep >= CODE_SEQUENCE_COUNT) {
    s_solved = true;
    Serial.println("[CodeSequence] SOLVED — all codes entered");
    return CodeSequenceResult::SOLVED;
  }
  return CodeSequenceResult::CORRECT;
}

void EY_CodeSequence_Reset() {
  for (uint8_t i = 0; i < CODE_SEQUENCE_COUNT; i++) {
    setLed(i, false);
  }
  s_currentStep = 0;
  s_solved = false;
  Serial.println("[CodeSequence] Reset");
}

void EY_CodeSequence_ForceSolve() {
  for (uint8_t i = 0; i < CODE_SEQUENCE_COUNT; i++) {
    setLed(i, true);
  }
  s_currentStep = CODE_SEQUENCE_COUNT;
  s_solved = true;
  Serial.println("[CodeSequence] Force-solved — all LEDs ON");
}

bool EY_CodeSequence_IsSolved() {
  return s_solved;
}

uint8_t EY_CodeSequence_GetCurrentStep() {
  return s_currentStep;
}

#endif // HAS_CODE_SEQUENCE
