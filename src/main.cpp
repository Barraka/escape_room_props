#include <Arduino.h>
#include "EY_Config.h"
#include "EY_Mqtt.h"
#include "EY_Sensors.h"

// =====================
// Prop State
// =====================
static bool solvedLatched = false;

// Track who last changed solved state and whether a GM override is active
static const char* lastChangeSource = EY_MQTT::SRC_DEVICE;
static bool overrideActive = false;

// Ignore sensors briefly after reset (to avoid instant re-solve)
static unsigned long ignoreSensorsUntil = 0;

// LED patterns
static unsigned long lastBlink = 0;
static bool ledState = false;

// Reset feedback
static unsigned long resetFeedbackUntil = 0;
static unsigned long lastResetBlink = 0;

// MQTT status announcement tracking
static bool statusAnnounced = false;

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

  // Reset sensor states
  EY_Sensors_Reset();

  ignoreSensorsUntil = millis() + IGNORE_SENSORS_MS;

  // Visual feedback
  resetFeedbackUntil = millis() + RESET_FEEDBACK_MS;
  lastResetBlink = millis();
  ledState = false;
  setLed(false);

  statusAnnounced = false;

  EY_PublishStatus(false, lastChangeSource, overrideActive);
  Serial.println("[Main] Reset complete");
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
  Serial.println("==============================");

  // Initialize hardware
  pinMode(LED_PIN, OUTPUT);
  pinMode(RESET_BTN_PIN, INPUT_PULLUP);
  setLed(false);

  // Initialize sensor system
  EY_Sensors_Begin();

  // Start networking (non-blocking)
  // - MQTT reset triggers handleReset()
  // - MQTT setSolved triggers onSetSolved(...)
  EY_Net_Begin(handleReset, onSetSolved);

  // Announce initial state (will only publish if MQTT is already connected)
  EY_PublishStatus(false, lastChangeSource, overrideActive);
}

// =====================
// Main Loop
// =====================

void loop() {
  // ---- LED mirrors magnet sensor state (direct feedback - first priority) ----
  // Read magnet sensor directly: LOW = magnet present
  // This must be first to ensure instant response without network delays
  bool magnetPresent = (digitalRead(27) == LOW);
  setLed(magnetPresent);

  // Networking always ticks, but never blocks prop logic
  EY_Net_Tick();

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
  if (millis() < resetFeedbackUntil) {
    if (millis() - lastResetBlink >= RESET_FEEDBACK_BLINK_MS) {
      ledState = !ledState;
      setLed(ledState);
      lastResetBlink = millis();
    }
    return;  // Skip sensor processing during reset feedback
  }

  // ---- Sensor processing ----
  if (millis() >= ignoreSensorsUntil) {
    bool sensorsSolved = EY_Sensors_Tick();

    // Latch solved state
    if (!solvedLatched && sensorsSolved) {
      solvedLatched = true;
      lastChangeSource = EY_MQTT::SRC_PLAYER;
      EY_PublishStatus(solvedLatched, lastChangeSource, overrideActive);
      Serial.println("[Main] SOLVED by player!");
    }
  }
}
