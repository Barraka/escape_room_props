#pragma once
// =====================================================
// Prop: Popcorn Cups (Hollywood Room)
// 5 RFID readers under cup slots — all 5 cups must
// be placed to solve. Opens trapdoor maglock.
// =====================================================

// Identity
static const char* SITE_ID     = "ey1";
static const char* ROOM_ID     = "hollywood";
static const char* DEVICE_ID   = "hollywood_popcorn";
static const char* DEVICE_NAME = "Popcorn Cups";

// Static IP
static const IPAddress STATIC_IP(192, 168, 2, 197);

// Sensors — 5 standalone RFID readers (3-wire: VCC, GND, Signal)
// These modules HOLD their signal line the whole time a tag sits on the reader
// (confirmed on the identical Poker readers 2026-06-26 — LED stays solid while
// present), so `latching` is OFF: each reader reports LIVE presence so the dashboard
// shows which cups are physically placed right now (clears on removal). ALL-solve
// then requires all 5 present SIMULTANEOUSLY.
// Wiring is DIRECT 5V (same as Poker / Coiffeuse / Walk of Fame, all working):
// readers powered at 5V, signal straight to the GPIO — chip present drives the line
// HIGH, hence PresentWhen::HIGH_LEVEL. The 12V/optocoupler "plan B" was ABANDONED
// 2026-06-26: the earlier 5V flakiness was a bad shared VCC/GND joint (re-wired on
// the prop side), not a voltage problem; the optos would have INVERTED the logic
// (the brief LOW_LEVEL setting made the dashboard read every cup backwards).
static const SensorDef SENSORS[] = {
  //  id         pin  presentWhen               actionEvent       needsArming  decorative  latching
  { "cup1",      13,  PresentWhen::HIGH_LEVEL,  "rfid_present",   true,        false,      false },
  { "cup2",      14,  PresentWhen::HIGH_LEVEL,  "rfid_present",   true,        false,      false },
  { "cup3",      26,  PresentWhen::HIGH_LEVEL,  "rfid_present",   true,        false,      false },
  { "cup4",      27,  PresentWhen::HIGH_LEVEL,  "rfid_present",   true,        false,      false },
  { "cup5",      32,  PresentWhen::HIGH_LEVEL,  "rfid_present",   true,        false,      false },
};
static constexpr uint8_t SENSOR_COUNT = sizeof(SENSORS) / sizeof(SENSORS[0]);
static constexpr SolveMode SOLVE_MODE = SolveMode::ALL;  // all 5 cups required

// Outputs — trapdoor maglock
static const OutputDef OUTPUTS[] = {
  //  id           pin  activeLow
  { "maglock1",    25,  false },  // XY-MOS module: HIGH = ON (active-high)
};
static constexpr uint8_t OUTPUT_COUNT = sizeof(OUTPUTS) / sizeof(OUTPUTS[0]);

// LED
static const bool LED_ACTIVE_LOW = false;
static const int LED_MIRROR_SENSOR = -1;  // no single sensor to mirror (5 inputs)
