#pragma once
// =====================================================
// Prop: Walk of Fame (Hollywood Room)
// 5 RFID readers under star slots — all 5 stars must
// be placed to solve. Opens trapdoor maglock.
// =====================================================

// Identity
static const char* SITE_ID     = "ey1";
static const char* ROOM_ID     = "hollywood";
static const char* DEVICE_ID   = "hollywood_walkoffame";
static const char* DEVICE_NAME = "Walk of Fame";

// Static IP
static const IPAddress STATIC_IP(192, 168, 2, 196);

// Sensors — 5 standalone RFID readers (3-wire: VCC, GND, Signal)
// These modules emit a momentary HIGH pulse when a chip is read (they do NOT
// hold the line), so each star is marked `latching`: once read, it stays
// "present" until reset — both for the dashboard and the ALL-solve.
static const SensorDef SENSORS[] = {
  //  id         pin  presentWhen              actionEvent       needsArming  decorative  latching   // star
  { "star1",     13,  PresentWhen::HIGH_LEVEL, "rfid_present",   true,        false,      true },     // Spielberg
  { "star2",     14,  PresentWhen::HIGH_LEVEL, "rfid_present",   true,        false,      true },     // Simpsons
  { "star3",     26,  PresentWhen::HIGH_LEVEL, "rfid_present",   true,        false,      true },     // Dion
  { "star4",     27,  PresentWhen::HIGH_LEVEL, "rfid_present",   true,        false,      true },     // Armstrong
  { "star5",     32,  PresentWhen::HIGH_LEVEL, "rfid_present",   true,        false,      true },     // Ali
};
static constexpr uint8_t SENSOR_COUNT = sizeof(SENSORS) / sizeof(SENSORS[0]);
static constexpr SolveMode SOLVE_MODE = SolveMode::ALL;  // all 5 stars required

// Outputs — trapdoor maglock
static const OutputDef OUTPUTS[] = {
  //  id           pin  activeLow
  { "maglock1",    25,  false },  // XY-MOS module: HIGH = ON (active-high)
};
static constexpr uint8_t OUTPUT_COUNT = sizeof(OUTPUTS) / sizeof(OUTPUTS[0]);

// LED
static const bool LED_ACTIVE_LOW = false;
static const int LED_MIRROR_SENSOR = -1;  // no single sensor to mirror (5 inputs)
