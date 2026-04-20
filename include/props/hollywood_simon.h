#pragma once
// =====================================================
// Prop: Simon Buzzer Buttons (Hollywood Room — Secret HQ)
// 10 illuminated buttons blink randomly. Press a lit
// button to lock it. All 10 locked = solved.
// =====================================================

#define HAS_SIMON

// Identity
static const char* SITE_ID     = "ey1";
static const char* ROOM_ID     = "hollywood";
static const char* DEVICE_ID   = "hollywood_simon";
static const char* DEVICE_NAME = "Simon Buzzers";

// Static IP
static const IPAddress STATIC_IP(192, 168, 2, 202);

// Simon pin assignments
// Buttons: LOW when pressed. GPIO 34-36 are input-only (need external 10kΩ pull-up to 3.3V)
static const uint8_t SIMON_BTN_PINS[] = { 4,  5, 16, 17, 18, 19, 33, 34, 35, 36};
static const uint8_t SIMON_LED_PINS[] = {12, 13, 14, 15, 21, 22, 23, 26, 27, 32};
static constexpr uint8_t SIMON_COUNT  = 10;

// Timing
static constexpr unsigned long SIMON_BLINK_MIN_MS      = 1000;  // Min interval between blinks
static constexpr unsigned long SIMON_BLINK_MAX_MS      = 3000;  // Max interval between blinks
static constexpr unsigned long SIMON_BLINK_DURATION_MS  = 600;  // LED on duration per blink
static constexpr unsigned long SIMON_DEBOUNCE_MS        = 50;   // Button debounce
static constexpr unsigned long SIMON_REPORT_INTERVAL_MS = 1000; // MQTT status report interval

// No standard sensors — Simon module handles button input
static const SensorDef SENSORS[] = {};
static constexpr uint8_t SENSOR_COUNT = 0;
static constexpr SolveMode SOLVE_MODE = SolveMode::ALL;

// Outputs — maglock
static const OutputDef OUTPUTS[] = {
  //  id           pin  activeLow
  { "maglock1",    25,  false },  // XY-MOS module: HIGH = ON (active-high)
};
static constexpr uint8_t OUTPUT_COUNT = sizeof(OUTPUTS) / sizeof(OUTPUTS[0]);

// LED
static const bool LED_ACTIVE_LOW = false;
static const int LED_MIRROR_SENSOR = -1;
