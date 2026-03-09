#pragma once
#include <Arduino.h>

namespace Config {
  static constexpr const char* WIFI_SSID = "CHANGE_ME";
  static constexpr const char* WIFI_PASSWORD = "CHANGE_ME";

  // Optional. If empty, endpoints are open on the local network.
  static constexpr const char* API_SECRET = "CHANGE_ME_OPTIONAL";

  static constexpr const char* HOSTNAME = "sealion-whaleshark";
  static constexpr const char* TZ = "<+03>-3"; // replaced at runtime by POSIX TZ string for Sao Paulo
  static constexpr const char* POSIX_TZ_SAO_PAULO = "<-03>3";
  static constexpr const char* NTP1 = "pool.ntp.org";
  static constexpr const char* NTP2 = "time.google.com";

  static constexpr uint8_t FLUX_I2C_ADDR = 0x12;

  static constexpr unsigned long WIFI_RETRY_MS = 15'000;
  static constexpr unsigned long SENSOR_PERIOD_MS = 2'000;
  static constexpr unsigned long STATUS_PUSH_MS = 2'000;
  static constexpr unsigned long TIME_SYNC_MS = 15 * 60 * 1000UL;
  static constexpr unsigned long AUTOMATION_TICK_MS = 1'000;

  static constexpr bool ENABLE_WATER_TEMP = true;
  static constexpr bool ENABLE_PH = true;
  static constexpr bool ENABLE_TDS = true;
  static constexpr bool ENABLE_TURBIDITY = true;
  static constexpr bool ENABLE_WATER_LEVEL_LOW = true;
  static constexpr bool ENABLE_WATER_LEVEL_HIGH = true;

  // ADC conversion placeholders. These must be calibrated on the real hardware.
  static constexpr float ADC_REF_V = 3.3f;
  static constexpr float ADC_MAX = 4095.0f;

  static constexpr bool RELAYS_ACTIVE_HIGH = true;

  // Main aquarium pump philosophy:
  // keep it ON by default for circulation and filtration.
  static constexpr bool MAIN_PUMP_DEFAULT_ON = true;

  // If low water is detected, skip aquaponics and optionally stop the main pump to avoid dry running.
  static constexpr bool STOP_MAIN_PUMP_ON_LOW_LEVEL = true;

  // Aquaponics defaults for V1.
  // Tiny peristaltic pump, conservative schedule.
  static constexpr bool AQUAPONICS_ENABLED_DEFAULT = true;
  static constexpr uint16_t AQUAPONICS_RUN_SECONDS = 60;
  static constexpr uint16_t AQUAPONICS_INTERVAL_MINUTES = 120;
  static constexpr uint8_t AQUAPONICS_DAY_START_HOUR = 8;
  static constexpr uint8_t AQUAPONICS_DAY_END_HOUR = 20;

  static constexpr uint8_t MATRIX_BRIGHTNESS = 6;
}
