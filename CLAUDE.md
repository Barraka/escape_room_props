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

## Future Development

- **Room Controller (MiniPC/Pi)**: Needs to be built
  - Subscribe to all prop MQTT messages
  - Manage game state and puzzle flow
  - WebSocket server for GM Dashboard
  - Cross-prop coordination
  - Persistence and analytics

## Related Documentation

- `MQTT_CONTRACT_v1.md` - Full MQTT message specification (frozen v1.0)
- Architecture PDF on Desktop: `c:\Users\Manu\Desktop\EY Prop Architecture V2.pdf`
