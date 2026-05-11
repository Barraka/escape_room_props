#include "EY_Config.h"  // Must be first — brings in PROP_CONFIG which may define HAS_BOBINE

#ifdef HAS_BOBINE

#include "EY_Bobine.h"
#include <Arduino.h>

enum class BobinePhase : uint8_t { IDLE, ON, GAP, PAUSE };

static BobinePhase   s_phase       = BobinePhase::IDLE;
static uint32_t      s_phaseStart  = 0;
static uint8_t       s_seqIdx      = 0;
static bool          s_running     = false;

static inline void setPuck(uint8_t puckIdx, bool on) {
  if (puckIdx >= BOBINE_PUCK_COUNT) return;
  bool level = BOBINE_ACTIVE_HIGH ? on : !on;
  digitalWrite(BOBINE_PUCK_PINS[puckIdx], level ? HIGH : LOW);
}

static void allOff() {
  for (uint8_t i = 0; i < BOBINE_PUCK_COUNT; i++) {
    setPuck(i, false);
  }
}

void EY_Bobine_Begin() {
  for (uint8_t i = 0; i < BOBINE_PUCK_COUNT; i++) {
    pinMode(BOBINE_PUCK_PINS[i], OUTPUT);
  }
  allOff();
  s_phase = BobinePhase::IDLE;
  s_running = false;
  s_seqIdx = 0;
  Serial.print("[Bobine] Initialized with ");
  Serial.print(BOBINE_PUCK_COUNT);
  Serial.print(" pucks, sequence length ");
  Serial.println(BOBINE_SEQUENCE_LENGTH);
}

void EY_Bobine_Start() {
  if (s_running) return;
  s_running = true;
  s_seqIdx = 0;
  s_phase = BobinePhase::ON;
  s_phaseStart = millis();
  setPuck(BOBINE_SEQUENCE[s_seqIdx], true);
  Serial.println("[Bobine] Sequence started");
}

void EY_Bobine_Stop() {
  if (!s_running && s_phase == BobinePhase::IDLE) return;
  s_running = false;
  s_phase = BobinePhase::IDLE;
  allOff();
  Serial.println("[Bobine] Sequence stopped");
}

void EY_Bobine_Reset() {
  EY_Bobine_Stop();
}

void EY_Bobine_Tick() {
  if (!s_running) return;

  uint32_t elapsed = millis() - s_phaseStart;

  switch (s_phase) {
    case BobinePhase::ON:
      if (elapsed >= BOBINE_ON_MS) {
        setPuck(BOBINE_SEQUENCE[s_seqIdx], false);
        s_seqIdx++;
        s_phaseStart = millis();
        if (s_seqIdx >= BOBINE_SEQUENCE_LENGTH) {
          s_phase = BobinePhase::PAUSE;
        } else {
          s_phase = BobinePhase::GAP;
        }
      }
      break;

    case BobinePhase::GAP:
      if (elapsed >= BOBINE_GAP_MS) {
        setPuck(BOBINE_SEQUENCE[s_seqIdx], true);
        s_phase = BobinePhase::ON;
        s_phaseStart = millis();
      }
      break;

    case BobinePhase::PAUSE:
      if (elapsed >= BOBINE_PAUSE_MS) {
        s_seqIdx = 0;
        setPuck(BOBINE_SEQUENCE[s_seqIdx], true);
        s_phase = BobinePhase::ON;
        s_phaseStart = millis();
      }
      break;

    case BobinePhase::IDLE:
    default:
      break;
  }
}

#endif  // HAS_BOBINE
