#pragma once
// =====================================================
// Prop: Gadgets Pinpad (Hollywood Room)
// =====================================================
// A Wiegand keypad mounted on a console. Players type
// gadget codes (from a physical "vitrine") and press #
// to send the gadget to a trapped hero shown on a nearby
// screen. The Pi managing the screen handles puzzle logic;
// this ESP32 is a dumb Wiegand reader that publishes
// entered codes via MQTT.

// Identity
static const char* SITE_ID     = "ey1";
static const char* ROOM_ID     = "hollywood";
static const char* DEVICE_ID   = "hollywood_gadgets_pinpad";
static const char* DEVICE_NAME = "Gadgets Pinpad";

// Static IP
static const IPAddress STATIC_IP(192, 168, 2, 195);

// No physical sensors — Wiegand is handled by EY_Wiegand module
static const SensorDef SENSORS[] = {
  { "", 0, PresentWhen::HIGH_LEVEL, "", false },  // Placeholder (SENSOR_COUNT=0 prevents access)
};
static constexpr uint8_t SENSOR_COUNT = 0;
static constexpr SolveMode SOLVE_MODE = SolveMode::ANY;

// No outputs (no maglocks, no relays)
static const OutputDef OUTPUTS[] = {
  { "", 0, false },  // Placeholder (OUTPUT_COUNT=0 prevents access)
};
static constexpr uint8_t OUTPUT_COUNT = 0;

// LED
static const bool LED_ACTIVE_LOW = false;
static const int LED_MIRROR_SENSOR = -1;

// =====================================================
// Wiegand-specific configuration
// =====================================================
#define HAS_WIEGAND

// Wiegand data pins (D0 = green wire, D1 = white wire)
static constexpr uint8_t WIEGAND_D0_PIN = 26;
static constexpr uint8_t WIEGAND_D1_PIN = 27;
