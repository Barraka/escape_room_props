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
static const SensorDef SENSORS[] = {
  //  id         pin  presentWhen              actionEvent       needsArming
  { "rfid1",     13,  PresentWhen::HIGH_LEVEL, "rfid_present",   true },
  { "rfid2",     14,  PresentWhen::HIGH_LEVEL, "rfid_present",   true },
  { "rfid3",     26,  PresentWhen::HIGH_LEVEL, "rfid_present",   true },
  { "rfid4",     27,  PresentWhen::HIGH_LEVEL, "rfid_present",   true },
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

// =====================================================
// Bobine (ceiling film-reel) — 6 MOSFET-switched puck lights
// =====================================================
// On solve, lights reveal a 6-digit code one puck at a time,
// then pauses and loops forever until reset.
//
// Wiring: 12V PSU (+) → puck (+), puck (−) → MOSFET drain,
// MOSFET source → 12V PSU (−) tied to ESP32 GND.
// ESP32 GPIO → MOSFET gate (via 220Ω), 10kΩ gate-to-GND pulldown.
#define HAS_BOBINE

static const uint8_t BOBINE_PUCK_PINS[] = { 4, 5, 16, 17, 18, 19 };
static constexpr uint8_t BOBINE_PUCK_COUNT =
  sizeof(BOBINE_PUCK_PINS) / sizeof(BOBINE_PUCK_PINS[0]);

// MOSFET: GPIO HIGH turns puck on (true = active-high).
static constexpr bool BOBINE_ACTIVE_HIGH = true;

// Sequence of puck indices to light, in order. The 6 digits
// revealed by the sequence form the code. Placeholder below
// lights pucks in index order — adjust once the code is set.
static const uint8_t BOBINE_SEQUENCE[] = { 0, 1, 2, 3, 4, 5 };
static constexpr uint8_t BOBINE_SEQUENCE_LENGTH =
  sizeof(BOBINE_SEQUENCE) / sizeof(BOBINE_SEQUENCE[0]);

static constexpr uint32_t BOBINE_ON_MS    = 2000;  // per-puck on-time
static constexpr uint32_t BOBINE_GAP_MS   = 1000;  // gap between pucks
static constexpr uint32_t BOBINE_PAUSE_MS = 5000;  // pause before repeat
