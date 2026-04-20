#include "EY_Config.h"  // Must be first — brings in PROP_CONFIG which may define HAS_SHAKER

#ifdef HAS_SHAKER

#include "EY_Shaker.h"
#include "EY_Mqtt.h"
#include <esp_now.h>
#include <WiFi.h>

// ============================================================
// ESP-NOW shake detection
// ============================================================
// An ESP32-C3 + MPU6050 inside the shaker sends ESP-NOW
// broadcast packets when shaking is detected. This module
// receives those packets and feeds them into the existing
// accumulation / decay / solve logic.

// Packet structure (must match transmitter)
typedef struct __attribute__((packed)) {
  uint8_t magic;      // 0x5A
  float   magnitude;  // acceleration in g
} ShakePacket;

// Set by ESP-NOW callback (runs in WiFi task context)
static volatile unsigned long s_lastShakeMs = 0;
static volatile bool s_shakeReceived = false;
static volatile unsigned long s_packetCount = 0;

static void onEspNowRecv(const uint8_t *mac, const uint8_t *data, int len) {
  if (len == sizeof(ShakePacket)) {
    const ShakePacket *pkt = (const ShakePacket *)data;
    if (pkt->magic == 0x5A) {
      s_lastShakeMs = millis();
      s_shakeReceived = true;
      s_packetCount++;
    }
  }
}

// Runtime state
static float    s_accumulatedMs = 0.0f;
static unsigned long s_lastTickMs = 0;
static bool     s_solved = false;
static bool     s_shakeEventSent = false;
static unsigned long s_lastReportMs = 0;
static bool     s_espNowReady = false;

// How long after last packet to still count as "shaking"
static constexpr unsigned long SHAKE_TIMEOUT_MS = 500;

void EY_Shaker_Begin(uint8_t rxPin) {
  (void)rxPin; // No longer used — ESP-NOW is wireless

  s_accumulatedMs = 0.0f;
  s_lastTickMs = millis();
  s_solved = false;
  s_shakeEventSent = false;
  s_lastReportMs = 0;
  s_espNowReady = false;

  Serial.println("[Shaker] Initialized (ESP-NOW mode, waiting for WiFi)");
  Serial.printf("[Shaker] Target: %lums, Decay: %.0fms/s\n", SHAKE_TARGET_MS, SHAKE_DECAY_PER_SEC);
}

bool EY_Shaker_Tick() {
  if (s_solved) return true;

  // Deferred ESP-NOW init — WiFi must be connected first (channel locked)
  if (!s_espNowReady && WiFi.status() == WL_CONNECTED) {
    if (esp_now_init() == ESP_OK) {
      esp_now_register_recv_cb(onEspNowRecv);
      s_espNowReady = true;
      Serial.println("[Shaker] ESP-NOW receiver ready");
      Serial.printf("[Shaker] WiFi channel: %d\n", WiFi.channel());
    } else {
      Serial.println("[Shaker] ESP-NOW init failed!");
    }
  }

  unsigned long now = millis();
  unsigned long deltaMs = now - s_lastTickMs;
  s_lastTickMs = now;

  // Clamp delta to avoid huge jumps
  if (deltaMs > 500) deltaMs = 500;

  // Shaking = received an ESP-NOW packet recently
  bool shaking = s_shakeReceived && (now - s_lastShakeMs < SHAKE_TIMEOUT_MS);

  // Accumulate or decay
  if (shaking) {
    s_accumulatedMs += (float)deltaMs;

    if (!s_shakeEventSent) {
      s_shakeEventSent = true;
      EY_PublishEvent("shake_started", EY_MQTT::SRC_PLAYER);
      Serial.println("[Shaker] Shake started");
    }
  } else {
    float decayMs = SHAKE_DECAY_PER_SEC * ((float)deltaMs / 1000.0f);
    s_accumulatedMs -= decayMs;
    if (s_accumulatedMs <= 0.0f) {
      s_accumulatedMs = 0.0f;
      s_shakeEventSent = false;
    }
  }

  // Clamp
  if (s_accumulatedMs > (float)SHAKE_TARGET_MS) s_accumulatedMs = (float)SHAKE_TARGET_MS;

  // Periodic progress report
  if (now - s_lastReportMs >= SHAKE_REPORT_INTERVAL_MS) {
    s_lastReportMs = now;
    uint8_t progress = EY_Shaker_GetProgress();
    unsigned long pkts = s_packetCount;
    s_packetCount = 0;
    Serial.printf("[Shaker] Progress: %d%% (%lu/%lums) %s [%lu pkts]\n",
      progress, (unsigned long)s_accumulatedMs, SHAKE_TARGET_MS,
      shaking ? "SHAKING" : "idle", pkts);
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
  s_shakeEventSent = false;
  s_lastTickMs = millis();
  s_lastReportMs = 0;
  s_shakeReceived = false;

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
