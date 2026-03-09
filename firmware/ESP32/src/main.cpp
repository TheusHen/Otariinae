#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <Wire.h>
#include <time.h>
#ifdef ESP32
#include <OneWire.h>
#include <DallasTemperature.h>
#endif

#include "pins.h"
#include "config.h"

namespace FluxProto {
  enum Command : uint8_t {
    CMD_SET_RELAYS      = 0x01,
    CMD_SET_MODE        = 0x02,
    CMD_SET_TEXT        = 0x03,
    CMD_SYNC_EPOCH      = 0x04,
    CMD_SET_BRIGHTNESS  = 0x05,
    CMD_SET_TIMER       = 0x06,
    CMD_PING            = 0x07,
    CMD_SET_ICON        = 0x08,
    CMD_GET_STATUS      = 0x10
  };

  enum MatrixMode : uint8_t {
    MATRIX_IDLE = 0,
    MATRIX_BOOT = 1,
    MATRIX_SCROLL_TEXT = 2,
    MATRIX_TIMER = 3,
    MATRIX_ICON = 4,
    MATRIX_BAR = 5
  };

  enum Icon : uint8_t {
    ICON_OK = 0,
    ICON_WIFI = 1,
    ICON_ALERT = 2,
    ICON_WATER = 3,
    ICON_PLANT = 4,
    ICON_LOADING = 5
  };

  struct StatusFrame {
    uint8_t relaysMask;
    uint8_t mode;
    uint8_t faults;
    uint8_t brightness;
    uint32_t uptimeSeconds;
  } __attribute__((packed));
}

enum class SystemMode : uint8_t {
  AUTO,
  MANUAL,
  MAINTENANCE
};

struct SensorState {
  float waterTempC = NAN;
  float ph = NAN;
  float tdsPpm = NAN;
  float turbidityPct = NAN;
  int ambientLightRaw = 0;
  bool lowLevel = false;
  bool highLevel = false;
  unsigned long lastReadMs = 0;
};

struct AutomationState {
  bool mainPump = Config::MAIN_PUMP_DEFAULT_ON;
  bool aquaponicsPump = false;
  bool spare1 = false;
  bool spare2 = false;
  bool aquaponicsEnabled = Config::AQUAPONICS_ENABLED_DEFAULT;
  time_t lastAquaponicsStart = 0;
  time_t lastTimeSync = 0;
};

struct RuntimeState {
  bool wifiConnected = false;
  bool ntpValid = false;
  bool fluxOnline = false;
  bool fluxEnabled = true;
  bool authEnabled = true;
  String lastMatrixText = "BOOT";
  SystemMode mode = SystemMode::AUTO;
};

WebServer server(80);
SensorState sensors;
AutomationState autoState;
RuntimeState runtime;

OneWire oneWire(Pins::WATER_TEMP_ONEWIRE);
DallasTemperature dallas(&oneWire);

static unsigned long lastSensorPoll = 0;
static unsigned long lastStatusPush = 0;
static unsigned long lastWifiAttempt = 0;
static unsigned long lastAutomationTick = 0;

bool hasApiSecret() {
  const char* secret = Config::API_SECRET;
  return secret && strlen(secret) > 0 && String(secret) != "CHANGE_ME_OPTIONAL";
}

bool checkAuth() {
  runtime.authEnabled = hasApiSecret();
  if (!runtime.authEnabled) return true;
  String header = server.header("X-API-Key");
  return header == String(Config::API_SECRET);
}

String modeToString(SystemMode mode) {
  switch (mode) {
    case SystemMode::AUTO: return "auto";
    case SystemMode::MANUAL: return "manual";
    case SystemMode::MAINTENANCE: return "maintenance";
  }
  return "unknown";
}

bool sendFluxPacket(const uint8_t* data, size_t len) {
  Wire.beginTransmission(Config::FLUX_I2C_ADDR);
  size_t written = Wire.write(data, len);
  uint8_t rc = Wire.endTransmission();
  runtime.fluxOnline = (rc == 0 && written == len);
  return runtime.fluxOnline;
}

bool fluxSetRelays(bool mainPump, bool aquaponics, bool spare1, bool spare2) {
  uint8_t mask = 0;
  if (mainPump) mask |= 0x01;
  if (aquaponics) mask |= 0x02;
  if (spare1) mask |= 0x04;
  if (spare2) mask |= 0x08;

  uint8_t packet[] = { FluxProto::CMD_SET_RELAYS, mask };
  return sendFluxPacket(packet, sizeof(packet));
}

bool fluxSetMode(uint8_t mode) {
  uint8_t packet[] = { FluxProto::CMD_SET_MODE, mode };
  return sendFluxPacket(packet, sizeof(packet));
}

bool fluxSetIcon(uint8_t icon) {
  uint8_t packet[] = { FluxProto::CMD_SET_ICON, icon };
  return sendFluxPacket(packet, sizeof(packet));
}

bool fluxSetBrightness(uint8_t value) {
  uint8_t packet[] = { FluxProto::CMD_SET_BRIGHTNESS, value };
  return sendFluxPacket(packet, sizeof(packet));
}

bool fluxSetTimer(uint32_t secondsRemaining) {
  uint8_t packet[1 + sizeof(uint32_t)] = { FluxProto::CMD_SET_TIMER };
  memcpy(packet + 1, &secondsRemaining, sizeof(uint32_t));
  return sendFluxPacket(packet, sizeof(packet));
}

bool fluxScrollText(const String& text) {
  String trimmed = text.substring(0, 32);
  uint8_t len = static_cast<uint8_t>(trimmed.length());
  uint8_t packet[2 + 32] = { FluxProto::CMD_SET_TEXT, len };
  memcpy(packet + 2, trimmed.c_str(), len);
  runtime.lastMatrixText = trimmed;
  return sendFluxPacket(packet, 2 + len);
}

bool fluxSyncEpoch(time_t epoch) {
  uint8_t packet[1 + sizeof(uint32_t)] = { FluxProto::CMD_SYNC_EPOCH };
  uint32_t e = static_cast<uint32_t>(epoch);
  memcpy(packet + 1, &e, sizeof(uint32_t));
  autoState.lastTimeSync = epoch;
  return sendFluxPacket(packet, sizeof(packet));
}

bool fluxGetStatus(FluxProto::StatusFrame& out) {
  Wire.beginTransmission(Config::FLUX_I2C_ADDR);
  Wire.write(static_cast<uint8_t>(FluxProto::CMD_GET_STATUS));
  if (Wire.endTransmission(false) != 0) {
    runtime.fluxOnline = false;
    return false;
  }
  const size_t need = sizeof(FluxProto::StatusFrame);
  size_t got = Wire.requestFrom((int)Config::FLUX_I2C_ADDR, (int)need);
  if (got != need) {
    runtime.fluxOnline = false;
    return false;
  }
  uint8_t* dst = reinterpret_cast<uint8_t*>(&out);
  for (size_t i = 0; i < need; ++i) dst[i] = Wire.read();
  runtime.fluxOnline = true;
  return true;
}

float readAdcVoltage(int pin) {
  uint16_t raw = analogRead(pin);
  return (static_cast<float>(raw) * Config::ADC_REF_V) / Config::ADC_MAX;
}

float estimatePh() {
  float v = readAdcVoltage(Pins::PH_ADC);
  return 7.0f + ((2.50f - v) * 3.0f);
}

float estimateTds() {
  float v = readAdcVoltage(Pins::TDS_ADC);
  float compensation = 1.0f;
  if (!isnan(sensors.waterTempC)) compensation = 1.0f + 0.02f * (sensors.waterTempC - 25.0f);
  float compensatedV = v / compensation;
  float tds = (133.42f * compensatedV * compensatedV * compensatedV
             - 255.86f * compensatedV * compensatedV
             + 857.39f * compensatedV) * 0.5f;
  return max(0.0f, tds);
}

float estimateTurbidityPct() {
  float v = readAdcVoltage(Pins::TURBIDITY_ADC);
  float pct = (1.0f - constrain(v / 3.0f, 0.0f, 1.0f)) * 100.0f;
  return constrain(pct, 0.0f, 100.0f);
}

void pollSensors() {
  if (Config::ENABLE_WATER_TEMP) {
    dallas.requestTemperatures();
    float t = dallas.getTempCByIndex(0);
    if (t > -50.0f && t < 120.0f) sensors.waterTempC = t;
  }
  if (Config::ENABLE_PH) sensors.ph = estimatePh();
  if (Config::ENABLE_TDS) sensors.tdsPpm = estimateTds();
  if (Config::ENABLE_TURBIDITY) sensors.turbidityPct = estimateTurbidityPct();
  sensors.ambientLightRaw = analogRead(Pins::AMBIENT_LIGHT_ADC);
  if (Config::ENABLE_WATER_LEVEL_LOW) sensors.lowLevel = digitalRead(Pins::WATER_LEVEL_LOW) == LOW;
  if (Config::ENABLE_WATER_LEVEL_HIGH) sensors.highLevel = digitalRead(Pins::WATER_LEVEL_HIGH) == LOW;
  sensors.lastReadMs = millis();
}

bool isWithinAquaponicsWindow(struct tm* tminfo) {
  if (!tminfo) return true;
  int h = tminfo->tm_hour;
  return h >= Config::AQUAPONICS_DAY_START_HOUR && h < Config::AQUAPONICS_DAY_END_HOUR;
}

void applyAutomation() {
  if (runtime.mode == SystemMode::MAINTENANCE) {
    autoState.mainPump = false;
    autoState.aquaponicsPump = false;
    fluxSetRelays(false, false, autoState.spare1, autoState.spare2);
    fluxSetIcon(FluxProto::ICON_ALERT);
    return;
  }

  if (runtime.mode == SystemMode::AUTO) {
    autoState.mainPump = Config::MAIN_PUMP_DEFAULT_ON;
    if (Config::STOP_MAIN_PUMP_ON_LOW_LEVEL && sensors.lowLevel) {
      autoState.mainPump = false;
    }

    bool canRunAquaponics = autoState.aquaponicsEnabled && !sensors.lowLevel;
    time_t now = time(nullptr);
    struct tm tminfo;
    struct tm* tmok = localtime_r(&now, &tminfo) ? &tminfo : nullptr;

    if (!canRunAquaponics || !isWithinAquaponicsWindow(tmok)) {
      autoState.aquaponicsPump = false;
    } else {
      time_t elapsed = now - autoState.lastAquaponicsStart;
      if (!autoState.aquaponicsPump && elapsed >= Config::AQUAPONICS_INTERVAL_MINUTES * 60UL) {
        autoState.aquaponicsPump = true;
        autoState.lastAquaponicsStart = now;
      }
      if (autoState.aquaponicsPump && elapsed >= Config::AQUAPONICS_RUN_SECONDS) {
        autoState.aquaponicsPump = false;
      }
    }
  }

  fluxSetRelays(autoState.mainPump, autoState.aquaponicsPump, autoState.spare1, autoState.spare2);

  if (!runtime.wifiConnected) {
    fluxSetIcon(FluxProto::ICON_WIFI);
  } else if (sensors.lowLevel) {
    fluxSetIcon(FluxProto::ICON_ALERT);
  } else if (autoState.aquaponicsPump) {
    fluxSetIcon(FluxProto::ICON_PLANT);
  } else {
    fluxSetIcon(FluxProto::ICON_OK);
  }
}

String jsonBool(bool value) { return value ? "true" : "false"; }

void handleStatus() {
  if (!checkAuth()) {
    server.send(401, "application/json", "{\"error\":\"unauthorized\"}");
    return;
  }

  FluxProto::StatusFrame flux{};
  bool gotFlux = fluxGetStatus(flux);
  String body = "{";
  body += "\"project\":\"SeaLion\",";
  body += "\"board\":\"WhaleShark\",";
  body += "\"mode\":\"" + modeToString(runtime.mode) + "\",";
  body += "\"wifi\":" + jsonBool(runtime.wifiConnected) + ",";
  body += "\"ntp\":" + jsonBool(runtime.ntpValid) + ",";
  body += "\"flux_online\":" + jsonBool(runtime.fluxOnline) + ",";
  body += "\"auth_enabled\":" + jsonBool(runtime.authEnabled) + ",";
  body += "\"main_pump\":" + jsonBool(autoState.mainPump) + ",";
  body += "\"aquaponics_pump\":" + jsonBool(autoState.aquaponicsPump) + ",";
  body += "\"spare1\":" + jsonBool(autoState.spare1) + ",";
  body += "\"spare2\":" + jsonBool(autoState.spare2) + ",";
  body += "\"matrix_text\":\"" + runtime.lastMatrixText + "\"";
  if (gotFlux) {
    body += ",\"flux_uptime_s\":" + String(flux.uptimeSeconds);
    body += ",\"flux_faults\":" + String(flux.faults);
    body += ",\"flux_brightness\":" + String(flux.brightness);
  }
  body += "}";
  server.send(200, "application/json", body);
}

void handleSensors() {
  if (!checkAuth()) {
    server.send(401, "application/json", "{\"error\":\"unauthorized\"}");
    return;
  }
  String body = "{";
  body += "\"water_temp_c\":" + String(sensors.waterTempC, 2) + ",";
  body += "\"ph\":" + String(sensors.ph, 2) + ",";
  body += "\"tds_ppm\":" + String(sensors.tdsPpm, 1) + ",";
  body += "\"turbidity_pct\":" + String(sensors.turbidityPct, 1) + ",";
  body += "\"ambient_light_raw\":" + String(sensors.ambientLightRaw) + ",";
  body += "\"low_level\":" + jsonBool(sensors.lowLevel) + ",";
  body += "\"high_level\":" + jsonBool(sensors.highLevel);
  body += "}";
  server.send(200, "application/json", body);
}

void handleMode() {
  if (!checkAuth()) {
    server.send(401, "application/json", "{\"error\":\"unauthorized\"}");
    return;
  }
  if (!server.hasArg("value")) {
    server.send(400, "application/json", "{\"error\":\"missing value\"}");
    return;
  }
  String value = server.arg("value");
  value.toLowerCase();
  if (value == "auto") runtime.mode = SystemMode::AUTO;
  else if (value == "manual") runtime.mode = SystemMode::MANUAL;
  else if (value == "maintenance") runtime.mode = SystemMode::MAINTENANCE;
  else {
    server.send(400, "application/json", "{\"error\":\"invalid mode\"}");
    return;
  }
  server.send(200, "application/json", "{\"ok\":true}");
}

void handleRelay() {
  if (!checkAuth()) {
    server.send(401, "application/json", "{\"error\":\"unauthorized\"}");
    return;
  }
  if (runtime.mode == SystemMode::AUTO) {
    server.send(409, "application/json", "{\"error\":\"switch to manual first\"}");
    return;
  }
  String name = server.arg("name");
  String state = server.arg("state");
  bool on = (state == "1" || state == "true" || state == "on");

  if (name == "main_pump") autoState.mainPump = on;
  else if (name == "aquaponics") autoState.aquaponicsPump = on;
  else if (name == "spare1") autoState.spare1 = on;
  else if (name == "spare2") autoState.spare2 = on;
  else {
    server.send(400, "application/json", "{\"error\":\"invalid relay name\"}");
    return;
  }
  fluxSetRelays(autoState.mainPump, autoState.aquaponicsPump, autoState.spare1, autoState.spare2);
  server.send(200, "application/json", "{\"ok\":true}");
}

void handleMatrixText() {
  if (!checkAuth()) {
    server.send(401, "application/json", "{\"error\":\"unauthorized\"}");
    return;
  }
  String text = server.arg("text");
  if (text.length() == 0) {
    server.send(400, "application/json", "{\"error\":\"missing text\"}");
    return;
  }
  fluxSetMode(FluxProto::MATRIX_SCROLL_TEXT);
  fluxScrollText(text);
  server.send(200, "application/json", "{\"ok\":true}");
}

void handleMatrixTimer() {
  if (!checkAuth()) {
    server.send(401, "application/json", "{\"error\":\"unauthorized\"}");
    return;
  }
  uint32_t seconds = server.arg("seconds").toInt();
  fluxSetMode(FluxProto::MATRIX_TIMER);
  fluxSetTimer(seconds);
  server.send(200, "application/json", "{\"ok\":true}");
}

void setupApi() {
  server.on("/api/v1/status", HTTP_GET, handleStatus);
  server.on("/api/v1/sensors", HTTP_GET, handleSensors);
  server.on("/api/v1/mode", HTTP_POST, handleMode);
  server.on("/api/v1/relay", HTTP_POST, handleRelay);
  server.on("/api/v1/matrix/text", HTTP_POST, handleMatrixText);
  server.on("/api/v1/matrix/timer", HTTP_POST, handleMatrixTimer);
  server.on("/api/v1/ping", HTTP_GET, []() { server.send(200, "application/json", "{\"ok\":true}"); });
  server.begin();
}

void connectWifi() {
  WiFi.mode(WIFI_STA);
  WiFi.setHostname(Config::HOSTNAME);
  WiFi.begin(Config::WIFI_SSID, Config::WIFI_PASSWORD);
}

void ensureWifi() {
  if (WiFi.status() == WL_CONNECTED) {
    runtime.wifiConnected = true;
    return;
  }
  runtime.wifiConnected = false;
  if (millis() - lastWifiAttempt >= Config::WIFI_RETRY_MS) {
    WiFi.disconnect();
    connectWifi();
    lastWifiAttempt = millis();
  }
}

void syncTimeIfPossible() {
  if (!runtime.wifiConnected) return;
  struct tm tminfo;
  if (!getLocalTime(&tminfo, 3000)) {
    runtime.ntpValid = false;
    return;
  }
  runtime.ntpValid = true;
  time_t now = time(nullptr);
  if ((now - autoState.lastTimeSync) >= (Config::TIME_SYNC_MS / 1000UL)) {
    fluxSyncEpoch(now);
  }
}

void setup() {
  Serial.begin(115200);
  pinMode(Pins::FLUX_INT, INPUT_PULLUP);
  pinMode(Pins::FLUX_ENABLE, OUTPUT);
  digitalWrite(Pins::FLUX_ENABLE, HIGH);

  pinMode(Pins::WATER_LEVEL_LOW, INPUT_PULLUP);
  pinMode(Pins::WATER_LEVEL_HIGH, INPUT_PULLUP);

  analogReadResolution(12);

  Wire.begin(Pins::I2C_SDA, Pins::I2C_SCL, 100000);
  dallas.begin();

  setenv("TZ", Config::POSIX_TZ_SAO_PAULO, 1);
  tzset();
  configTime(0, 0, Config::NTP1, Config::NTP2);

  connectWifi();
  setupApi();

  fluxSetBrightness(Config::MATRIX_BRIGHTNESS);
  fluxSetMode(FluxProto::MATRIX_BOOT);
  fluxScrollText("SEALION BOOT");
}

void loop() {
  server.handleClient();
  ensureWifi();
  syncTimeIfPossible();

  if (millis() - lastSensorPoll >= Config::SENSOR_PERIOD_MS) {
    lastSensorPoll = millis();
    pollSensors();
  }

  if (millis() - lastAutomationTick >= Config::AUTOMATION_TICK_MS) {
    lastAutomationTick = millis();
    applyAutomation();
  }

  if (runtime.mode == SystemMode::AUTO && autoState.aquaponicsPump) {
    time_t now = time(nullptr);
    uint32_t remaining = 0;
    if (now > 0 && now >= autoState.lastAquaponicsStart) {
      uint32_t elapsed = static_cast<uint32_t>(now - autoState.lastAquaponicsStart);
      remaining = (elapsed >= Config::AQUAPONICS_RUN_SECONDS) ? 0 : (Config::AQUAPONICS_RUN_SECONDS - elapsed);
    }
    fluxSetMode(FluxProto::MATRIX_TIMER);
    fluxSetTimer(remaining);
  }

  delay(5);
}
