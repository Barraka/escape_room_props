#pragma once

#include "EY_Types.h"  // For SensorDef, PresentWhen, SolveMode

// Firmware Version (increment when making changes)
static constexpr const char* FIRMWARE_VERSION = "1.3.0";

// MQTT Contract: MQTT_CONTRACT_v1.md (v1.0 FROZEN)
static constexpr const char* MQTT_CONTRACT_VERSION = "1.0";


// =====================
// Identity (edit per prop)
// =====================
static const char* SITE_ID   = "default";           // Site identifier for multi-location support
static const char* ROOM_ID   = "magie";
static const char* DEVICE_ID = "magie_roueFortune";
static const char* DEVICE_NAME = "Roue de la Fortune";

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
// OTA (Over-The-Air updates)
// =====================
static const char* OTA_PASSWORD = "escapeyourself";  // Shared password for all props
static const int   OTA_PORT     = 3232;              // Default ArduinoOTA port

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
  // WARNING: GPIO 12 is an ESP32 strapping pin. If pulled HIGH at boot, the chip
  // may fail to start (wrong flash voltage). Prefer GPIO 13, 14, 25, 26, 32, 33.
  { "rfid1",     12,  PresentWhen::HIGH_LEVEL, "rfid_present",   true  },
  { "magnet1",   27,  PresentWhen::LOW_LEVEL,  "magnet_present", false },
};
static constexpr uint8_t SENSOR_COUNT = sizeof(SENSORS) / sizeof(SENSORS[0]);
static constexpr SolveMode SOLVE_MODE = SolveMode::ANY;

// =====================
// Output Definitions (edit per prop)
// =====================
// Each prop defines its outputs here (maglocks, relays, etc.).
// Lifecycle: INACTIVE (boot) → ARMED (GM arms) → RELEASED (solve/force_solve)
// Fail-safe: outputs start INACTIVE (e.g., maglock unlocked) on boot/power loss.
//
static const OutputDef OUTPUTS[] = {
  //  id           pin  activeLow
  { "maglock1",    25,  false },  // XY-MOS module: HIGH = ON (active-high)
};
static constexpr uint8_t OUTPUT_COUNT = sizeof(OUTPUTS) / sizeof(OUTPUTS[0]);

// =====================
// Behavior Tuning
// =====================
static const unsigned long SOLVED_BLINK_MS         = 300;   // LED blink rate when solved
static const unsigned long RESET_HOLD_MS           = 1000;  // Button hold time to trigger reset
static const unsigned long RESET_FEEDBACK_MS       = 1500;  // Fast blink duration after reset
static const unsigned long RESET_FEEDBACK_BLINK_MS = 100;   // Fast blink rate
static const unsigned long IGNORE_SENSORS_MS       = 2000;  // Ignore sensors briefly after reset
static const unsigned long DEBOUNCE_MS             = 20;    // Debounce window for mechanical sensors (ms)

// LED polarity (set true if LED is wired active-low)
static const bool LED_ACTIVE_LOW = false;

// LED mirror: index into SENSORS[] whose state is mirrored on LED_PIN (-1 to disable)
static const int LED_MIRROR_SENSOR = 1;  // magnet1
