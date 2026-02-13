#pragma once
// =====================================================
// Prop: Roue de la Fortune (Magic Room)
// =====================================================

// Identity
static const char* SITE_ID     = "default";
static const char* ROOM_ID     = "magie";
static const char* DEVICE_ID   = "magie_roueFortune";
static const char* DEVICE_NAME = "Roue de la Fortune";

// Sensors
// WARNING: GPIO 12 is an ESP32 strapping pin. If pulled HIGH at boot, the chip
// may fail to start (wrong flash voltage). Prefer GPIO 13, 14, 25, 26, 32, 33.
static const SensorDef SENSORS[] = {
  //  id         pin  presentWhen              actionEvent       needsArming
  { "rfid1",     12,  PresentWhen::HIGH_LEVEL, "rfid_present",   true  },
  { "magnet1",   27,  PresentWhen::LOW_LEVEL,  "magnet_present", false },
};
static constexpr uint8_t SENSOR_COUNT = sizeof(SENSORS) / sizeof(SENSORS[0]);
static constexpr SolveMode SOLVE_MODE = SolveMode::ANY;

// Outputs
static const OutputDef OUTPUTS[] = {
  //  id           pin  activeLow
  { "maglock1",    25,  false },  // XY-MOS module: HIGH = ON (active-high)
};
static constexpr uint8_t OUTPUT_COUNT = sizeof(OUTPUTS) / sizeof(OUTPUTS[0]);

// LED
static const bool LED_ACTIVE_LOW = false;
static const int LED_MIRROR_SENSOR = 1;  // magnet1
