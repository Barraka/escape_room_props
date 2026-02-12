#include "EY_Mqtt.h"
#include "EY_Config.h"
#include "EY_Sensors.h"
#include "EY_Outputs.h"

#include <WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>

static WiFiClient    s_wifi;
static PubSubClient  s_mqtt(s_wifi);
static ResetCallback s_onReset = nullptr;
static SetSolvedCallback s_onSetSolved = nullptr;
static ArmCallback s_onArm = nullptr;

// retry timers (non-blocking)
static unsigned long s_lastWifiAttempt = 0;
static unsigned long s_lastMqttAttempt = 0;

// Topic helpers following contract: ey/<site>/<room>/prop/<propId>/...
static String buildTopicBase() {
  return String("ey/") + SITE_ID + "/" + ROOM_ID + "/prop/" + DEVICE_ID;
}

static String buildStatusTopic() {
  return buildTopicBase() + "/status";
}

static String buildEventTopic() {
  return buildTopicBase() + "/event";
}

static String buildCmdTopic() {
  return buildTopicBase() + "/cmd";
}

static String buildLwtTopic() {
  return buildTopicBase() + "/lwt";
}

static String buildBroadcastCmdTopic() {
  return String("ey/") + SITE_ID + "/" + ROOM_ID + "/all/cmd";
}

static void mqttCallback(char* topic, byte* payload, unsigned int length) {
  (void)topic;

  // Copy payload to a null-terminated buffer
  char buf[256];
  unsigned int n = (length < sizeof(buf) - 1) ? length : (sizeof(buf) - 1);
  memcpy(buf, payload, n);
  buf[n] = '\0';

  // Accept either plain commands (legacy) or JSON commands (preferred)
  // Supported formats:
  //   Legacy:
  //     - "reset"
  //     - {"type":"reset"}
  //     - {"type":"setSolved","value":true/false,"source":"gm"}
  //   Contract v1 (preferred):
  //     - {"type":"cmd","command":"reset",...}
  //     - {"type":"cmd","command":"force_solved",...}

  bool isReset = false;
  bool isForceSolved = false;
  bool isArm = false;
  const char* triggerSensorId = nullptr;
  const char* cmdSource = EY_MQTT::SRC_GM;

  // Legacy shortcut: plain text "reset"
  if (strcmp(buf, "reset") == 0) {
    isReset = true;
  }

  // JSON parse (preferred)
  StaticJsonDocument<256> doc;
  DeserializationError err = deserializeJson(doc, buf);
  if (!err) {
    const char* type = doc[EY_MQTT::F_TYPE];
    const char* source = doc[EY_MQTT::F_SOURCE];
    if (source && source[0]) cmdSource = source;

    // Contract v1 format: type="cmd", command="reset"|"force_solved"
    if (type && strcmp(type, EY_MQTT::TYPE_CMD) == 0) {
      const char* command = doc["command"];
      if (command) {
        if (strcmp(command, "reset") == 0) {
          isReset = true;
        } else if (strcmp(command, "force_solved") == 0) {
          isForceSolved = true;
        } else if (strcmp(command, "arm") == 0) {
          isArm = true;
        } else if (strcmp(command, "set_output") == 0) {
          triggerSensorId = doc["sensorId"];
        }
      }
    }
    // Legacy format: type="reset"
    else if (type && strcmp(type, "reset") == 0) {
      isReset = true;
    }
    // Legacy format: type="setSolved"
    else if (type && strcmp(type, "setSolved") == 0) {
      bool value = doc["value"] | false;
      if (value) isForceSolved = true;
    }
  }

  // Execute actions
  if (isReset && s_onReset) {
    Serial.println("CMD: reset");
    s_onReset();
  }

  if (isForceSolved && s_onSetSolved) {
    Serial.print("CMD: force_solved from ");
    Serial.println(cmdSource);
    s_onSetSolved(true, cmdSource);
  }

  if (isArm && s_onArm) {
    Serial.println("CMD: arm");
    s_onArm();
  }

  if (triggerSensorId) {
    Serial.print("CMD: set_output sensorId=");
    Serial.print(triggerSensorId);
    Serial.print(" from ");
    Serial.println(cmdSource);
    EY_Sensors_ForceTrigger(triggerSensorId);
  }
}

static void wifiTick() {
  static bool printedWifi = false;
  if (WiFi.status() == WL_CONNECTED) {
    if (!printedWifi) {
      Serial.print("WiFi OK. IP=");
      Serial.println(WiFi.localIP());
      printedWifi = true;
    }
    return;
  }


  // If connecting, don't spam begin() (prevents wifi: "sta is connecting..." noise)
  if (WiFi.status() == WL_IDLE_STATUS) return;

  if (millis() - s_lastWifiAttempt < 5000) return;
  s_lastWifiAttempt = millis();

  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASS);
}

static void mqttTick() {
  if (WiFi.status() != WL_CONNECTED) return;
  if (s_mqtt.connected()) return;

  if (millis() - s_lastMqttAttempt < 3000) return;
  s_lastMqttAttempt = millis();

  String clientId = String("esp32_") + SITE_ID + "_" + ROOM_ID + "_" + DEVICE_ID;

  // Build LWT payload on /lwt topic (broker publishes on unexpected disconnect)
  String lwtTopic = buildLwtTopic();
  StaticJsonDocument<128> lwtDoc;
  lwtDoc[EY_MQTT::F_PROP_ID] = DEVICE_ID;
  lwtDoc[EY_MQTT::F_ONLINE] = false;

  char lwtPayload[128];
  serializeJson(lwtDoc, lwtPayload, sizeof(lwtPayload));

  // Connect with LWT (QoS 1, retained)
  if (s_mqtt.connect(clientId.c_str(), nullptr, nullptr, lwtTopic.c_str(), 1, true, lwtPayload)) {
    Serial.println("MQTT OK. Subscribed.");

    // Subscribe to command topics (contract-compliant)
    String tDev = buildCmdTopic();
    String tAll = buildBroadcastCmdTopic();

    s_mqtt.subscribe(tDev.c_str());
    s_mqtt.subscribe(tAll.c_str());

    // Publish online=true on /lwt topic (retained)
    StaticJsonDocument<128> onlineDoc;
    onlineDoc[EY_MQTT::F_PROP_ID] = DEVICE_ID;
    onlineDoc[EY_MQTT::F_ONLINE] = true;

    char onlinePayload[128];
    unsigned int len = serializeJson(onlineDoc, onlinePayload, sizeof(onlinePayload));
    s_mqtt.publish(lwtTopic.c_str(), (const uint8_t*)onlinePayload, len, true);  // retained

  } else {
    Serial.print("MQTT FAIL rc=");
    Serial.println(s_mqtt.state());
  }
}

void EY_Net_Begin(ResetCallback onReset, SetSolvedCallback onSetSolved, ArmCallback onArm) {
  s_onReset = onReset;
  s_onSetSolved = onSetSolved;
  s_onArm = onArm;

  s_mqtt.setServer(MQTT_HOST, MQTT_PORT);
  s_mqtt.setBufferSize(768);  // Default 256 is too small for status messages with sensor + output details
  s_mqtt.setCallback(mqttCallback);

  wifiTick();
  mqttTick();
}

void EY_Net_Tick() {
  wifiTick();
  mqttTick();
  s_mqtt.loop();
}

bool EY_Mqtt_Connected() {
  return s_mqtt.connected();
}

static void publishJson(const String& topic, const JsonDocument& doc) {
  if (!s_mqtt.connected()) return;

  char out[256];
  size_t len = serializeJson(doc, out, sizeof(out));
  bool ok = s_mqtt.publish(topic.c_str(), out, len);
  if (!ok) {
    Serial.print("MQTT publish failed on ");
    Serial.println(topic);
  }

}

void EY_PublishEventOk(const char* sensorName) {
  // Legacy helper kept for backward compatibility.
  // Map the old "sensor ok" semantics to the new action event contract.
  // Example: sensorName="magnet" -> action="magnet_present".
  if (!sensorName) return;

  char action[48];
  snprintf(action, sizeof(action), "%s_present", sensorName);
  EY_PublishEvent(action, EY_MQTT::SRC_PLAYER);
}

void EY_PublishSolved(bool solved) {
  // Legacy helper kept for backward compatibility.
  // Map to the new status contract using conservative defaults.
  EY_PublishStatus(solved, EY_MQTT::SRC_DEVICE, false);
}

// ============================================================
// New publishing functions (v1 contract compliant)
// ============================================================

void EY_PublishEvent(const char* action, const char* source) {
  if (!s_mqtt.connected()) return;
  if (!action) return;

  StaticJsonDocument<256> doc;
  doc[EY_MQTT::F_TYPE] = EY_MQTT::TYPE_EVENT;
  doc[EY_MQTT::F_PROP_ID] = DEVICE_ID;
  doc[EY_MQTT::F_ACTION] = action;
  doc[EY_MQTT::F_SOURCE] = source ? source : EY_MQTT::SRC_DEVICE;
  doc[EY_MQTT::F_TIMESTAMP] = millis();  // Note: real timestamp requires NTP

  String topic = buildEventTopic();
  publishJson(topic, doc);

  Serial.print("Event: ");
  Serial.print(action);
  Serial.print(" (");
  Serial.print(source ? source : "device");
  Serial.println(")");
}

void EY_PublishStatus(bool solved, const char* lastChangeSource, bool overrideActive) {
  if (!s_mqtt.connected()) return;

  // Larger buffer to accommodate sensor + output details
  StaticJsonDocument<768> doc;
  doc[EY_MQTT::F_TYPE] = EY_MQTT::TYPE_STATUS;
  doc[EY_MQTT::F_PROP_ID] = DEVICE_ID;
  doc["name"] = DEVICE_NAME;
  doc[EY_MQTT::F_ONLINE] = true;
  doc[EY_MQTT::F_SOLVED] = solved;
  doc[EY_MQTT::F_LAST_CHANGE_SOURCE] = lastChangeSource ? lastChangeSource : EY_MQTT::SRC_DEVICE;
  doc[EY_MQTT::F_OVERRIDE] = overrideActive;
  doc[EY_MQTT::F_TIMESTAMP] = millis();  // Note: real timestamp requires NTP

  // Add sensor-level details for GM Dashboard
  JsonObject details = doc.createNestedObject("details");
  JsonArray sensors = details.createNestedArray("sensors");

  uint8_t sensorCount = EY_Sensors_GetCount();
  for (uint8_t i = 0; i < sensorCount; i++) {
    const SensorState* state = EY_Sensors_GetState(i);
    JsonObject sensor = sensors.createNestedObject();
    sensor["sensorId"] = SENSORS[i].id;
    sensor["triggered"] = state ? state->present : false;
  }

  // Add output-level details
  uint8_t outputCount = EY_Outputs_GetCount();
  if (outputCount > 0) {
    JsonArray outputs = details.createNestedArray("outputs");
    for (uint8_t i = 0; i < outputCount; i++) {
      JsonObject output = outputs.createNestedObject();
      output["outputId"] = OUTPUTS[i].id;
      OutputPinState oState = EY_Outputs_GetState(i);
      output["state"] = (oState == OutputPinState::ARMED) ? "armed"
                       : (oState == OutputPinState::RELEASED) ? "released"
                       : "inactive";
    }
  }

  String topic = buildStatusTopic();

  // Status messages are RETAINED per contract
  char out[768];
  unsigned int len = serializeJson(doc, out, sizeof(out));
  bool ok = s_mqtt.publish(topic.c_str(), (const uint8_t*)out, len, true);  // retained = true

  if (!ok) {
    Serial.print("MQTT publish failed on ");
    Serial.println(topic);
  } else {
    Serial.print("Status: solved=");
    Serial.print(solved ? "true" : "false");
    Serial.print(", source=");
    Serial.print(lastChangeSource ? lastChangeSource : "device");
    Serial.print(", override=");
    Serial.print(overrideActive ? "true" : "false");
    Serial.print(", sensors=");
    Serial.println(sensorCount);
  }
}
