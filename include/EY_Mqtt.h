#pragma once
// MQTT Contract: MQTT_CONTRACT_v1.md (v1.0 FROZEN)

#include <Arduino.h>

// ---- MQTT JSON contract constants (avoid typos across the project) ----
// Must match MQTT_CONTRACT_v1.md field names exactly.
namespace EY_MQTT {
  // Field names (contract v1)
  static constexpr const char* F_PROP_ID            = "propId";
  static constexpr const char* F_TYPE               = "type";               // "event" | "status"
  static constexpr const char* F_ACTION             = "action";
  static constexpr const char* F_SOURCE             = "source";             // "player" | "gm" | "device"
  static constexpr const char* F_TIMESTAMP          = "timestamp";

  static constexpr const char* F_SOLVED             = "solved";
  static constexpr const char* F_LAST_CHANGE_SOURCE = "lastChangeSource";
  static constexpr const char* F_OVERRIDE           = "override";
  static constexpr const char* F_ONLINE             = "online";

  // Values
  static constexpr const char* TYPE_EVENT  = "event";
  static constexpr const char* TYPE_STATUS = "status";
  static constexpr const char* TYPE_CMD    = "cmd";
  static constexpr const char* SRC_PLAYER  = "player";
  static constexpr const char* SRC_GM      = "gm";
  static constexpr const char* SRC_DEVICE  = "device";
}

// Callbacks
typedef void (*ResetCallback)();
typedef void (*SetSolvedCallback)(bool solved, const char* source);
typedef void (*ArmCallback)();

// Network lifecycle
void EY_Net_Begin(ResetCallback onReset, SetSolvedCallback onSetSolved = nullptr, ArmCallback onArm = nullptr);
void EY_Net_Tick();                 // call every loop
bool EY_Mqtt_Connected();

// Legacy publishing helpers (kept for backward compatibility)
void EY_PublishEventOk(const char* sensorName);
void EY_PublishSolved(bool solved);

// New publishing helpers (v2 contract)
void EY_PublishEvent(const char* action, const char* source);
void EY_PublishStatus(bool solved, const char* lastChangeSource, bool overrideActive);
