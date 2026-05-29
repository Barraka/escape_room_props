#pragma once
// =====================================================
// Prop: Bobine (Hollywood Room)
// =====================================================
// Passive display prop — a ceiling "film reel" with 6 puck
// lights, each backlighting an opaque acrylic disc with a
// digit sticker. No sensors, no maglocks: the bobine reacts
// purely to MQTT commands sent by the Room Controller's
// scenario engine.
//
// Flow:
//   1. Oscars solves              → RC sends `start_sequence`
//      → bobine cycles puck-by-puck, looping forever
//        to display the 6-digit code "746281".
//   2. Players enter correct code → RC sends `reveal_all`
//      → bobine stops cycling, all 6 pucks stay solid
//        (full code visible).
//   3. Session reset              → standard `reset` command
//      → all pucks off, ready for next session.
//
// The code itself is validated by hollywood_screen_reveal
// (IR remote → RF UP signal to the rollable screen).

// Identity
static const char* SITE_ID     = "ey1";
static const char* ROOM_ID     = "hollywood";
static const char* DEVICE_ID   = "hollywood_bobine";
static const char* DEVICE_NAME = "Bobine";

// Static IP
static const IPAddress STATIC_IP(192, 168, 2, 205);

// No physical sensors
static const SensorDef SENSORS[] = {
  { "", 0, PresentWhen::HIGH_LEVEL, "", false },  // Placeholder (SENSOR_COUNT=0 prevents access)
};
static constexpr uint8_t SENSOR_COUNT = 0;
static constexpr SolveMode SOLVE_MODE = SolveMode::ANY;

// No outputs
static const OutputDef OUTPUTS[] = {
  { "", 0, false },  // Placeholder (OUTPUT_COUNT=0 prevents access)
};
static constexpr uint8_t OUTPUT_COUNT = 0;

// LED
static const bool LED_ACTIVE_LOW = false;
static const int LED_MIRROR_SENSOR = -1;

// =====================================================
// Bobine (ceiling film-reel) — 6 MOSFET-switched puck lights
// =====================================================
// Each puck is driven by a HW-548 single-channel MOSFET module
// (P-channel F5305S MOSFET + PC817 optocoupler, signal voltage
// 3–20V, load 5–36V). Six modules total, one per puck.
//
// Per-puck wiring (HW-548 terminal → connection):
//   Signal +  (top-left, small)   → ESP32 GPIO (see BOBINE_PUCK_PINS)
//   Signal −  (bottom-left, small)→ ESP32 GND
//   DC +      (bottom-right pair) → 12V PSU (+)
//   DC −      (bottom-right pair) → 12V PSU (−)
//   OUT +     (top-right pair)    → puck (+)
//   OUT −     (top-right pair)    → puck (−)
//
// Optocoupler isolation: the signal side and the 12V load side
// are galvanically isolated, so the ESP32 GND must NOT be tied
// to the 12V PSU GND. Keep the two grounds fully separate.
#define HAS_BOBINE

static const uint8_t BOBINE_PUCK_PINS[] = { 32, 33, 25, 26, 27, 13 };
static constexpr uint8_t BOBINE_PUCK_COUNT =
  sizeof(BOBINE_PUCK_PINS) / sizeof(BOBINE_PUCK_PINS[0]);

// MOSFET: GPIO HIGH turns puck on (true = active-high).
static constexpr bool BOBINE_ACTIVE_HIGH = true;

// Wiring convention — each puck index must sit behind the matching
// digit sticker on the film reel. All six pucks wire to the LEFT
// terminal block (screw terminals D32→D14, six adjacent terminals):
//   puck 0 (GPIO 32 / D32) → "7"
//   puck 1 (GPIO 33 / D33) → "4"
//   puck 2 (GPIO 25 / D25) → "6"
//   puck 3 (GPIO 26 / D26) → "2"
//   puck 4 (GPIO 27 / D27) → "8"
//   puck 5 (GPIO 13 / D13) → "1"
// (D34/D35/VP/VN are input-only; D12 is a boot strapping pin; D14
//  emits a glitch pulse at every boot — all avoided. D13 is clean.)
// With this mapping the sequence is simply { 0, 1, 2, 3, 4, 5 } —
// lighting pucks in index order spells "746281" left-to-right.
static const uint8_t BOBINE_SEQUENCE[] = { 0, 1, 2, 3, 4, 5 };
static constexpr uint8_t BOBINE_SEQUENCE_LENGTH =
  sizeof(BOBINE_SEQUENCE) / sizeof(BOBINE_SEQUENCE[0]);

static constexpr uint32_t BOBINE_ON_MS    = 2000;  // per-puck on-time
static constexpr uint32_t BOBINE_GAP_MS   = 0;     // no gap — direct hand-off to next puck
static constexpr uint32_t BOBINE_PAUSE_MS = 3000;  // pause before repeat
