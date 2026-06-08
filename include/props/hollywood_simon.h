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

// Simon pin assignments (this devkit does NOT break out GPIO0).
// Buttons: LOW when pressed. 9 use internal pull-ups; button #10 (GPIO34) is
// input-only and needs an external 10kΩ to 3.3V (only resistor on the board).
// LEDs: HIGH = on. GPIO 2/12/15 are boot-strapping pins — reserved for LEDs only
// (an LED to GND is safe there; a button would idle HIGH and break boot).
static const uint8_t SIMON_BTN_PINS[] = { 4,  5, 13, 16, 17, 18, 19, 25, 32, 34};
static const uint8_t SIMON_LED_PINS[] = { 2, 12, 14, 15, 21, 22, 23, 26, 27, 33};
static constexpr uint8_t SIMON_COUNT  = 10;

// Timing
static constexpr unsigned long SIMON_BLINK_MIN_MS      = 1000;  // Min interval between blinks
static constexpr unsigned long SIMON_BLINK_MAX_MS      = 3000;  // Max interval between blinks
static constexpr unsigned long SIMON_BLINK_DURATION_MS  = 450;  // LED on duration per blink
static constexpr unsigned long SIMON_DEBOUNCE_MS        = 50;   // Button debounce
static constexpr unsigned long SIMON_REPORT_INTERVAL_MS = 1000; // MQTT status report interval
static constexpr unsigned long SIMON_VICTORY_BLINK_MS   = 200;  // Per-phase toggle in the win animation (3 blinks, ending solid on)

// No standard sensors — Simon module handles button input
static const SensorDef SENSORS[] = {};
static constexpr uint8_t SENSOR_COUNT = 0;
static constexpr SolveMode SOLVE_MODE = SolveMode::ALL;

// No outputs — this prop sits on the HQ console. On solve it latches and
// publishes its MQTT status; the Room Controller fires the next video sequence.
static const OutputDef OUTPUTS[] = {};
static constexpr uint8_t OUTPUT_COUNT = 0;

// LED
static const bool LED_ACTIVE_LOW = false;
static const int LED_MIRROR_SENSOR = -1;
