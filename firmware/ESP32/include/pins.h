#pragma once

// WhaleShark (ESP32) recommended V1 mapping.
// Chosen to avoid typical ESP32 flash / boot-strap trouble.

namespace Pins {
  static constexpr int I2C_SDA = 21;
  static constexpr int I2C_SCL = 22;

  static constexpr int FLUX_INT = 27;      // Flux -> WhaleShark status / fault hint
  static constexpr int FLUX_ENABLE = 26;   // WhaleShark -> Flux enable / maintenance request

  // Optional local sensors on WhaleShark.
  static constexpr int WATER_TEMP_ONEWIRE = 4;
  static constexpr int WATER_LEVEL_LOW = 32;
  static constexpr int WATER_LEVEL_HIGH = 33;
  static constexpr int PH_ADC = 34;
  static constexpr int TDS_ADC = 35;
  static constexpr int TURBIDITY_ADC = 36;
  static constexpr int AMBIENT_LIGHT_ADC = 39;
}
