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
c:\Users\Manu\Documents\PlatformIO\Projects\EY_Prop_Base_PIO\
```

## File Structure

| File | Purpose | Edit? |
|------|---------|-------|
| `include/EY_Config.h` | Per-prop configuration (identity, sensors, WiFi) | **YES - only file to edit for new props** |
| `include/EY_Types.h` | Shared type definitions (enums, structs) | No |
| `include/EY_Sensors.h` | Sensor API interface | No |
| `include/EY_Mqtt.h` | MQTT and networking API | No |
| `src/EY_Sensors.cpp` | Sensor polling and trigger detection | No |
| `src/EY_Mqtt.cpp` | MQTT communication layer | No |
| `src/main.cpp` | Application entry point, LED feedback, reset logic | Rarely (custom behavior only) |

## Hardware

- **Board**: FM-DevKit (ESP32-based)
- **Framework**: Arduino via PlatformIO
- **Dependencies**: PubSubClient (MQTT), ArduinoJson

### Current Pin Configuration
- **GPIO 2**: Onboard blue LED (feedback)
- **GPIO 0**: BOOT button (long-press for reset)
- **GPIO 12**: RFID sensor (HIGH when tag present)
- **GPIO 27**: Magnet/reed switch (LOW when magnet present)

## Network Configuration

- **WiFi**: SFR_3474
- **MQTT Broker**: Raspberry Pi at `192.168.1.99:1883`
- **MQTT Software**: Mosquitto (enabled, auto-starts on Pi boot)

## MQTT Topics (Contract v1.0)

Base: `ey/<site>/<room>/prop/<propId>/`

| Suffix | Retained | Purpose |
|--------|----------|---------|
| `/status` | Yes | Current state (solved, online, override) |
| `/event` | No | Sensor triggers, one-shot actions |
| `/cmd` | No | Commands from room controller |

### Current Device Identity
- Site: `default`
- Room: `magie`
- Device: `magie_roueFortune`

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

### Build and Upload
```bash
cd "c:\Users\Manu\Documents\PlatformIO\Projects\EY_Prop_Base_PIO"
pio run --target upload
```

### Monitor Serial Output
```bash
pio device monitor
```

### Monitor MQTT on Raspberry Pi
```bash
# All messages from all props
mosquitto_sub -h localhost -t "ey/#" -v

# Just this prop
mosquitto_sub -h localhost -t "ey/default/magie/prop/magie_roueFortune/#" -v
```

### Check Mosquitto Status on Pi
```bash
sudo systemctl status mosquitto
```

## Setting Up a New Prop

1. Copy the entire project folder
2. Open `include/EY_Config.h`
3. Change `DEVICE_ID` to a unique name
4. Change `ROOM_ID` if different
5. Define `SENSORS[]` array with pin numbers and polarities
6. Set `SOLVE_MODE` to `ANY` or `ALL`
7. Flash to ESP32

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

### v1.1.0 — Audit Bug Fix Pass
- **MQTT field names**: Aligned with Contract v1.0 — `deviceId` → `propId`, `ts` → `timestamp`
- **LWT topic**: Moved from `/status` to dedicated `/lwt` topic for clean separation
- **DEVICE_NAME**: Added to config and included in status messages for auto-discovery
- **MQTT buffer size**: Increased to 512 bytes (default 256 was too small for status with sensor details)
- **Sensor debouncing**: Added configurable `DEBOUNCE_MS` (20ms default) to prevent false triggers from mechanical noise
- **Force-trigger locking**: GM-triggered sensors now stay locked (`forceLocked`) until reset, preventing physical state from overriding GM actions
- **LED mirror configurable**: LED mirror sensor is now configurable via `LED_MIRROR_SENSOR` index instead of hardcoded GPIO 27
- **Millis overflow safety**: Replaced `millis() < deadline` patterns with `millis() - start >= duration` for correct 49-day rollover handling
- **GPIO 12 strapping pin warning**: Added comment warning about ESP32 GPIO 12 boot behavior

- **Remote sensor triggering** (added): ESP32 now handles `set_output` command from GM Dashboard
  - MQTT command format: `{"type":"cmd","command":"set_output","sensorId":"rfid1"}`
  - Forces sensor to triggered state regardless of physical input
  - Publishes event with source "gm" and updates status

- **Sensor-level reporting** (added): MQTT status messages now include `details.sensors` array
  - Format: `"details": { "sensors": [{ "sensorId": "rfid1", "triggered": true }, ...] }`
  - Enables GM Dashboard to show individual sensor states
  - Enables GM to trigger individual sensors remotely

## Future Development

- **NTP time sync**: Replace `millis()` with real timestamps

## Related Projects

- **Room Controller**: `c:\02 - GM Manager\room-controller\` (Node.js, MQTT + WebSocket)
- **GM Dashboard**: `c:\02 - GM Manager\escapeRoomManager\` (React)

## Related Documentation

- `MQTT_CONTRACT_v1.md` - Full MQTT message specification (frozen v1.0)
- `c:\02 - GM Manager\WEBSOCKET_CONTRACT_v1.md` - Room Controller ↔ Dashboard protocol
- Architecture PDF on Desktop: `c:\Users\Manu\Desktop\EY Prop Architecture V2.pdf`
