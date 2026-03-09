#pragma once
#include <Arduino.h>

namespace Config {
  static constexpr uint8_t I2C_ADDRESS = 0x12;
  static constexpr bool RELAYS_ACTIVE_HIGH = true;
  static constexpr uint8_t DEFAULT_BRIGHTNESS = 6;

  static constexpr unsigned long LOG_PERIOD_MS = 60'000;
  static constexpr unsigned long HEARTBEAT_PERIOD_MS = 1000;
  static constexpr unsigned long FAILSAFE_TIMEOUT_MS = 20'000;

  static constexpr bool SAFE_MAIN_PUMP_ON_IF_LINK_LOST = true;
  static constexpr bool SAFE_AQUAPONICS_OFF_IF_LINK_LOST = true;
}
