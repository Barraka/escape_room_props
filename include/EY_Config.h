#pragma once

#include "EY_Types.h"  // For SensorDef, PresentWhen, SolveMode

// MQTT Contract: MQTT_CONTRACT_v1.md (v1.0 FROZEN)
static constexpr const char* MQTT_CONTRACT_VERSION = "1.0";


// =====================
// Identity (edit per prop)
// =====================
static const char* SITE_ID   = "default";           // Site identifier for multi-location support
static const char* ROOM_ID   = "magie";
static const char* DEVICE_ID = "magie_roueFortune";

// =====================
// Wi-Fi (same per room)
// =====================
static const char* WIFI_SSID = "SFR_3474";
static const char* WIFI_PASS = "y6zj8asvtvjxy524qdzq";

// =====================
// MQTT (same per room)
// =====================
static const char* MQTT_HOST = "192.168.1.99";
static const int   MQTT_PORT = 1883;

// =====================
// Hardware Pins (edit per prop)
// =====================
static const int LED_PIN       = 2;    // Onboard LED (feedback)
static const int RESET_BTN_PIN = 0;    // BOOT button (active LOW)

// =====================
// Sensor Definitions (edit per prop)
// =====================
// Each prop defines its sensors here. The sensor system handles:
// - Pin initialization (INPUT_PULLUP)
// - Polarity (HIGH_LEVEL or LOW_LEVEL for "present")
// - Arming (optional: must see "not present" before "present" counts)
// - Event publishing (one-shot per reset)
//
// Example configurations:
//
// Fortune Wheel (RFID + magnet, either triggers solve):
//   SolveMode::ANY
//
// 3-Magnet puzzle (all magnets must be placed):
//   SolveMode::ALL
//
static const SensorDef SENSORS[] = {
  //  id         pin  presentWhen              actionEvent       needsArming
  { "rfid1",     12,  PresentWhen::HIGH_LEVEL, "rfid_present",   true  },
  { "magnet1",   27,  PresentWhen::LOW_LEVEL,  "magnet_present", false },
};
static constexpr uint8_t SENSOR_COUNT = sizeof(SENSORS) / sizeof(SENSORS[0]);
static constexpr SolveMode SOLVE_MODE = SolveMode::ANY;

// =====================
// Behavior Tuning
// =====================
static const unsigned long SOLVED_BLINK_MS         = 300;   // LED blink rate when solved
static const unsigned long RESET_HOLD_MS           = 1000;  // Button hold time to trigger reset
static const unsigned long RESET_FEEDBACK_MS       = 1500;  // Fast blink duration after reset
static const unsigned long RESET_FEEDBACK_BLINK_MS = 100;   // Fast blink rate
static const unsigned long IGNORE_SENSORS_MS       = 2000;  // Ignore sensors briefly after reset

// LED polarity (set true if LED is wired active-low)
static const bool LED_ACTIVE_LOW = false;
