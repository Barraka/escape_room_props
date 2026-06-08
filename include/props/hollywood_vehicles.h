#pragma once
// =====================================================
// Prop: Vehicles (Hollywood Room)
// 6 three-position toggle switches (ON-OFF-ON, center-off —
// Heschen C513A type). Each switch must be set to a specific
// position. When all 6 match the target combination at once,
// the prop latches SOLVED and publishes its MQTT status; the
// Room Controller fires the next sequence. No local outputs.
// =====================================================

#define HAS_VEHICLES

// Identity
static const char* SITE_ID     = "ey1";
static const char* ROOM_ID     = "hollywood";
static const char* DEVICE_ID   = "hollywood_vehicles";
static const char* DEVICE_NAME = "Vehicles";

// Static IP (next free after bobine .205)
static const IPAddress STATIC_IP(192, 168, 2, 206);

// ---- Switch wiring ----
// Each ON-OFF-ON toggle is wired with the COMMON (centre) terminal to GND
// and the two outer terminals to a GPIO each (LEFT pin, RIGHT pin), read with
// INPUT_PULLUP. Lever toward a side closes COM to that side -> that GPIO reads
// LOW. Centre (OFF) = both terminals open = both GPIOs HIGH.
//   left LOW,  right HIGH  -> position LEFT
//   left HIGH, right LOW   -> position RIGHT
//   both HIGH              -> position CENTRE
//   both LOW               -> impossible (wiring fault) -> treated as invalid
//
// All 12 pins below have internal pull-ups (none are input-only 34-39 or
// strapping pins 2/12/15), so NO external resistors are needed. Adjust to the
// actual Vehicles devkit if its broken-out pins differ.
//                                    { LEFT pin, RIGHT pin }
static const uint8_t VEHICLE_PINS[][2] = {
  {  4,  5 },   // Vehicle 1
  { 13, 16 },   // Vehicle 2
  { 17, 18 },   // Vehicle 3
  { 19, 21 },   // Vehicle 4
  { 22, 23 },   // Vehicle 5
  { 25, 26 },   // Vehicle 6
};
static constexpr uint8_t VEHICLE_COUNT = sizeof(VEHICLE_PINS) / sizeof(VEHICLE_PINS[0]);

// ---- Position values ----
// (Defined here so the target table below reads clearly.)
#define VEH_LEFT    0
#define VEH_CENTRE  1
#define VEH_RIGHT   2

// ---- Target combination ----
// The position each switch must be in to solve. EDIT THIS to the real
// in-room solution. One entry per vehicle, in the same order as VEHICLE_PINS.
// Placeholder until the puzzle solution is confirmed.
static const uint8_t VEHICLE_TARGET[VEHICLE_COUNT] = {
  VEH_LEFT,    // Vehicle 1
  VEH_RIGHT,   // Vehicle 2
  VEH_CENTRE,  // Vehicle 3
  VEH_RIGHT,   // Vehicle 4
  VEH_LEFT,    // Vehicle 5
  VEH_CENTRE,  // Vehicle 6
};

// Timing
static constexpr unsigned long VEHICLE_DEBOUNCE_MS       = 50;    // Switch debounce
static constexpr unsigned long VEHICLE_REPORT_INTERVAL_MS = 1000; // MQTT status report interval

// No standard sensors — the Vehicles module handles switch input directly.
static const SensorDef SENSORS[] = {};
static constexpr uint8_t SENSOR_COUNT = 0;
static constexpr SolveMode SOLVE_MODE = SolveMode::ALL;

// No outputs — on solve it latches and publishes MQTT status; the Room
// Controller fires the next sequence (same model as the Simon prop).
static const OutputDef OUTPUTS[] = {};
static constexpr uint8_t OUTPUT_COUNT = 0;

// LED (onboard status LED mirrors progress; see main.cpp)
static const bool LED_ACTIVE_LOW = false;
static const int LED_MIRROR_SENSOR = -1;
