#pragma once
// =====================================================
// Prop: Test Magnet (Magic Room)
// =====================================================

// Identity
static const char* SITE_ID     = "default";
static const char* ROOM_ID     = "magie";
static const char* DEVICE_ID   = "magie_test_magnet";
static const char* DEVICE_NAME = "Test Magnet";

// Sensors
// WARNING: GPIO 12 is an ESP32 strapping pin. If pulled HIGH at boot, the chip
// may fail to start (wrong flash voltage). Prefer GPIO 13, 14, 25, 26, 32, 33.
static const SensorDef SENSORS[] = {
  //  id         pin  presentWhen              actionEvent       needsArming
  { "magnet1",   12,  PresentWhen::LOW_LEVEL,  "magnet_present", false },
};
static constexpr uint8_t SENSOR_COUNT = sizeof(SENSORS) / sizeof(SENSORS[0]);
static constexpr SolveMode SOLVE_MODE = SolveMode::ALL;

// No outputs
static const OutputDef OUTPUTS[] = {};
static constexpr uint8_t OUTPUT_COUNT = 0;

// LED
static const bool LED_ACTIVE_LOW = false;
static const int LED_MIRROR_SENSOR = 0;  // magnet1
