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
  solvedLatched = false;
  lastChangeSource = EY_MQTT::SRC_DEVICE;
  overrideActive = false;

  // Reset sensor states and re-arm outputs (re-lock maglocks)
  EY_Sensors_Reset();
  EY_Outputs_Reset();

#ifdef HAS_SHAKER
  EY_Shaker_Reset();
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
  EY_PublishStatus(solvedLatched, lastChangeSource, overrideActive);
  Serial.println("[Main] Outputs armed");
}

// =====================
// Setup
// =====================

void setup() {
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
    handleReset();
    resetBtnWasPressed = false;
  }

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

#ifdef HAS_SHAKER
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
    bool sensorsSolved = EY_Sensors_Tick();

    // Latch solved state
    if (!solvedLatched && sensorsSolved) {
      solvedLatched = true;
      lastChangeSource = EY_MQTT::SRC_PLAYER;

      // Unlock outputs (maglocks) on player solve
      EY_Outputs_Release();

      EY_PublishStatus(solvedLatched, lastChangeSource, overrideActive);
      Serial.println("[Main] SOLVED by player!");
    }
#endif
  }
}
