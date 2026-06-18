#pragma once
// =====================================================
// Prop: Cocktail Machine (Hollywood Room)
// 5 cocktail buttons must be pressed in order:
//   Old Fashioned -> Cosmopolitan -> Blue Lagoon -> Martini -> Mojito
// Wrong press resets progress. Solve opens trapdoor servo.
// =====================================================

// Identity
static const char* SITE_ID     = "ey1";
static const char* ROOM_ID     = "hollywood";
static const char* DEVICE_ID   = "hollywood_cocktail";
static const char* DEVICE_NAME = "Cocktail Machine";

// Static IP
static const IPAddress STATIC_IP(192, 168, 2, 199);

// Sensors — buttons wired to GND (INPUT_PULLUP, LOW when pressed).
// Declaration order of NON-decorative sensors = required press order.
// Decorative buttons (decorative=true) publish events on every press but
// never affect the SEQUENCE solve logic — used for sound feedback only.
static const SensorDef SENSORS[] = {
  //  id                pin  presentWhen              actionEvent        needsArming  decorative
  { "old_fashioned",    13,  PresentWhen::LOW_LEVEL,  "button_pressed",  true,        false },
  { "cosmopolitan",     14,  PresentWhen::LOW_LEVEL,  "button_pressed",  true,        false },
  { "blue_lagoon",      27,  PresentWhen::LOW_LEVEL,  "button_pressed",  true,        false },
  { "martini",          26,  PresentWhen::LOW_LEVEL,  "button_pressed",  true,        false },
  { "mojito",           32,  PresentWhen::LOW_LEVEL,  "button_pressed",  true,        false },
  // Decorative buttons (sound feedback via RC scenarios — no puzzle impact)
  { "fake1",            18,  PresentWhen::LOW_LEVEL,  "fake1_pressed",   true,        true  },
  { "fake2",            19,  PresentWhen::LOW_LEVEL,  "fake2_pressed",   true,        true  },
  { "fake3",            21,  PresentWhen::LOW_LEVEL,  "fake3_pressed",   true,        true  },
  { "fake4",             5,  PresentWhen::LOW_LEVEL,  "fake4_pressed",   true,        true  },
};
static constexpr uint8_t SENSOR_COUNT = sizeof(SENSORS) / sizeof(SENSORS[0]);
static constexpr SolveMode SOLVE_MODE = SolveMode::SEQUENCE;

// Servo output (replaces maglock)
#define HAS_SERVO
static constexpr uint8_t SERVO_PIN = 25;
static constexpr int SERVO_ANGLE_CLOSED = 0;    // Reset position
static constexpr int SERVO_ANGLE_OPEN   = 180;  // Solved position (reveal)

// Manual reset: GM holds first button (Old Fashioned) for 3 seconds to reset prop
#define HAS_MANUAL_RESET
static constexpr uint8_t MANUAL_RESET_PIN = 13;       // Same as old_fashioned
static constexpr unsigned long MANUAL_RESET_HOLD_MS = 3000;

// No standard outputs — servo handled by HAS_SERVO
static const OutputDef OUTPUTS[] = {};
static constexpr uint8_t OUTPUT_COUNT = 0;

// LED
static const bool LED_ACTIVE_LOW = false;
static const int LED_MIRROR_SENSOR = -1;  // no single sensor to mirror
