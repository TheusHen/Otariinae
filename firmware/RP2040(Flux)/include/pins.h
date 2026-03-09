#pragma once

namespace Pins {
  static constexpr int I2C_SDA = 4;
  static constexpr int I2C_SCL = 5;

  static constexpr int MAX_DIN = 6;
  static constexpr int MAX_CLK = 7;
  static constexpr int MAX_CS  = 8;

  static constexpr int RELAY_MAIN_PUMP = 10;
  static constexpr int RELAY_AQUAPONICS = 11;
  static constexpr int RELAY_SPARE1 = 12;
  static constexpr int RELAY_SPARE2 = 13;

  static constexpr int STATUS_INT = 14;
  static constexpr int ENABLE_IN = 15;

  static constexpr int SD_MISO = 16;
  static constexpr int SD_CS = 17;
  static constexpr int SD_SCK = 18;
  static constexpr int SD_MOSI = 19;
  static constexpr int SD_DET = 20;
}
