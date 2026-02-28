#pragma once
// =====================================================
// Prop: Shaker (Hollywood Room)
// =====================================================
// Players shake a bar cocktail shaker containing a small
// 433MHz wireless vibration sensor. A nearby ESP32 with
// a 433MHz receiver accumulates shake time and releases
// a maglock (trap door) once the target is reached.

// Identity
static const char* SITE_ID     = "ey1";
static const char* ROOM_ID     = "hollywood";
static const char* DEVICE_ID   = "hollywood_shaker";
static const char* DEVICE_NAME = "Shaker";

// Static IP
static const IPAddress STATIC_IP(192, 168, 2, 194);

// Sensors
// The 433MHz receiver DATA pin goes HIGH when the vibration
// sensor inside the shaker transmits. We define it as a sensor
// so EY_Sensors knows about it (for status reporting), but
// solve logic is handled by EY_Shaker instead of EY_Sensors.
static const SensorDef SENSORS[] = {
  //  id       pin  presentWhen              actionEvent        needsArming
  { "shake",   13,  PresentWhen::HIGH_LEVEL, "shake_detected",  false },
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
static const int LED_MIRROR_SENSOR = -1;  // No direct mirror — LED controlled by shaker progress

// =====================================================
// Shaker-specific constants
// =====================================================
#define HAS_SHAKER

// Total accumulated shake time needed to solve (ms)
static constexpr unsigned long SHAKE_TARGET_MS = 4000;

// How fast accumulated credit decays when not shaking (ms lost per second)
static constexpr float SHAKE_DECAY_PER_SEC = 500.0f;

// How often to publish shake progress via MQTT (ms)
static constexpr unsigned long SHAKE_REPORT_INTERVAL_MS = 500;
