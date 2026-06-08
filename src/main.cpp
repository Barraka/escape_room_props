#include <Arduino.h>
#include <WiFi.h>
#include <ArduinoOTA.h>
#include "EY_Config.h"
#include "EY_Mqtt.h"
#include "EY_Sensors.h"
#include "EY_Outputs.h"

#ifdef HAS_SHAKER
#include "EY_Shaker.h"
#endif

#ifdef HAS_WIEGAND
#include "EY_Wiegand.h"
#endif

#ifdef HAS_CODE_SEQUENCE
#include "EY_CodeSequence.h"
#endif

#ifdef HAS_SIMON
#include "EY_Simon.h"
#endif

#ifdef HAS_VEHICLES
#include "EY_Vehicles.h"
#endif

#ifdef HAS_BOBINE
#include "EY_Bobine.h"
#endif

#ifdef HAS_IR
#include "EY_IR.h"
#endif

#ifdef HAS_RF433
#include "EY_RF433.h"
#endif

#ifdef HAS_PIR
#include "EY_PIR.h"
#endif

#ifdef HAS_SERVO
static void servoSetAngle(int angle) {
  // DS3225: 500µs (0°) to 2500µs (180°)
  // At 50Hz (20ms period), 16-bit resolution (65536 steps)
  int dutyMin = (500 * 65536) / 20000;   // ~1638
  int dutyMax = (2500 * 65536) / 20000;  // ~8192
  int duty = map(angle, 0, 180, dutyMin, dutyMax);
  ledcWrite(0, duty);  // channel 0
  Serial.print("[Servo] Angle: ");
  Serial.println(angle);
}
#endif

// =====================
// Prop State
// =====================
static bool solvedLatched = false;

// Track who last changed solved state and whether a GM override is active
static const char* lastChangeSource = EY_MQTT::SRC_DEVICE;
static bool overrideActive = false;

// Ignore sensors briefly after reset (to avoid instant re-solve)
static unsigned long ignoreSensorsStart = 0;
static bool ignoringSensors = false;

// LED patterns
static unsigned long lastBlink = 0;
static bool ledState = false;

// LED flash on sensor trigger (testing)
static unsigned long ledFlashStart = 0;
static bool ledFlashing = false;

// Reset feedback
static unsigned long resetFeedbackStart = 0;
static bool resetFeedbackActive = false;
static unsigned long lastResetBlink = 0;

// MQTT status announcement tracking
static bool statusAnnounced = false;

// OTA deferred init (needs WiFi connected first)
static bool otaStarted = false;

// BOOT button press tracking
static unsigned long resetBtnPressedAt = 0;
static bool resetBtnWasPressed = false;

#ifdef HAS_MANUAL_RESET
// Manual reset button (long-press)
static unsigned long manualResetPressedAt = 0;
static bool manualResetWasPressed = false;
#endif

// =====================
// Callbacks
// =====================

static void onSetSolved(bool value, const char* source) {
  // GM override: only allow forcing the puzzle forward (solved=true).
  // Reverting state mid-session is not supported; use reset instead.
  if (!value) return;

  lastChangeSource = (source && source[0]) ? source : EY_MQTT::SRC_DEVICE;

  if (strcmp(lastChangeSource, EY_MQTT::SRC_GM) == 0) {
    overrideActive = true;
  }

  solvedLatched = true;

  // Unlock outputs (maglocks) on force solve
  EY_Outputs_Release();

#ifdef HAS_SIMON
  EY_Simon_ForceSolve();
#endif

#ifdef HAS_VEHICLES
  EY_Vehicles_ForceSolve();
#endif

#ifdef HAS_CODE_SEQUENCE
  EY_CodeSequence_ForceSolve();
#endif

#ifdef HAS_SERVO
  servoSetAngle(SERVO_ANGLE_OPEN);
#endif

#ifdef HAS_BOBINE
  // Bobine has no sensors — force_solved only fires via GM cmd from
  // the dashboard. Treat it as "reveal everything now" (the bobine's
  // semantic solved state). The Room Controller drives the looping
  // light sequence via the separate `start_sequence` MQTT command.
  EY_Bobine_RevealAll();
#endif

#ifdef HAS_RF433
  EY_RF433_Send(RF_CODE_UP);
#endif

  EY_PublishEvent("force_solved", lastChangeSource);
  EY_PublishStatus(true, lastChangeSource, overrideActive);
}

#ifdef HAS_CODE_SEQUENCE
static void onCodeSubmitted(const char* code) {
  CodeSequenceResult result = EY_CodeSequence_Submit(code);
  const char* resultStr = (result == CodeSequenceResult::WRONG) ? "wrong" : "correct";
  EY_PublishEventWithData("code_result", EY_MQTT::SRC_PLAYER, "result", resultStr);
}
#endif

static void setLed(bool on) {
#ifdef HAS_SIMON
  // GPIO2 (LED_PIN) doubles as Simon LED #1 — the game owns that pin.
  // Suppress the onboard status LED so it can't fight the game's blink.
  (void)on;
  return;
#else
  if (LED_ACTIVE_LOW) {
    digitalWrite(LED_PIN, on ? LOW : HIGH);
  } else {
    digitalWrite(LED_PIN, on ? HIGH : LOW);
  }
#endif
}

static void handleReset() {
#ifdef HAS_SIMON
  // Simon auto-manages its state — ignore external resets to avoid reset storms
  // The game restarts on boot and after solve. GM can force-solve if needed.
  Serial.println("[Main] Reset ignored (Simon auto-manages)");
  return;
#endif

  solvedLatched = false;
  lastChangeSource = EY_MQTT::SRC_DEVICE;
  overrideActive = false;

  // Reset sensor states and re-arm outputs (re-lock maglocks)
  EY_Sensors_Reset();
  EY_Outputs_Reset();

#ifdef HAS_SHAKER
  EY_Shaker_Reset();
#endif

#ifdef HAS_WIEGAND
  EY_Wiegand_Reset();
#endif

#ifdef HAS_CODE_SEQUENCE
  EY_CodeSequence_Reset();
#endif

#ifdef HAS_SIMON
  EY_Simon_Reset();
#endif

#ifdef HAS_VEHICLES
  EY_Vehicles_Reset();
#endif

#ifdef HAS_SERVO
  servoSetAngle(SERVO_ANGLE_CLOSED);
#endif

#ifdef HAS_BOBINE
  EY_Bobine_Reset();
#endif

#ifdef HAS_IR
  EY_IR_Reset();
#endif

#ifdef HAS_RF433
  EY_RF433_Send(RF_CODE_DOWN);
#endif

  ignoringSensors = true;
  ignoreSensorsStart = millis();

  // Visual feedback
  resetFeedbackActive = true;
  resetFeedbackStart = millis();
  lastResetBlink = millis();
  ledState = false;
  setLed(false);

  statusAnnounced = false;

  EY_PublishStatus(false, lastChangeSource, overrideActive);
  Serial.println("[Main] Reset complete");
}

static void handleArm() {
  EY_Outputs_Arm();
#ifdef HAS_SIMON
  EY_Simon_Activate();
#endif
#ifdef HAS_VEHICLES
  EY_Vehicles_Activate();
#endif
  EY_PublishStatus(solvedLatched, lastChangeSource, overrideActive);
  Serial.println("[Main] Outputs armed");
}

// =====================
// Setup
// =====================

void setup() {
#ifdef HAS_RF433
  // Pin RF433 DATA low ASAP so FS1000A can't radiate from a floating input
  // during boot/init. A floating FS1000A jams the nearby IR receiver.
  pinMode(RF433_TX_PIN, OUTPUT);
  digitalWrite(RF433_TX_PIN, LOW);
#endif

  Serial.begin(115200);
  delay(800);
  Serial.println("==============================");
  Serial.println("EY_Prop_Base: setup()");
  Serial.print("Device: ");
  Serial.println(DEVICE_ID);
  Serial.print("Sensors: ");
  Serial.println(SENSOR_COUNT);
  Serial.print("Outputs: ");
  Serial.println(OUTPUT_COUNT);
  Serial.println("==============================");

  // Initialize hardware
  pinMode(LED_PIN, OUTPUT);
  pinMode(RESET_BTN_PIN, INPUT_PULLUP);
  setLed(false);

  // Initialize sensor system
  EY_Sensors_Begin();

  // Initialize output system (maglocks, relays — starts INACTIVE/unlocked)
  EY_Outputs_Begin();

#ifdef HAS_SHAKER
  // Initialize shaker module (uses receiver pin from first sensor)
  EY_Shaker_Begin(SENSORS[0].pin);
#endif

#ifdef HAS_WIEGAND
  EY_Wiegand_Begin(WIEGAND_D0_PIN, WIEGAND_D1_PIN);
#endif

#ifdef HAS_CODE_SEQUENCE
  EY_CodeSequence_Begin();
  EY_Wiegand_OnCode(onCodeSubmitted);
#endif

#ifdef HAS_SIMON
  EY_Simon_Begin();
#endif

#ifdef HAS_VEHICLES
  EY_Vehicles_Begin();
#endif

#ifdef HAS_BOBINE
  EY_Bobine_Begin();
#endif

#ifdef HAS_IR
  EY_IR_Begin(IR_RECV_PIN);
#endif

#ifdef HAS_RF433
  EY_RF433_Begin(RF433_TX_PIN);
#endif

#ifdef HAS_PIR
  EY_PIR_Begin(PIR_PIN);
#endif

#ifdef HAS_SERVO
  ledcSetup(0, 50, 16);          // channel 0, 50Hz, 16-bit resolution
  ledcAttachPin(SERVO_PIN, 0);   // attach servo pin to channel 0
  // Startup test: sweep open then back to closed
  servoSetAngle(SERVO_ANGLE_OPEN);
  delay(1000);
  servoSetAngle(SERVO_ANGLE_CLOSED);
  Serial.println("[Servo] Initialized (startup sweep done)");
#endif

  // Start networking (non-blocking)
  // - MQTT reset triggers handleReset()
  // - MQTT setSolved triggers onSetSolved(...)
  // - MQTT arm triggers handleArm()
  EY_Net_Begin(handleReset, onSetSolved, handleArm);

  // Initialize OTA (Over-The-Air updates)
  ArduinoOTA.setHostname(DEVICE_ID);
  ArduinoOTA.setPassword(OTA_PASSWORD);
  ArduinoOTA.setPort(OTA_PORT);

  ArduinoOTA.onStart([]() {
    Serial.println("[OTA] Update starting...");
  });
  ArduinoOTA.onEnd([]() {
    Serial.println("[OTA] Update complete, rebooting...");
  });
  ArduinoOTA.onError([](ota_error_t error) {
    Serial.print("[OTA] Error: ");
    if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
    else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
    else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
    else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
    else if (error == OTA_END_ERROR) Serial.println("End Failed");
  });

  // ArduinoOTA.begin() is deferred to loop() — requires WiFi to be connected

  // Announce initial state (will only publish if MQTT is already connected)
  EY_PublishStatus(false, lastChangeSource, overrideActive);

#if defined(BOBINE_TEST_MODE) && defined(HAS_BOBINE)
  // Wiring-bench test: auto-start the looping sequence at boot so the
  // bobine can be validated without needing the Room Controller online.
  // Enabled via the `hollywood_bobine-test` / `-test-ota` PlatformIO envs.
  Serial.println("[TEST MODE] Auto-starting bobine sequence");
  EY_Bobine_Start();
#endif
}

// =====================
// Main Loop
// =====================

void loop() {
#if defined(SIMON_TEST_MODE) && defined(HAS_SIMON)
  // DEV WIRING TEST: all LEDs solid ON; press a lit button → it blinks 3x → solid.
  // Networking + OTA stay alive so we can flash back out of test mode.
  EY_Net_Tick();
  if (!otaStarted && WiFi.status() == WL_CONNECTED) {
    ArduinoOTA.begin();
    otaStarted = true;
  }
  if (otaStarted) ArduinoOTA.handle();
  static bool simonTestInit = false;
  if (!simonTestInit) { EY_Simon_TestBegin(); simonTestInit = true; }
  EY_Simon_TestTick();
  return;
#endif

#if defined(VEHICLES_TEST_MODE) && defined(HAS_VEHICLES)
  // DEV WIRING TEST: prints each switch's position as you flip it — use to map
  // switches and discover the in-room combination. Networking + OTA stay alive.
  EY_Net_Tick();
  if (!otaStarted && WiFi.status() == WL_CONNECTED) {
    ArduinoOTA.begin();
    otaStarted = true;
  }
  if (otaStarted) ArduinoOTA.handle();
  static bool vehiclesTestInit = false;
  if (!vehiclesTestInit) { EY_Vehicles_TestBegin(); vehiclesTestInit = true; }
  EY_Vehicles_TestTick();
  return;
#endif

  // ---- LED mirrors sensor state (direct feedback - first priority) ----
  // This must be first to ensure instant response without network delays
  if (LED_MIRROR_SENSOR >= 0 && LED_MIRROR_SENSOR < SENSOR_COUNT) {
    const SensorDef& mirrorDef = SENSORS[LED_MIRROR_SENSOR];
    bool present = (mirrorDef.presentWhen == PresentWhen::LOW_LEVEL)
      ? (digitalRead(mirrorDef.pin) == LOW)
      : (digitalRead(mirrorDef.pin) == HIGH);
    setLed(present);
  }

  // Networking always ticks, but never blocks prop logic
  EY_Net_Tick();

#ifdef HAS_BOBINE
  // Ceiling bobine sequence — non-blocking; no-op when not running
  EY_Bobine_Tick();
#endif

#ifdef HAS_PIR
  // PIR motion sensor — independent of solve/sensor logic, just publishes events
  EY_PIR_Tick();
#endif

  // Start OTA once WiFi is connected (deferred from setup)
  if (!otaStarted && WiFi.status() == WL_CONNECTED) {
    ArduinoOTA.begin();
    otaStarted = true;
    Serial.println("[OTA] Ready");
  }

  // Handle OTA updates (must be called frequently)
  if (otaStarted) {
    ArduinoOTA.handle();
  }

  // If we (re)connected to MQTT, announce current status once
  if (EY_Mqtt_Connected() && !statusAnnounced) {
    EY_PublishStatus(solvedLatched, lastChangeSource, overrideActive);
    statusAnnounced = true;
  }

  // ---- RESET button (BOOT) long-press ----
  bool resetPressed = (digitalRead(RESET_BTN_PIN) == LOW);

  if (resetPressed && !resetBtnWasPressed) {
    resetBtnPressedAt = millis();
    resetBtnWasPressed = true;
  }
  if (!resetPressed) {
    resetBtnWasPressed = false;
  }
  if (resetBtnWasPressed && (millis() - resetBtnPressedAt >= RESET_HOLD_MS)) {
    Serial.println("[DEBUG] Reset triggered by BOOT button");
    handleReset();
    resetBtnWasPressed = false;
  }

#ifdef HAS_MANUAL_RESET
  // ---- Manual reset button (long-press) ----
  bool manualResetPressed = (digitalRead(MANUAL_RESET_PIN) == LOW);

  if (manualResetPressed && !manualResetWasPressed) {
    manualResetPressedAt = millis();
    manualResetWasPressed = true;
  }
  if (!manualResetPressed) {
    manualResetWasPressed = false;
  }
  if (manualResetWasPressed && (millis() - manualResetPressedAt >= MANUAL_RESET_HOLD_MS)) {
    Serial.println("[DEBUG] Reset triggered by manual reset button (3s hold)");
    handleReset();
    manualResetWasPressed = false;
  }
#endif

  // ---- Reset feedback (fast blink) has priority ----
  if (resetFeedbackActive) {
    if (millis() - resetFeedbackStart >= RESET_FEEDBACK_MS) {
      resetFeedbackActive = false;
    } else {
      if (millis() - lastResetBlink >= RESET_FEEDBACK_BLINK_MS) {
        ledState = !ledState;
        setLed(ledState);
        lastResetBlink = millis();
      }
      return;  // Skip sensor processing during reset feedback
    }
  }

  // ---- Sensor processing ----
  if (!ignoringSensors || millis() - ignoreSensorsStart >= IGNORE_SENSORS_MS) {
    ignoringSensors = false;

#ifdef HAS_WIEGAND
    // Wiegand: just tick the reader. Solve logic lives in code-sequence
    // module (if present); otherwise Pi handles puzzle state.
    EY_Wiegand_Tick();
  #ifdef HAS_CODE_SEQUENCE
    if (!solvedLatched && EY_CodeSequence_IsSolved()) {
      solvedLatched = true;
      lastChangeSource = EY_MQTT::SRC_PLAYER;
      EY_Outputs_Release();
      EY_PublishStatus(solvedLatched, lastChangeSource, overrideActive);
      Serial.println("[Main] SOLVED by player!");
    }
  #endif
#elif defined(HAS_IR)
    // IR: dumb reader — tick decoder, RC validates the entered code
    EY_IR_Tick();
#elif defined(HAS_SIMON)
    // Simon: custom game logic with LED blinking and button press detection
    bool simonSolved = EY_Simon_Tick();

    // Periodically publish status with simonProgress
    {
      static unsigned long lastSimonStatus = 0;
      if (millis() - lastSimonStatus >= SIMON_REPORT_INTERVAL_MS) {
        lastSimonStatus = millis();
        EY_PublishStatus(solvedLatched, lastChangeSource, overrideActive);
      }
    }

    // Onboard LED: blink proportional to progress, solid when solved
    if (!solvedLatched) {
      uint8_t progress = EY_Simon_GetProgress();
      if (progress == 0) {
        setLed(false);
      } else {
        unsigned long blinkRate = 1000 - (progress * 9);
        if (millis() - lastBlink >= blinkRate) {
          ledState = !ledState;
          setLed(ledState);
          lastBlink = millis();
        }
      }
    } else {
      setLed(true);
    }

    // Latch solved state
    if (!solvedLatched && simonSolved) {
      solvedLatched = true;
      lastChangeSource = EY_MQTT::SRC_PLAYER;

      EY_Outputs_Release();

      EY_PublishStatus(solvedLatched, lastChangeSource, overrideActive);
      Serial.println("[Main] SOLVED by player!");
    }
#elif defined(HAS_VEHICLES)
    // Vehicles: custom solve logic — all 6 switches must hold the target combination
    bool vehiclesSolved = EY_Vehicles_Tick();

    // Periodically publish status with vehiclesProgress
    {
      static unsigned long lastVehiclesStatus = 0;
      if (millis() - lastVehiclesStatus >= VEHICLE_REPORT_INTERVAL_MS) {
        lastVehiclesStatus = millis();
        EY_PublishStatus(solvedLatched, lastChangeSource, overrideActive);
      }
    }

    // Onboard LED: blink proportional to progress, solid when solved
    if (!solvedLatched) {
      uint8_t progress = EY_Vehicles_GetProgress();
      if (progress == 0) {
        setLed(false);
      } else {
        unsigned long blinkRate = 1000 - (progress * 9);
        if (millis() - lastBlink >= blinkRate) {
          ledState = !ledState;
          setLed(ledState);
          lastBlink = millis();
        }
      }
    } else {
      setLed(true);
    }

    // Latch solved state. Note: once the combination latches it stays solved
    // even if a player nudges a switch afterward (until GM reset / re-arm).
    if (!solvedLatched && vehiclesSolved) {
      solvedLatched = true;
      lastChangeSource = EY_MQTT::SRC_PLAYER;

      EY_Outputs_Release();

      EY_PublishStatus(solvedLatched, lastChangeSource, overrideActive);
      Serial.println("[Main] SOLVED by player!");
    }
#elif defined(HAS_SHAKER)
    // Shaker: custom solve logic replaces generic EY_Sensors_Tick()
    bool shakerSolved = EY_Shaker_Tick();

    // Periodically publish status with shakeProgress
    {
      static unsigned long lastShakerStatus = 0;
      if (millis() - lastShakerStatus >= SHAKE_REPORT_INTERVAL_MS) {
        lastShakerStatus = millis();
        EY_PublishStatus(solvedLatched, lastChangeSource, overrideActive);
      }
    }

    // LED: blink proportional to shake progress
    if (!solvedLatched) {
      uint8_t progress = EY_Shaker_GetProgress();
      if (progress == 0) {
        setLed(false);
      } else {
        // Blink faster as progress increases (1000ms at 1% → 100ms at 100%)
        unsigned long blinkRate = 1000 - (progress * 9);
        if (millis() - lastBlink >= blinkRate) {
          ledState = !ledState;
          setLed(ledState);
          lastBlink = millis();
        }
      }
    } else {
      // Solved: solid LED
      setLed(true);
    }

    // Latch solved state
    if (!solvedLatched && shakerSolved) {
      solvedLatched = true;
      lastChangeSource = EY_MQTT::SRC_PLAYER;

      // Unlock outputs (maglocks) on player solve
      EY_Outputs_Release();

      EY_PublishStatus(solvedLatched, lastChangeSource, overrideActive);
      Serial.println("[Main] SOLVED by player!");
    }
#else
    // Track sensor count before tick to detect new triggers
    static uint8_t prevPresentCount = 0;

    bool sensorsSolved = EY_Sensors_Tick();

    // Count currently present sensors
    uint8_t presentCount = 0;
    for (uint8_t i = 0; i < SENSOR_COUNT; i++) {
      const SensorState* st = EY_Sensors_GetState(i);
      if (st && st->present) presentCount++;
    }

    // Flash LED for 3s when a new sensor triggers
    if (presentCount > prevPresentCount) {
      ledFlashStart = millis();
      ledFlashing = true;
      Serial.print("[LED] Flash — ");
      Serial.print(presentCount);
      Serial.println(" sensor(s) present");
    }
    prevPresentCount = presentCount;

    // LED: flash if active, solid when solved, off otherwise
    if (ledFlashing && millis() - ledFlashStart < 3000) {
      if (millis() - lastBlink >= 150) {
        ledState = !ledState;
        setLed(ledState);
        lastBlink = millis();
      }
    } else {
      ledFlashing = false;
      setLed(solvedLatched);
    }

    // Latch solved state
    if (!solvedLatched && sensorsSolved) {
      solvedLatched = true;
      lastChangeSource = EY_MQTT::SRC_PLAYER;

      // Unlock outputs (maglocks) on player solve
      EY_Outputs_Release();

#ifdef HAS_SERVO
      servoSetAngle(SERVO_ANGLE_OPEN);
#endif

      EY_PublishStatus(solvedLatched, lastChangeSource, overrideActive);
      Serial.println("[Main] SOLVED by player!");
    }
#endif
  }
}
