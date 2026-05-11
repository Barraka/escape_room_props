#include "EY_Config.h"  // Must be first — brings in PROP_CONFIG which may define HAS_RF433

#ifdef HAS_RF433

#include "EY_RF433.h"
#include <RCSwitch.h>

static RCSwitch s_tx;
static bool     s_ready = false;

void EY_RF433_Begin(uint8_t pin) {
  s_tx.enableTransmit(pin);
  s_tx.setProtocol(RF433_PROTOCOL);
  s_tx.setPulseLength(RF433_PULSE_LENGTH);
  s_tx.setRepeatTransmit(RF433_REPEATS);
  s_ready = true;

  Serial.print("[RF433] Ready on GPIO");
  Serial.print(pin);
  Serial.print(" proto=");
  Serial.print(RF433_PROTOCOL);
  Serial.print(" pulse=");
  Serial.print(RF433_PULSE_LENGTH);
  Serial.print("us bits=");
  Serial.print(RF433_BITS);
  Serial.print(" repeats=");
  Serial.println(RF433_REPEATS);
}

void EY_RF433_Send(unsigned long code) {
  if (!s_ready) return;
  Serial.print("[RF433] Sending code ");
  Serial.println(code);
  s_tx.send(code, RF433_BITS);
}

#endif // HAS_RF433
