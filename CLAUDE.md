# EY Escape Room Prop System - Project Context

## Overview

This is an ESP32-based escape room prop controller system. Each prop monitors physical sensors (RFID readers, magnet/reed switches, buttons) and communicates with a central room controller via MQTT.

## Architecture

```
ESP32 Prop ← MQTT → Raspberry Pi/MiniPC ← WebSocket → GM Dashboard
(this code)        (room controller)                   (React UI)
```

### Authority Model
- **ESP32**: Handles physical I/O and local solve detection
- **MiniPC/Pi**: Owns session logic, analytics, persistence, cross-prop coordination
- **GM Dashboard**: UI only (cannot infer truth)

## Project Location

```
c:\02 - GM Manager\EY_Prop_Base_PIO\
```

## File Structure

| File | Purpose | Edit? |
|------|---------|-------|
| `include/EY_Config.h` | Shared configuration (WiFi, MQTT, OTA, pins, timing) | Rarely |
| `include/props/*.h` | Per-prop config (identity, sensors, outputs, static IP) | **YES - one file per prop** |
| `include/EY_Types.h` | Shared type definitions (enums, structs) | No |
| `include/EY_Sensors.h` | Sensor API interface | No |
| `include/EY_Mqtt.h` | MQTT and networking API | No |
| `include/EY_Outputs.h` | Output pin API (maglocks, relays) | No |
| `include/EY_Shaker.h` | Shaker module API (433MHz shake detection) | No |
| `include/EY_Wiegand.h` | Wiegand keypad reader API | No |
| `src/EY_Sensors.cpp` | Sensor polling and trigger detection | No |
| `src/EY_Outputs.cpp` | Output pin control (arm/release) | No |
| `src/EY_Shaker.cpp` | Shaker module implementation (guarded by `#ifdef HAS_SHAKER`) | No |
| `src/EY_Wiegand.cpp` | Wiegand reader implementation (guarded by `#ifdef HAS_WIEGAND`) | No |
| `src/EY_Mqtt.cpp` | MQTT communication layer | No |
| `src/main.cpp` | Application entry point, LED feedback, reset logic | Rarely (custom behavior only) |
| `platformio.ini` | PlatformIO environments (one per prop + OTA variants) | When adding a new prop |

## Hardware

- **Board**: FM-DevKit (ESP32-based)
- **Framework**: Arduino via PlatformIO
- **Dependencies**: PubSubClient (MQTT), ArduinoJson

### Common Pin Configuration
- **GPIO 2**: Onboard blue LED (feedback)
- **GPIO 0**: BOOT button (long-press for reset)

Per-prop pin assignments are defined in each prop's config file (`include/props/*.h`).

## Network Configuration

- **WiFi**: Hollywood (password: Escape37) — TP-Link AC1200, channel 1
- **MQTT Broker**: Raspberry Pi at `192.168.2.10:1883`
- **MQTT Software**: Mosquitto (enabled, auto-starts on Pi boot)

## MQTT Topics (Contract v1.0)

Base: `ey/<site>/<room>/prop/<propId>/`

| Suffix | Retained | Purpose |
|--------|----------|---------|
| `/status` | Yes | Current state (solved, online, override) |
| `/event` | No | Sensor triggers, one-shot actions |
| `/cmd` | No | Commands from room controller |

### Current Props

| Prop | Device ID | Static IP | Sensors | Outputs |
|------|-----------|-----------|---------|---------|
| Roue de la Fortune | `magie_roueFortune` | 192.168.2.193 | rfid1 (pin 12), magnet1 (pin 27) | maglock1 (pin 25) |
| Test Magnet | `magie_test_magnet` | 192.168.2.102 | magnet1 (pin 12) | None |
| Shaker | `hollywood_shaker` | 192.168.2.194 | shake (ESP-NOW from ESP32-C3+MPU6050) | maglock1 (pin 25) |
| Gadgets Pinpad | `hollywood_gadgets_pinpad` | 192.168.2.195 | Wiegand D0 (pin 26), D1 (pin 27) | None |

Full topic example: `ey/ey1/hollywood/prop/magie_roueFortune/status`

## Current Behavior

1. **LED mirrors magnet sensor in real-time** (custom modification in main.cpp)
   - Magnet present (GPIO 27 LOW) → LED ON
   - Magnet absent → LED OFF
   - This happens at the START of the loop for instant response

2. **Solve Logic**: Either RFID or magnet triggers "solved" state (`SolveMode::ANY`)

3. **MQTT Events**: Published when sensors trigger and when solved state changes

4. **Reset**: Hold BOOT button for 1 second to reset puzzle state

## Useful Commands

### Build a Specific Prop
```bash
pio run -e magie_roueFortune
pio run -e magie_test_magnet
pio run -e hollywood_shaker
pio run -e hollywood_gadgets_pinpad
```

### Flash via USB
```bash
pio run -e magie_roueFortune --target upload
```

### Flash via OTA (WiFi)
```bash
pio run -e magie_roueFortune-ota --target upload
# Or override IP:
pio run -e magie_roueFortune-ota --target upload --upload-port 192.168.2.193
```

### Monitor Serial Output
```bash
pio device monitor
```

### Monitor MQTT on Raspberry Pi
```bash
# All messages from all props
mosquitto_sub -h localhost -t "ey/#" -v

# Just one prop
mosquitto_sub -h localhost -t "ey/ey1/hollywood/prop/magie_roueFortune/#" -v
```

### Check Mosquitto Status on Pi
```bash
sudo systemctl status mosquitto
```

## Setting Up a New Prop

1. Create a new file in `include/props/` (e.g., `hollywood_newProp.h`)
   - Copy an existing prop file as template
   - Set `DEVICE_ID`, `DEVICE_NAME`, `STATIC_IP`
   - Define `SENSORS[]` with pin numbers and polarities
   - Define `OUTPUTS[]` if the prop controls maglocks/relays
   - Set `SOLVE_MODE` to `ANY` or `ALL`
2. Add PlatformIO environments in `platformio.ini`:
   ```ini
   [env:hollywood_newProp]
   build_flags = -DPROP_CONFIG=\"props/hollywood_newProp.h\"

   [env:hollywood_newProp-ota]
   extends = env:hollywood_newProp
   upload_protocol = espota
   upload_port = 192.168.2.XXX
   upload_flags =
     --auth=escapeyourself
   ```
3. Flash to ESP32 via USB: `pio run -e hollywood_newProp --target upload`
4. Subsequent updates can use OTA: `pio run -e hollywood_newProp-ota --target upload`

### Sensor Polarity Quick Reference

| Sensor Type | PresentWhen | Wiring |
|-------------|-------------|--------|
| Reed switch (to GND) | `LOW_LEVEL` | Switch between pin and GND, INPUT_PULLUP |
| RFID (high when tag) | `HIGH_LEVEL` | Module outputs HIGH when tag present |
| Button (to GND) | `LOW_LEVEL` | Button between pin and GND, INPUT_PULLUP |
| IR break-beam | `HIGH_LEVEL` | Output HIGH when beam broken (typical) |
| 433MHz receiver | `HIGH_LEVEL` | DATA pin HIGH when transmitter active |

## Known Issues / Notes

- **MQTT blocking**: If the MQTT broker is unreachable, `s_mqtt.connect()` can block for 1-2 seconds. The LED reading was moved to the top of `loop()` to ensure instant sensor response regardless of network state.
- **Error code -2**: Means MQTT broker is unreachable (check Pi is on, Mosquitto running)
- **433MHz RXB6 receivers are unusable for presence detection**: AGC amplifies noise to match signal levels when no strong signal present. Pulse-width filtering doesn't help — noise pulses fall in the same 100–2000µs range as real OOK signals. Use a learning receiver (hardware code matching) or switch to 2.4GHz digital (ESP-NOW / nRF24L01+) instead.

## Custom Modules

### Shaker (`EY_Shaker.h` / `EY_Shaker.cpp`)

First custom module extending the base firmware with prop-specific logic.

- **Feature guard**: `#define HAS_SHAKER` in prop config — enables conditional compilation in `main.cpp`, `EY_Mqtt.cpp`, and `EY_Shaker.cpp`
- **Hardware (planned)**: ESP32-C3 SuperMini + MPU6050 accelerometer inside the shaker, communicating via **ESP-NOW** (2.4GHz peer-to-peer) to the main ESP32. No extra receiver hardware needed — ESP32 has built-in 2.4GHz radio.
  - Previous 433MHz approach (RXB6 receiver) abandoned — AGC noise indistinguishable from real signals
  - Plan A (in progress): QIACHIP 433MHz learning receiver as simpler alternative (ordered, awaiting delivery)
  - Plan C (fallback): ESP32-C3 + MPU6050 + ESP-NOW + LiPo battery inside shaker
- **Algorithm**: Accumulates shake time, decays when idle, solves at configurable target
- **MQTT**: Publishes `shakeProgress` (0-100) in `details` of status messages
- **LED**: Blinks proportionally to progress (faster = closer to solved), solid when solved
- **Tuning**: `SHAKE_TARGET_MS`, `SHAKE_DECAY_PER_SEC`, `SHAKE_REPORT_INTERVAL_MS` in prop config
- **Code status**: `EY_Shaker.cpp` currently has pulse-width filtered edge counting (diagnostic mode, threshold=9999). Will be rewritten for either digitalRead (Plan A) or ESP-NOW receive callback (Plan C) once hardware arrives.

**Pattern for future custom modules**: Define a feature guard (`#define HAS_XXX`) in the prop config, wrap the `.cpp` in `#ifdef`, add `#ifdef` blocks in `main.cpp` for init/tick/reset, and optionally in `EY_Mqtt.cpp` for extra status fields.

### Wiegand Keypad Reader (`EY_Wiegand.h` / `EY_Wiegand.cpp`)

Reads Wiegand keypads via interrupt-driven D0/D1 capture. Used by the Gadgets Pinpad prop.

- **Feature guard**: `#define HAS_WIEGAND` in prop config
- **Auto-detect**: Supports 4-bit (key-by-key), 8-bit (key + complement), and 26-bit (full Wiegand) frames
- **4-bit mode**: Accumulates digits in a buffer; `*` clears, `#` submits
- **26-bit mode**: Extracts 16-bit card number and publishes directly
- **MQTT events**: `keypress` (each digit), `code_clear` (`*` pressed), `code_entered` (with `"code"` field in payload)
- **No solve logic**: This prop never becomes "solved" — it's a dumb reader. The Pi-side puzzle logic validates codes.
- **Wiring**: D0 (green) → GPIO 26, D1 (white) → GPIO 27, 12V power + shared GND

## Recent Changes

### v1.6.0 — Wiegand Pinpad Module
- **EY_Wiegand module**: New custom module for Wiegand keypad reading (`EY_Wiegand.h`, `EY_Wiegand.cpp`)
- **Gadgets Pinpad prop**: `hollywood_gadgets_pinpad` config, static IP 192.168.2.195, no sensors/outputs
- **EY_PublishEventWithData()**: New generic MQTT helper for events with extra key/value pairs (e.g. `"code": "4527"`)
- **Feature guard pattern**: `#define HAS_WIEGAND` — second custom module following the shaker pattern

### v1.5.0 — Shaker Module & Feature Guards
- **EY_Shaker module**: New custom module for shake detection (`EY_Shaker.h`, `EY_Shaker.cpp`)
- **Feature guard pattern**: `#define HAS_SHAKER` enables conditional compilation — first use of `#ifdef` in the codebase
- **Shaker prop**: `hollywood_shaker` config with maglock on GPIO 25, static IP 192.168.2.194
- **MQTT status extension**: `details.shakeProgress` (0-100) published for shaker props
- **LED progress feedback**: Blink rate scales with shake progress (1000ms→100ms)
- **433MHz abandoned**: RXB6 receiver AGC noise is indistinguishable from real signals at software level. Tried: raw edge counting, rc-switch code decoding, pulse-width filtered edge counting — all failed.
- **New approach**: Plan A = QIACHIP 433MHz learning receiver (ordered). Plan C = ESP32-C3 SuperMini + MPU6050 accelerometer inside shaker, communicating via ESP-NOW to main ESP32 (no receiver hardware needed)

### v1.4.0 — NTP Time Sync
- **Real timestamps**: MQTT messages now include Unix epoch timestamps (seconds since 1970) instead of `millis()`
- **Automatic NTP sync**: `configTime()` runs once after WiFi connects, syncs from router (`192.168.2.1`) or `pool.ntp.org`
- **Graceful fallback**: If NTP is unavailable (no internet, no router NTP), falls back to `millis()` transparently
  - Room controller can distinguish: epoch values are > 1,000,000,000; millis() values are much smaller
- **UTC timestamps**: All times are UTC (timezone offset = 0); room controller handles local time conversion
- **NTP config**: Server addresses and timezone in `EY_Config.h`

### v1.3.0 — Per-Prop Config & Static IPs
- **Per-prop config files**: Moved prop-specific config (identity, sensors, outputs, static IP) from `EY_Config.h` to individual files in `include/props/`
- **PlatformIO multi-environment**: Each prop has its own build environment + OTA variant in `platformio.ini`
- **Static IPs**: Each ESP32 gets a fixed IP via `WiFi.config()` for deterministic addressing
- **ArduinoOTA**: Over-the-air flashing via WiFi (deferred init — `.begin()` called in `loop()` after WiFi connects to avoid crash)
  - Password-protected (shared password in `EY_Config.h`)
  - Hostname set to `DEVICE_ID` for mDNS discovery

### v1.2.0 — Output Pin Support
- **Output pins** (maglocks, relays): New `EY_Outputs` module with arm/release lifecycle
  - Outputs arm on solve, release on reset
  - GM can trigger individual outputs via `set_output` command
- **Output-level reporting**: Status messages include `details.outputs` array with state (inactive/armed/released)
- **MQTT buffer size**: Increased to 768 bytes to accommodate sensor + output details

### v1.1.0 — Audit Bug Fix Pass
- **MQTT field names**: Aligned with Contract v1.0 — `deviceId` → `propId`, `ts` → `timestamp`
- **LWT topic**: Moved from `/status` to dedicated `/lwt` topic for clean separation
- **DEVICE_NAME**: Added to config and included in status messages for auto-discovery
- **Sensor debouncing**: Added configurable `DEBOUNCE_MS` (20ms default) to prevent false triggers from mechanical noise
- **Force-trigger locking**: GM-triggered sensors now stay locked (`forceLocked`) until reset
- **LED mirror configurable**: Via `LED_MIRROR_SENSOR` index instead of hardcoded GPIO
- **Millis overflow safety**: Replaced `millis() < deadline` with `millis() - start >= duration`
- **GPIO 12 strapping pin warning**: Added comment warning about ESP32 boot behavior
- **Remote sensor triggering**: `set_output` command forces sensor to triggered state
- **Sensor-level reporting**: Status messages include `details.sensors` array

## Related Projects

- **Room Controller**: `c:\02 - GM Manager\room-controller\` (Node.js, MQTT + WebSocket)
- **GM Dashboard**: `c:\02 - GM Manager\escapeRoomManager\` (React)

## Related Documentation

- `MQTT_CONTRACT_v1.md` - Full MQTT message specification (frozen v1.0)
- `c:\02 - GM Manager\WEBSOCKET_CONTRACT_v1.md` - Room Controller ↔ Dashboard protocol
- Architecture PDF on Desktop: `c:\Users\Manu\Desktop\EY Prop Architecture V2.pdf`
