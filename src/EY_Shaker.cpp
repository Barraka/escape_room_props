#include "EY_Config.h"  // Must be first — brings in PROP_CONFIG which may define HAS_SHAKER

#ifdef HAS_SHAKER

#include "EY_Shaker.h"
#include "EY_Mqtt.h"

// Runtime state
static uint8_t  s_rxPin = 0;
static float    s_accumulatedMs = 0.0f;     // Accumulated shake time (ms)
static unsigned long s_lastTickMs = 0;      // millis() of last tick
static bool     s_solved = false;           // Latched solved state
static bool     s_shaking = false;          // Currently receiving signal
static bool     s_shakeEventSent = false;   // One-shot "shake_started" event
static unsigned long s_lastReportMs = 0;    // Last MQTT progress report

// Noise filter: ignore pulses shorter than this
static constexpr unsigned long PULSE_FILTER_MS = 10;
static bool     s_lastRaw = false;
static unsigned long s_lastRawChangeMs = 0;
static bool     s_filteredState = false;    // Debounced/filtered signal state

void EY_Shaker_Begin(uint8_t rxPin) {
  s_rxPin = rxPin;
  pinMode(rxPin, INPUT);
  s_accumulatedMs = 0.0f;
  s_lastTickMs = millis();
  s_solved = false;
  s_shaking = false;
  s_shakeEventSent = false;
  s_lastReportMs = 0;
  s_lastRaw = false;
  s_lastRawChangeMs = millis();
  s_filteredState = false;

  Serial.println("[Shaker] Initialized on GPIO " + String(rxPin));
  Serial.println("[Shaker] Target: " + String(SHAKE_TARGET_MS) + "ms, Decay: " + String(SHAKE_DECAY_PER_SEC) + "ms/s");
}

bool EY_Shaker_Tick() {
  if (s_solved) return true;

  unsigned long now = millis();
  unsigned long deltaMs = now - s_lastTickMs;
  s_lastTickMs = now;

  // Clamp delta to avoid huge jumps (e.g. after OTA or debug pause)
  if (deltaMs > 500) deltaMs = 500;

  // Read raw GPIO
  bool rawHigh = (digitalRead(s_rxPin) == HIGH);

  // Noise filter: only accept state change after stable for PULSE_FILTER_MS
  if (rawHigh != s_lastRaw) {
    s_lastRaw = rawHigh;
    s_lastRawChangeMs = now;
  }

  if (s_lastRaw != s_filteredState && (now - s_lastRawChangeMs >= PULSE_FILTER_MS)) {
    s_filteredState = s_lastRaw;
  }

  s_shaking = s_filteredState;

  // Accumulate or decay
  if (s_shaking) {
    s_accumulatedMs += (float)deltaMs;

    // Publish one-shot "shake_started" event
    if (!s_shakeEventSent) {
      s_shakeEventSent = true;
      EY_PublishEvent("shake_started", EY_MQTT::SRC_PLAYER);
      Serial.println("[Shaker] Shake started");
    }
  } else {
    // Decay accumulated time
    float decayMs = SHAKE_DECAY_PER_SEC * ((float)deltaMs / 1000.0f);
    s_accumulatedMs -= decayMs;
  }

  // Clamp
  if (s_accumulatedMs < 0.0f) s_accumulatedMs = 0.0f;
  if (s_accumulatedMs > (float)SHAKE_TARGET_MS) s_accumulatedMs = (float)SHAKE_TARGET_MS;

  // Periodic progress report via serial
  if (now - s_lastReportMs >= SHAKE_REPORT_INTERVAL_MS) {
    s_lastReportMs = now;
    uint8_t progress = EY_Shaker_GetProgress();
    Serial.print("[Shaker] Progress: ");
    Serial.print(progress);
    Serial.print("% (");
    Serial.print((unsigned long)s_accumulatedMs);
    Serial.print("/");
    Serial.print(SHAKE_TARGET_MS);
    Serial.println("ms)");
  }

  // Check solve
  if (s_accumulatedMs >= (float)SHAKE_TARGET_MS) {
    s_solved = true;
    Serial.println("[Shaker] SOLVED!");
    return true;
  }

  return false;
}

void EY_Shaker_Reset() {
  s_accumulatedMs = 0.0f;
  s_solved = false;
  s_shaking = false;
  s_shakeEventSent = false;
  s_lastTickMs = millis();
  s_lastReportMs = 0;
  s_lastRaw = false;
  s_lastRawChangeMs = millis();
  s_filteredState = false;
  Serial.println("[Shaker] Reset");
}

uint8_t EY_Shaker_GetProgress() {
  if (s_solved) return 100;
  float ratio = s_accumulatedMs / (float)SHAKE_TARGET_MS;
  if (ratio < 0.0f) ratio = 0.0f;
  if (ratio > 1.0f) ratio = 1.0f;
  return (uint8_t)(ratio * 100.0f);
}

#endif // HAS_SHAKER
