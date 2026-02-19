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
| `src/EY_Sensors.cpp` | Sensor polling and trigger detection | No |
| `src/EY_Outputs.cpp` | Output pin control (arm/release) | No |
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

- **WiFi**: SFR_3474
- **MQTT Broker**: Raspberry Pi at `192.168.1.10:1883`
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
| Roue de la Fortune | `magie_roueFortune` | 192.168.1.193 | rfid1 (pin 12), magnet1 (pin 27) | maglock1 (pin 25) |
| Test Magnet | `magie_test_magnet` | 192.168.1.102 | magnet1 (pin 12) | None |

Full topic example: `ey/default/magie/prop/magie_roueFortune/status`

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
```

### Flash via USB
```bash
pio run -e magie_roueFortune --target upload
```

### Flash via OTA (WiFi)
```bash
pio run -e magie_roueFortune-ota --target upload
# Or override IP:
pio run -e magie_roueFortune-ota --target upload --upload-port 192.168.1.193
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
mosquitto_sub -h localhost -t "ey/default/magie/prop/magie_roueFortune/#" -v
```

### Check Mosquitto Status on Pi
```bash
sudo systemctl status mosquitto
```

## Setting Up a New Prop

1. Create a new file in `include/props/` (e.g., `magie_newProp.h`)
   - Copy an existing prop file as template
   - Set `DEVICE_ID`, `DEVICE_NAME`, `STATIC_IP`
   - Define `SENSORS[]` with pin numbers and polarities
   - Define `OUTPUTS[]` if the prop controls maglocks/relays
   - Set `SOLVE_MODE` to `ANY` or `ALL`
2. Add PlatformIO environments in `platformio.ini`:
   ```ini
   [env:magie_newProp]
   build_flags = -DPROP_CONFIG=\"props/magie_newProp.h\"

   [env:magie_newProp-ota]
   extends = env:magie_newProp
   upload_protocol = espota
   upload_port = 192.168.1.XXX
   upload_flags =
     --auth=escapeyourself
   ```
3. Flash to ESP32 via USB: `pio run -e magie_newProp --target upload`
4. Subsequent updates can use OTA: `pio run -e magie_newProp-ota --target upload`

### Sensor Polarity Quick Reference

| Sensor Type | PresentWhen | Wiring |
|-------------|-------------|--------|
| Reed switch (to GND) | `LOW_LEVEL` | Switch between pin and GND, INPUT_PULLUP |
| RFID (high when tag) | `HIGH_LEVEL` | Module outputs HIGH when tag present |
| Button (to GND) | `LOW_LEVEL` | Button between pin and GND, INPUT_PULLUP |
| IR break-beam | `HIGH_LEVEL` | Output HIGH when beam broken (typical) |

## Known Issues / Notes

- **MQTT blocking**: If the MQTT broker is unreachable, `s_mqtt.connect()` can block for 1-2 seconds. The LED reading was moved to the top of `loop()` to ensure instant sensor response regardless of network state.
- **Error code -2**: Means MQTT broker is unreachable (check Pi is on, Mosquitto running)

## Recent Changes

### v1.4.0 — NTP Time Sync
- **Real timestamps**: MQTT messages now include Unix epoch timestamps (seconds since 1970) instead of `millis()`
- **Automatic NTP sync**: `configTime()` runs once after WiFi connects, syncs from router (`192.168.1.1`) or `pool.ntp.org`
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
