#pragma once
// =====================================================
// Prop: Cocktail Machine (Hollywood Room)
// 5 buttons — all 5 must be pressed simultaneously
// to solve. Opens trapdoor maglock.
// =====================================================

// Identity
static const char* SITE_ID     = "ey1";
static const char* ROOM_ID     = "hollywood";
static const char* DEVICE_ID   = "hollywood_cocktail";
static const char* DEVICE_NAME = "Cocktail Machine";

// Static IP
static const IPAddress STATIC_IP(192, 168, 2, 199);

// Sensors — 5 buttons wired to GND (INPUT_PULLUP, LOW when pressed)
static const SensorDef SENSORS[] = {
  //  id         pin  presentWhen              actionEvent        needsArming
  { "btn1",      13,  PresentWhen::LOW_LEVEL,  "button_pressed",  true },
  { "btn2",      14,  PresentWhen::LOW_LEVEL,  "button_pressed",  true },
  { "btn3",      27,  PresentWhen::LOW_LEVEL,  "button_pressed",  true },
  { "btn4",      26,  PresentWhen::LOW_LEVEL,  "button_pressed",  true },
  { "btn5",      32,  PresentWhen::LOW_LEVEL,  "button_pressed",  true },
};
static constexpr uint8_t SENSOR_COUNT = sizeof(SENSORS) / sizeof(SENSORS[0]);
static constexpr SolveMode SOLVE_MODE = SolveMode::ALL;

// Servo output (replaces maglock)
#define HAS_SERVO
static constexpr uint8_t SERVO_PIN = 25;
static constexpr int SERVO_ANGLE_CLOSED = 0;    // Reset position
static constexpr int SERVO_ANGLE_OPEN   = 180;  // Solved position (reveal)

// Manual reset: GM holds btn1 for 3 seconds to reset prop
#define HAS_MANUAL_RESET
static constexpr uint8_t MANUAL_RESET_PIN = 13;       // Same as btn1
static constexpr unsigned long MANUAL_RESET_HOLD_MS = 3000;

// No standard outputs — servo handled by HAS_SERVO
static const OutputDef OUTPUTS[] = {};
static constexpr uint8_t OUTPUT_COUNT = 0;

// LED
static const bool LED_ACTIVE_LOW = false;
static const int LED_MIRROR_SENSOR = -1;  // no single sensor to mirror
