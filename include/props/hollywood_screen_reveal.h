#pragma once
// =====================================================
// Prop: Screen Reveal (Hollywood Room)
// =====================================================
// Players type a 6-digit code on a TV remote. ESP32 receives
// IR presses and publishes each digit via MQTT (dumb reader —
// Room Controller validates the code). On solve, ESP32 sends a
// 433MHz RF signal to open the rollable screen that hides the
// Secret HQ room. On reset, sends DOWN to close it.
//
// Codes and protocol were captured from the original remotes
// (see memory/prop_screen_reveal.md).

// Identity
static const char* SITE_ID     = "ey1";
static const char* ROOM_ID     = "hollywood";
static const char* DEVICE_ID   = "hollywood_screen_reveal";
static const char* DEVICE_NAME = "Screen Reveal";

// Static IP
static const IPAddress STATIC_IP(192, 168, 2, 203);

// No physical sensors — IR handled by EY_IR module
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
// IR receiver (VS1838B)
// =====================================================
#define HAS_IR

static constexpr uint8_t IR_RECV_PIN = 15;

// NEC 32-bit codes captured from the player-facing TV remote.
// Mapped to digit characters (power button clears the buffer).
struct IRKeyMap {
  uint32_t code;
  char     key;  // '0'..'9' for digits, 'C' for clear (power)
};

static const IRKeyMap IR_KEY_MAP[] = {
  { 0xFEA857, 'C' },  // Power = clear buffer
  { 0xFE807F, '1' },
  { 0xFE40BF, '2' },
  { 0xFEC03F, '3' },
  { 0xFE20DF, '4' },
  { 0xFEA05F, '5' },
  { 0xFE609F, '6' },
  { 0xFEE01F, '7' },
  { 0xFE10EF, '8' },
  { 0xFE906F, '9' },
  { 0xFE00FF, '0' },
};
static constexpr uint8_t IR_KEY_MAP_LEN =
  sizeof(IR_KEY_MAP) / sizeof(IR_KEY_MAP[0]);

// =====================================================
// 433MHz RF transmitter (FS1000A)
// =====================================================
#define HAS_RF433

static constexpr uint8_t RF433_TX_PIN = 4;

// rc-switch parameters (captured from screen remote: protocol 1, 24-bit, 401us)
static constexpr uint8_t  RF433_PROTOCOL     = 1;
static constexpr uint16_t RF433_PULSE_LENGTH = 401;
static constexpr uint8_t  RF433_BITS         = 24;
static constexpr uint8_t  RF433_REPEATS      = 10;

// Captured codes for the rollable screen
static constexpr unsigned long RF_CODE_UP   = 8320770;
static constexpr unsigned long RF_CODE_STOP = 8320776;
static constexpr unsigned long RF_CODE_DOWN = 8320772;

// =====================================================
// PIR motion sensor (HC-SR501) — HQ entry detector
// =====================================================
// Detects the first player entering the HQ room after the screen rolls up,
// so the Room Controller can start the welcome video sequence.
#define HAS_PIR

static constexpr uint8_t PIR_PIN = 14;
