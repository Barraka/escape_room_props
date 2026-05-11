#include "EY_Config.h"  // Must be first — brings in PROP_CONFIG which may define HAS_PIR

#ifdef HAS_PIR

#include "EY_PIR.h"
#include "EY_Mqtt.h"

static uint8_t       s_pin = 0;
static bool          s_lastState = false;
static unsigned long s_lastTriggerMs = 0;

// HC-SR501 in repeat-trigger (H) mode holds OUT HIGH for the full delay
// window and re-extends on new motion. We only publish on the rising edge,
// but enforce a cooldown so we don't spam events if motion is intermittent.
static constexpr unsigned long PIR_RETRIGGER_COOLDOWN_MS = 1000;

void EY_PIR_Begin(uint8_t pin) {
  s_pin = pin;
  pinMode(s_pin, INPUT);
  s_lastState = false;
  s_lastTriggerMs = 0;
  Serial.print("[PIR] Ready on GPIO");
  Serial.println(pin);
}

void EY_PIR_Tick() {
  bool now = (digitalRead(s_pin) == HIGH);
  unsigned long t = millis();

  if (now && !s_lastState) {
    if (t - s_lastTriggerMs >= PIR_RETRIGGER_COOLDOWN_MS) {
      s_lastTriggerMs = t;
      Serial.println("[PIR] Motion detected");
      EY_PublishEvent("motion_detected", EY_MQTT::SRC_PLAYER);
    }
  }
  s_lastState = now;
}

#endif // HAS_PIR
