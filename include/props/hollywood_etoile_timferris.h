#pragma once
// =====================================================
// Prop: Étoile Tim Ferris (Hollywood Room)
// 7 reed switches — all 7 magnets must be placed
// (no order required) to solve. Opens trapdoor maglock.
// =====================================================

// Identity
static const char* SITE_ID     = "ey1";
static const char* ROOM_ID     = "hollywood";
static const char* DEVICE_ID   = "hollywood_etoile_timferris";
static const char* DEVICE_NAME = "Étoile Tim Ferris";

// Static IP
static const IPAddress STATIC_IP(192, 168, 2, 204);

// Sensors — 7 reed switches wired between pin and GND
// INPUT_PULLUP: pin reads HIGH when no magnet, LOW when magnet present
static const SensorDef SENSORS[] = {
  //  id          pin  presentWhen              actionEvent        needsArming
  { "magnet1",    13,  PresentWhen::LOW_LEVEL,  "magnet_present",  true },
  { "magnet2",    14,  PresentWhen::LOW_LEVEL,  "magnet_present",  true },
  { "magnet3",    17,  PresentWhen::LOW_LEVEL,  "magnet_present",  true },
  { "magnet4",    26,  PresentWhen::LOW_LEVEL,  "magnet_present",  true },
  { "magnet5",    27,  PresentWhen::LOW_LEVEL,  "magnet_present",  true },
  { "magnet6",    32,  PresentWhen::LOW_LEVEL,  "magnet_present",  true },
  { "magnet7",    33,  PresentWhen::LOW_LEVEL,  "magnet_present",  true },
};
static constexpr uint8_t SENSOR_COUNT = sizeof(SENSORS) / sizeof(SENSORS[0]);
static constexpr SolveMode SOLVE_MODE = SolveMode::ALL;  // all 7 magnets required

// Outputs — trapdoor maglock
static const OutputDef OUTPUTS[] = {
  //  id           pin  activeLow
  { "maglock1",    25,  false },  // XY-MOS module: HIGH = ON (active-high)
};
static constexpr uint8_t OUTPUT_COUNT = sizeof(OUTPUTS) / sizeof(OUTPUTS[0]);

// LED
static const bool LED_ACTIVE_LOW = false;
static const int LED_MIRROR_SENSOR = -1;  // no single sensor to mirror (7 inputs)
