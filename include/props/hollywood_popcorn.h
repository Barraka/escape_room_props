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
// Each outputs HIGH when its cup's RFID chip is present
static const SensorDef SENSORS[] = {
  //  id         pin  presentWhen              actionEvent       needsArming
  { "cup1",      13,  PresentWhen::HIGH_LEVEL, "rfid_present",   true },
  { "cup2",      14,  PresentWhen::HIGH_LEVEL, "rfid_present",   true },
  { "cup3",      26,  PresentWhen::HIGH_LEVEL, "rfid_present",   true },
  { "cup4",      27,  PresentWhen::HIGH_LEVEL, "rfid_present",   true },
  { "cup5",      32,  PresentWhen::HIGH_LEVEL, "rfid_present",   true },
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
