#ifdef HAS_WIEGAND

#include "EY_Wiegand.h"
#include "EY_Mqtt.h"
#include "EY_Config.h"

// Frame is complete after this many ms of silence on D0/D1.
// Wiegand bits arrive <1ms apart; 50ms is very conservative.
static constexpr unsigned long WIEGAND_TIMEOUT_MS = 50;

// Max digits in the code buffer (before # submit)
static constexpr uint8_t MAX_CODE_LEN = 10;

// ---- ISR state (volatile, modified in interrupts) ----
static volatile uint32_t s_bits = 0;
static volatile uint8_t  s_bitCount = 0;
static volatile unsigned long s_lastBitTime = 0;

// ---- Code buffer (accumulates digits until # is pressed) ----
static char    s_codeBuffer[MAX_CODE_LEN + 1];
static uint8_t s_codeLen = 0;

// ---- ISRs ----

static void IRAM_ATTR isrD0() {
  s_bits <<= 1;        // 0 bit
  s_bitCount++;
  s_lastBitTime = millis();
}

static void IRAM_ATTR isrD1() {
  s_bits <<= 1;
  s_bits |= 1;         // 1 bit
  s_bitCount++;
  s_lastBitTime = millis();
}

// ---- Key handling ----

// Wiegand 4-bit key values:
//   0-9 → digits
//   0x0A (10) → * (clear)
//   0x0B (11) → # (submit)

static void handleKey(uint8_t key) {
  if (key <= 9) {
    if (s_codeLen < MAX_CODE_LEN) {
      s_codeBuffer[s_codeLen++] = '0' + key;
      s_codeBuffer[s_codeLen] = '\0';

      Serial.print("[Wiegand] Digit ");
      Serial.print(key);
      Serial.print(" -> \"");
      Serial.print(s_codeBuffer);
      Serial.println("\"");

      EY_PublishEvent("keypress", EY_MQTT::SRC_PLAYER);
    }
  }
  else if (key == 0x0B) {
    // # = submit
    Serial.print("[Wiegand] # SUBMIT -> \"");
    Serial.print(s_codeBuffer);
    Serial.println("\"");

    if (s_codeLen > 0) {
      EY_PublishEventWithData("code_entered", EY_MQTT::SRC_PLAYER, "code", s_codeBuffer);
    }

    s_codeLen = 0;
    s_codeBuffer[0] = '\0';
  }
  else if (key == 0x0A) {
    // * = clear
    Serial.println("[Wiegand] * CLEAR");
    s_codeLen = 0;
    s_codeBuffer[0] = '\0';
    EY_PublishEvent("code_clear", EY_MQTT::SRC_PLAYER);
  }
  else {
    Serial.print("[Wiegand] Unknown key: 0x");
    Serial.println(key, HEX);
  }
}

// ---- Frame processing ----

static void processFrame(uint32_t bits, uint8_t bitCount) {
  Serial.print("[Wiegand] ");
  Serial.print(bitCount);
  Serial.print("-bit frame: 0x");
  Serial.println(bits, HEX);

  if (bitCount == 4) {
    // 4-bit keypress mode: each frame = one key
    handleKey(bits & 0x0F);
  }
  else if (bitCount == 8) {
    // 8-bit keypress mode: high nibble = key, low nibble = bitwise complement
    uint8_t high = (bits >> 4) & 0x0F;
    uint8_t low  = bits & 0x0F;
    if ((high ^ low) == 0x0F) {
      handleKey(high);
    } else {
      Serial.println("[Wiegand] Invalid 8-bit parity");
    }
  }
  else if (bitCount == 26) {
    // Wiegand 26: 1 parity + 8 facility + 16 card/PIN + 1 parity
    uint16_t cardNumber = (bits >> 1) & 0xFFFF;
    uint8_t  facility   = (bits >> 17) & 0xFF;

    Serial.print("[Wiegand] W26: facility=");
    Serial.print(facility);
    Serial.print(" card=");
    Serial.println(cardNumber);

    // Publish card number as code (keypad already buffered + confirmed with #)
    char code[6];
    snprintf(code, sizeof(code), "%u", cardNumber);
    EY_PublishEventWithData("code_entered", EY_MQTT::SRC_PLAYER, "code", code);
  }
  else {
    Serial.print("[Wiegand] Unexpected bit count: ");
    Serial.println(bitCount);
  }
}

// ---- Public API ----

void EY_Wiegand_Begin(uint8_t pinD0, uint8_t pinD1) {
  pinMode(pinD0, INPUT_PULLUP);
  pinMode(pinD1, INPUT_PULLUP);

  attachInterrupt(digitalPinToInterrupt(pinD0), isrD0, FALLING);
  attachInterrupt(digitalPinToInterrupt(pinD1), isrD1, FALLING);

  s_bits = 0;
  s_bitCount = 0;
  s_lastBitTime = 0;
  s_codeLen = 0;
  s_codeBuffer[0] = '\0';

  Serial.print("[Wiegand] Ready on D0=GPIO");
  Serial.print(pinD0);
  Serial.print(", D1=GPIO");
  Serial.println(pinD1);
}

void EY_Wiegand_Tick() {
  // Snapshot ISR state with interrupts disabled
  noInterrupts();
  uint8_t bc = s_bitCount;
  uint32_t bv = s_bits;
  unsigned long lastBit = s_lastBitTime;
  interrupts();

  if (bc > 0 && (millis() - lastBit >= WIEGAND_TIMEOUT_MS)) {
    // Frame complete — clear ISR state
    noInterrupts();
    s_bitCount = 0;
    s_bits = 0;
    interrupts();

    processFrame(bv, bc);
  }
}

void EY_Wiegand_Reset() {
  noInterrupts();
  s_bitCount = 0;
  s_bits = 0;
  interrupts();

  s_codeLen = 0;
  s_codeBuffer[0] = '\0';
  Serial.println("[Wiegand] Reset");
}

#endif // HAS_WIEGAND
