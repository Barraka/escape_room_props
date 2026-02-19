#pragma once

#include "EY_Types.h"  // For SensorDef, PresentWhen, SolveMode

// Firmware Version (increment when making changes)
static constexpr const char* FIRMWARE_VERSION = "1.4.0";

// MQTT Contract: MQTT_CONTRACT_v1.md (v1.0 FROZEN)
static constexpr const char* MQTT_CONTRACT_VERSION = "1.0";

// =====================
// Wi-Fi (same per room)
// =====================
static const char* WIFI_SSID = "SFR_3474";
static const char* WIFI_PASS = "y6zj8asvtvjxy524qdzq";

// Static IP network settings (same for all props on this network)
static const IPAddress WIFI_GATEWAY(192, 168, 1, 1);
static const IPAddress WIFI_SUBNET(255, 255, 255, 0);
static const IPAddress WIFI_DNS(192, 168, 1, 1);

// =====================
// MQTT (same per room)
// =====================
static const char* MQTT_HOST = "192.168.1.10";
static const int   MQTT_PORT = 1883;

// =====================
// NTP (Real-time clock sync)
// =====================
static const char* NTP_SERVER1  = "192.168.1.1";   // Router (often has built-in NTP)
static const char* NTP_SERVER2  = "pool.ntp.org";   // Fallback (requires internet)
static const long  NTP_GMT_OFFSET = 0;              // UTC (room controller handles timezone)
static const int   NTP_DST_OFFSET = 0;              // No DST adjustment (using UTC)

// =====================
// OTA (Over-The-Air updates)
// =====================
static const char* OTA_PASSWORD = "escapeyourself";  // Shared password for all props
static const int   OTA_PORT     = 3232;              // Default ArduinoOTA port

// =====================
// Hardware Pins (common to all ESP32 devkits)
// =====================
static const int LED_PIN       = 2;    // Onboard LED (feedback)
static const int RESET_BTN_PIN = 0;    // BOOT button (active LOW)

// =====================
// Behavior Tuning
// =====================
static const unsigned long SOLVED_BLINK_MS         = 300;   // LED blink rate when solved
static const unsigned long RESET_HOLD_MS           = 1000;  // Button hold time to trigger reset
static const unsigned long RESET_FEEDBACK_MS       = 1500;  // Fast blink duration after reset
static const unsigned long RESET_FEEDBACK_BLINK_MS = 100;   // Fast blink rate
static const unsigned long IGNORE_SENSORS_MS       = 2000;  // Ignore sensors briefly after reset
static const unsigned long DEBOUNCE_MS             = 20;    // Debounce window for mechanical sensors (ms)

// =====================
// Per-Prop Config
// =====================
// Each prop has its own file in include/props/ defining:
//   SITE_ID, ROOM_ID, DEVICE_ID, DEVICE_NAME
//   SENSORS[], SENSOR_COUNT, SOLVE_MODE
//   OUTPUTS[], OUTPUT_COUNT
//   LED_ACTIVE_LOW, LED_MIRROR_SENSOR
//
// Build with: pio run -e <prop_name>
//
#ifndef PROP_CONFIG
  #error "No prop selected! Use: pio run -e <prop_name>"
#endif
#include PROP_CONFIG
