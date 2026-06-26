#pragma once
// =====================================================
// Prop: The Oscars (Hollywood Room)
// 4 RFID readers — all 4 must be placed to solve.
// Opens trapdoor maglock.
// =====================================================

// Identity
static const char* SITE_ID     = "ey1";
static const char* ROOM_ID     = "hollywood";
static const char* DEVICE_ID   = "hollywood_oscars";
static const char* DEVICE_NAME = "The Oscars";

// Static IP
static const IPAddress STATIC_IP(192, 168, 2, 201);

// Sensors — 4 standalone RFID readers (3-wire: VCC, GND, Signal)
// These modules HOLD their signal line HIGH the whole time a tag sits on the reader
// (confirmed on the identical Poker readers 2026-06-26), so `latching` is OFF: each
// reader reports LIVE presence so the dashboard shows which chips are physically
// placed right now (clears on removal). ALL-solve requires all 4 present
// SIMULTANEOUSLY.
static const SensorDef SENSORS[] = {
  //  id         pin  presentWhen              actionEvent       needsArming  decorative  latching
  { "rfid1",     13,  PresentWhen::HIGH_LEVEL, "rfid_present",   true,        false,      false },
  { "rfid2",     14,  PresentWhen::HIGH_LEVEL, "rfid_present",   true,        false,      false },
  { "rfid3",     26,  PresentWhen::HIGH_LEVEL, "rfid_present",   true,        false,      false },
  { "rfid4",     27,  PresentWhen::HIGH_LEVEL, "rfid_present",   true,        false,      false },
};
static constexpr uint8_t SENSOR_COUNT = sizeof(SENSORS) / sizeof(SENSORS[0]);
static constexpr SolveMode SOLVE_MODE = SolveMode::ALL;  // all 4 required

// Outputs — trapdoor maglock
static const OutputDef OUTPUTS[] = {
  //  id           pin  activeLow
  { "maglock1",    25,  false },  // XY-MOS module: HIGH = ON (active-high)
};
static constexpr uint8_t OUTPUT_COUNT = sizeof(OUTPUTS) / sizeof(OUTPUTS[0]);

// LED
static const bool LED_ACTIVE_LOW = false;
static const int LED_MIRROR_SENSOR = -1;

// The ceiling film-reel ("bobine") that displays the 6-digit code
// for the screen reveal lives on its own ESP32 — see
// include/props/hollywood_bobine.h. The Room Controller starts the
// light sequence via an mqtt_cmd scenario when this prop solves.
