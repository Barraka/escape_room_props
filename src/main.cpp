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

#ifdef HAS_SIMON
#include "EY_Simon.h"
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

#ifdef HAS_SERVO
  servoSetAngle(SERVO_ANGLE_OPEN);
#endif

#ifdef HAS_BOBINE
  EY_Bobine_Start();
#endif

#ifdef HAS_RF433
  EY_RF433_Send(RF_CODE_UP);
#endif

  EY_PublishEvent("force_solved", lastChangeSource);
  EY_PublishStatus(true, lastChangeSource, overrideActive);
}

static void setLed(bool on) {
  if (LED_ACTIVE_LOW) {
    digitalWrite(LED_PIN, on ? LOW : HIGH);
  } else {
    digitalWrite(LED_PIN, on ? HIGH : LOW);
  }
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

#ifdef HAS_SIMON
  EY_Simon_Reset();
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

#ifdef HAS_SIMON
  EY_Simon_Begin();
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
}

// =====================
// Main Loop
// =====================

void loop() {
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
    // Wiegand: just tick the reader — no solve logic (Pi handles puzzle state)
    EY_Wiegand_Tick();
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

#ifdef HAS_BOBINE
      EY_Bobine_Start();
#endif

      EY_PublishStatus(solvedLatched, lastChangeSource, overrideActive);
      Serial.println("[Main] SOLVED by player!");
    }
#endif
  }
}
