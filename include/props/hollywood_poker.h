#pragma once
// =====================================================
// Prop: Poker Table (Hollywood Room)
// 5 RFID readers — all 5 must be placed to solve.
// Opens trapdoor maglock.
// =====================================================

// Identity
static const char* SITE_ID     = "ey1";
static const char* ROOM_ID     = "hollywood";
static const char* DEVICE_ID   = "hollywood_poker";
static const char* DEVICE_NAME = "Poker Table";

// Static IP
static const IPAddress STATIC_IP(192, 168, 2, 200);

// Sensors — 5 standalone RFID readers (3-wire: VCC, GND, Signal)
// Each outputs HIGH when its RFID chip is present
static const SensorDef SENSORS[] = {
  //  id         pin  presentWhen              actionEvent       needsArming
  { "rfid1",     13,  PresentWhen::HIGH_LEVEL, "rfid_present",   true },
  { "rfid2",     14,  PresentWhen::HIGH_LEVEL, "rfid_present",   true },
  { "rfid3",     26,  PresentWhen::HIGH_LEVEL, "rfid_present",   true },
  { "rfid4",     27,  PresentWhen::HIGH_LEVEL, "rfid_present",   true },
  { "rfid5",     32,  PresentWhen::HIGH_LEVEL, "rfid_present",   true },
};
static constexpr uint8_t SENSOR_COUNT = sizeof(SENSORS) / sizeof(SENSORS[0]);
static constexpr SolveMode SOLVE_MODE = SolveMode::ALL;  // all 5 required

// Outputs — trapdoor maglock
static const OutputDef OUTPUTS[] = {
  //  id           pin  activeLow
  { "maglock1",    25,  false },  // XY-MOS module: HIGH = ON (active-high)
};
static constexpr uint8_t OUTPUT_COUNT = sizeof(OUTPUTS) / sizeof(OUTPUTS[0]);

// LED
static const bool LED_ACTIVE_LOW = false;
static const int LED_MIRROR_SENSOR = -1;  // no single sensor to mirror
