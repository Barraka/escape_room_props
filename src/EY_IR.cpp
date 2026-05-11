#include "EY_Config.h"  // Must be first — brings in PROP_CONFIG which may define HAS_IR

#ifdef HAS_IR

#include "EY_IR.h"
#include "EY_Mqtt.h"
#include <IRremoteESP8266.h>
#include <IRrecv.h>
#include <IRutils.h>

static IRrecv*       s_recv = nullptr;
static decode_results s_results;

// Debounce: NEC remotes send repeat frames (value 0xFFFFFFFFFFFFFFFF) every
// ~110ms when a button is held. Also normal presses can produce duplicate
// frames. Ignore identical codes within this window.
static constexpr unsigned long IR_DEBOUNCE_MS = 250;
static uint64_t s_lastCode = 0;
static unsigned long s_lastCodeTime = 0;

// Onboard LED diagnostic blink — fires on every accepted IR press so we can
// see at-the-prop whether decoding works, independent of MQTT publish.
static constexpr unsigned long IR_LED_BLINK_MS = 80;
static unsigned long s_ledOffAt = 0;
static bool s_ledOn = false;

static void irSetLed(bool on) {
  if (LED_ACTIVE_LOW) {
    digitalWrite(LED_PIN, on ? LOW : HIGH);
  } else {
    digitalWrite(LED_PIN, on ? HIGH : LOW);
  }
}

// Look up a captured NEC code in IR_KEY_MAP (defined in prop header).
// Returns mapped char ('0'..'9' or 'C'), or 0 if not found.
static char mapCodeToKey(uint64_t code) {
  uint32_t code32 = (uint32_t)code;
  for (uint8_t i = 0; i < IR_KEY_MAP_LEN; i++) {
    if (IR_KEY_MAP[i].code == code32) {
      return IR_KEY_MAP[i].key;
    }
  }
  return 0;
}

void EY_IR_Begin(uint8_t pin) {
  if (s_recv) {
    delete s_recv;
    s_recv = nullptr;
  }
  s_recv = new IRrecv(pin);
  s_recv->enableIRIn();

  s_lastCode = 0;
  s_lastCodeTime = 0;

  Serial.print("[IR] Ready on GPIO");
  Serial.println(pin);
}

void EY_IR_Tick() {
  // Turn the diagnostic blink off once its window expires
  if (s_ledOn && millis() >= s_ledOffAt) {
    irSetLed(false);
    s_ledOn = false;
  }

  if (!s_recv) return;
  if (!s_recv->decode(&s_results)) return;

  uint64_t code = s_results.value;
  decode_type_t proto = s_results.decode_type;
  s_recv->resume();

  // Ignore NEC repeat frames (button held) and non-NEC noise
  if (proto != NEC) return;
  if (code == 0xFFFFFFFFFFFFFFFFULL) return;

  // Debounce duplicate codes
  unsigned long now = millis();
  if (code == s_lastCode && (now - s_lastCodeTime) < IR_DEBOUNCE_MS) return;
  s_lastCode = code;
  s_lastCodeTime = now;

  // Diagnostic: pulse the onboard LED on every accepted press, regardless
  // of whether the code maps to a known key. Lets us see at-the-prop
  // whether the IR receiver is decoding at all, without relying on MQTT.
  irSetLed(true);
  s_ledOn = true;
  s_ledOffAt = now + IR_LED_BLINK_MS;

  char key = mapCodeToKey(code);
  if (key == 0) {
    Serial.print("[IR] Unknown code: 0x");
    Serial.println(uint64ToString(code, 16));
    return;
  }

  if (key == 'C') {
    Serial.println("[IR] Power -> clear");
    EY_PublishEvent("code_clear", EY_MQTT::SRC_PLAYER);
    return;
  }

  // Digit press: publish keypress event with the digit character
  char digit[2] = { key, '\0' };
  Serial.print("[IR] Digit ");
  Serial.println(digit);
  EY_PublishEventWithData("keypress", EY_MQTT::SRC_PLAYER, "digit", digit);
}

void EY_IR_Reset() {
  s_lastCode = 0;
  s_lastCodeTime = 0;
  Serial.println("[IR] Reset");
}

#endif // HAS_IR
