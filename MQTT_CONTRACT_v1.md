# MQTT Contract (v1.0) — FROZEN

Version: 1.0  
Status: FROZEN  
Breaking changes require a new contract file (v2+). Do not edit v1 retroactively.

## 1) Topic Schema

All topics are scoped by: `ey/<site>/<room>/...`

### 1.1 Prop → MQTT (published by ESP32)
- `ey/<site>/<room>/prop/<propId>/status`  (RETAINED)
- `ey/<site>/<room>/prop/<propId>/event`   (NOT retained)
- `ey/<site>/<room>/prop/<propId>/lwt`     (RETAINED)  *(may be implemented as `.../online` if preferred, but must be retained)*

### 1.2 MiniPC → Prop (commands)
- `ey/<site>/<room>/prop/<propId>/cmd`     (NOT retained)
- `ey/<site>/<room>/all/cmd`               (NOT retained, optional broadcast)

### 1.3 MiniPC → Room (optional)
- `ey/<site>/<room>/room/status`           (RETAINED, optional)
- `ey/<site>/<room>/room/event`            (NOT retained, optional)

## 2) Payload Schemas

### 2.1 Status (RETAINED)
Required fields:
- `type`: `"status"`
- `propId`: string
- `online`: boolean
- `solved`: boolean
- `timestamp`: number (unix ms) OR ISO string (choose one format per room and keep consistent)

Recommended fields:
- `lastChangeSource`: `"player"` | `"gm"` | `"system"`
- `override`: boolean
- `details`: object (prop-specific; MiniPC core must not rely on details for truth)

### 2.2 Event (NOT retained)
Required fields:
- `type`: `"event"`
- `propId`: string
- `action`: string
- `source`: `"player"` | `"gm"` | `"system"`
- `timestamp`: number (unix ms) OR ISO string

Optional fields:
- `value`: boolean | number | string
- `meta`: object

#### 2.2.1 Canonical GM Override Event (analytics rule)
A GM bypass MUST be expressed only as:
- `type = "event"`
- `action = "force_solved"`
- `source = "gm"`

No other message is counted as an override.

### 2.3 Command (NOT retained)
Required fields:
- `type`: `"cmd"`
- `propId`: string
- `command`: string
- `source`: `"gm"` | `"system"`
- `timestamp`: number (unix ms) OR ISO string
- `requestId`: string (unique)

Optional fields:
- `params`: object

Minimal frozen command set:
- `reset`
- `force_solved`
- `set_output`

## 3) Retain Rules

- `/status` MUST be retained
- `/event` MUST NOT be retained
- `/cmd` MUST NOT be retained
- `/lwt` (or `/online`) MUST be retained

## 4) LWT Rule (ESP32)

Each ESP32 MUST configure MQTT LWT such that an unexpected disconnect results in a retained publish that indicates:
- `online: false`

On successful (re)connect, the ESP32 MUST publish a retained status indicating:
- `online: true`

## 5) Authority Boundary

- ESP32 MAY compute *local physical completion* and set `solved=true` for its prop.
- MiniPC owns *session truth*, analytics, persistence, and any cross-prop logic.
- Dashboard is UI only; it must not infer truth.

