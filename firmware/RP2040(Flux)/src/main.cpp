#include <Arduino.h>
#include <SPI.h>
#include <Wire.h>
#include <SD.h>

#include "pins.h"
#include "config.h"

namespace Proto {
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

class Max7219Matrix {
public:
  void begin() {
    pinMode(Pins::MAX_CS, OUTPUT);
    pinMode(Pins::MAX_CLK, OUTPUT);
    pinMode(Pins::MAX_DIN, OUTPUT);
    SPI.setRX(Pins::SD_MISO);
    SPI.setTX(Pins::MAX_DIN);
    SPI.setSCK(Pins::MAX_CLK);
    SPI.begin(false);
    writeReg(0x0F, 0x00); // display test off
    writeReg(0x0C, 0x01); // shutdown off
    writeReg(0x0B, 0x07); // scan all digits
    writeReg(0x09, 0x00); // no decode
    setBrightness(Config::DEFAULT_BRIGHTNESS);
    clear();
    push();
  }

  void setBrightness(uint8_t value) {
    brightness = constrain(value, 0, 15);
    writeReg(0x0A, brightness);
  }

  uint8_t getBrightness() const { return brightness; }

  void clear() {
    for (uint8_t i = 0; i < 8; ++i) rows[i] = 0;
  }

  // Logical coordinates match the schematic view requested by the project:
  // x grows left->right, y grows top->bottom.
  // The schematic labels are COL7..COL0 from left to right,
  // so physical bit order is mirrored here intentionally.
  void setPixel(uint8_t x, uint8_t y, bool on) {
    if (x > 7 || y > 7) return;
    uint8_t physicalBit = 7 - x;
    if (on) rows[y] |= (1u << physicalBit);
    else rows[y] &= ~(1u << physicalBit);
  }

  void drawBitmap(const uint8_t bitmap[8]) {
    for (uint8_t y = 0; y < 8; ++y) rows[y] = bitmap[y];
  }

  void drawBar(uint8_t percent) {
    clear();
    uint8_t filled = map(percent, 0, 100, 0, 8);
    for (uint8_t y = 0; y < 8; ++y) {
      for (uint8_t x = 0; x < filled; ++x) setPixel(x, 7 - y, true);
    }
  }

  void push() {
    for (uint8_t i = 0; i < 8; ++i) writeReg(i + 1, rows[i]);
  }

private:
  uint8_t rows[8]{};
  uint8_t brightness = Config::DEFAULT_BRIGHTNESS;

  void writeReg(uint8_t reg, uint8_t value) {
    digitalWrite(Pins::MAX_CS, LOW);
    SPI.transfer(reg);
    SPI.transfer(value);
    digitalWrite(Pins::MAX_CS, HIGH);
  }
};

struct Runtime {
  volatile uint8_t requestedCommand = 0;
  volatile bool packetReady = false;
  volatile uint8_t rxBuffer[40]{};
  volatile size_t rxLen = 0;
  volatile bool statusRequested = false;

  bool sdReady = false;
  bool fluxEnabled = true;
  uint8_t brightness = Config::DEFAULT_BRIGHTNESS;
  uint8_t relayMask = 0;
  uint8_t faults = 0;
  uint8_t matrixMode = Proto::MATRIX_BOOT;

  String scrollText = "BOOT";
  uint32_t timerSeconds = 0;
  uint32_t epochBase = 0;
  uint32_t millisBase = 0;
  unsigned long lastLogMs = 0;
  unsigned long lastHeartbeatMs = 0;
  unsigned long lastMasterPacketMs = 0;
  unsigned long scrollStepMs = 0;
  uint16_t scrollOffset = 0;
  uint8_t bootFrame = 0;
};

Runtime runtime;
Max7219Matrix matrix;
File logFile;

const uint8_t ICON_OK[8] = {
  0b00000000,
  0b00000001,
  0b00000011,
  0b10110110,
  0b01111100,
  0b00111000,
  0b00010000,
  0b00000000,
};
const uint8_t ICON_WIFI[8] = {
  0b00011000,
  0b00100100,
  0b01000010,
  0b00011000,
  0b00100100,
  0b00011000,
  0b00000000,
  0b00011000,
};
const uint8_t ICON_ALERT[8] = {
  0b00011000,
  0b00111100,
  0b00111100,
  0b00111100,
  0b00111100,
  0b00011000,
  0b00000000,
  0b00011000,
};
const uint8_t ICON_WATER[8] = {
  0b00011000,
  0b00111100,
  0b01111110,
  0b01111110,
  0b00111100,
  0b00111100,
  0b00011000,
  0b00000000,
};
const uint8_t ICON_PLANT[8] = {
  0b00011000,
  0b00111100,
  0b00011000,
  0b01011010,
  0b10111101,
  0b00011000,
  0b00011000,
  0b00100100,
};

void setRelayPin(int pin, bool on) {
  digitalWrite(pin, Config::RELAYS_ACTIVE_HIGH ? on : !on);
}

void applyRelayMask(uint8_t mask) {
  runtime.relayMask = mask;
  setRelayPin(Pins::RELAY_MAIN_PUMP, mask & 0x01);
  setRelayPin(Pins::RELAY_AQUAPONICS, mask & 0x02);
  setRelayPin(Pins::RELAY_SPARE1, mask & 0x04);
  setRelayPin(Pins::RELAY_SPARE2, mask & 0x08);
}

uint32_t currentEpoch() {
  if (runtime.epochBase == 0) return 0;
  return runtime.epochBase + ((millis() - runtime.millisBase) / 1000UL);
}

void logLine(const String& line) {
  if (!runtime.sdReady) return;
  logFile = SD.open("/flux_log.csv", FILE_WRITE);
  if (!logFile) return;
  logFile.println(line);
  logFile.close();
}

void drawIcon(uint8_t icon) {
  switch (icon) {
    case Proto::ICON_OK: matrix.drawBitmap(ICON_OK); break;
    case Proto::ICON_WIFI: matrix.drawBitmap(ICON_WIFI); break;
    case Proto::ICON_ALERT: matrix.drawBitmap(ICON_ALERT); break;
    case Proto::ICON_WATER: matrix.drawBitmap(ICON_WATER); break;
    case Proto::ICON_PLANT: matrix.drawBitmap(ICON_PLANT); break;
    default: {
      matrix.clear();
      for (uint8_t i = 0; i <= runtime.bootFrame; ++i) matrix.setPixel(i, 3, true);
      break;
    }
  }
  matrix.push();
}

const uint8_t* glyphFor(char c) {
  static const uint8_t blank[5] = {0,0,0,0,0};
  static const uint8_t digits[10][5] = {
    {0x3E,0x51,0x49,0x45,0x3E}, {0x00,0x42,0x7F,0x40,0x00}, {0x62,0x51,0x49,0x49,0x46},
    {0x22,0x49,0x49,0x49,0x36}, {0x18,0x14,0x12,0x7F,0x10}, {0x2F,0x49,0x49,0x49,0x31},
    {0x3E,0x49,0x49,0x49,0x32}, {0x01,0x71,0x09,0x05,0x03}, {0x36,0x49,0x49,0x49,0x36},
    {0x26,0x49,0x49,0x49,0x3E}
  };
  static const uint8_t letters[][5] = {
    {0x7E,0x11,0x11,0x11,0x7E}, // A
    {0x7F,0x49,0x49,0x49,0x36}, // B
    {0x3E,0x41,0x41,0x41,0x22}, // C
    {0x7F,0x41,0x41,0x22,0x1C}, // D
    {0x7F,0x49,0x49,0x49,0x41}, // E
    {0x7F,0x09,0x09,0x09,0x01}, // F
    {0x3E,0x41,0x49,0x49,0x7A}, // G
    {0x7F,0x08,0x08,0x08,0x7F}, // H
    {0x00,0x41,0x7F,0x41,0x00}, // I
    {0x20,0x40,0x41,0x3F,0x01}, // J
    {0x7F,0x08,0x14,0x22,0x41}, // K
    {0x7F,0x40,0x40,0x40,0x40}, // L
    {0x7F,0x02,0x0C,0x02,0x7F}, // M
    {0x7F,0x04,0x08,0x10,0x7F}, // N
    {0x3E,0x41,0x41,0x41,0x3E}, // O
    {0x7F,0x09,0x09,0x09,0x06}, // P
    {0x3E,0x41,0x51,0x21,0x5E}, // Q
    {0x7F,0x09,0x19,0x29,0x46}, // R
    {0x26,0x49,0x49,0x49,0x32}, // S
    {0x01,0x01,0x7F,0x01,0x01}, // T
    {0x3F,0x40,0x40,0x40,0x3F}, // U
    {0x1F,0x20,0x40,0x20,0x1F}, // V
    {0x7F,0x20,0x18,0x20,0x7F}, // W
    {0x63,0x14,0x08,0x14,0x63}, // X
    {0x03,0x04,0x78,0x04,0x03}, // Y
    {0x61,0x51,0x49,0x45,0x43}  // Z
  };
  if (c >= '0' && c <= '9') return digits[c - '0'];
  if (c >= 'A' && c <= 'Z') return letters[c - 'A'];
  if (c >= 'a' && c <= 'z') return letters[c - 'a'];
  return blank;
}

void drawScrollTextFrame(const String& text, uint16_t pixelOffset) {
  matrix.clear();
  uint16_t cursor = 0;
  for (size_t idx = 0; idx < text.length(); ++idx) {
    const uint8_t* glyph = glyphFor(text[idx]);
    for (uint8_t col = 0; col < 5; ++col) {
      int16_t logicalX = cursor + col - pixelOffset;
      if (logicalX >= 0 && logicalX < 8) {
        uint8_t columnBits = glyph[col];
        for (uint8_t row = 0; row < 7; ++row) {
          bool on = columnBits & (1u << row);
          matrix.setPixel(logicalX, row, on);
        }
      }
    }
    cursor += 6;
  }
  matrix.push();
}

void drawTimer(uint32_t seconds) {
  // MM or SS compact display on 8x8.
  char buf[3];
  uint32_t value = seconds >= 60 ? (seconds + 59) / 60 : seconds;
  snprintf(buf, sizeof(buf), "%02lu", (unsigned long)(value % 100));
  matrix.clear();
  drawScrollTextFrame(String(buf), 0);
}

void updateMatrix() {
  switch (runtime.matrixMode) {
    case Proto::MATRIX_BOOT: {
      matrix.clear();
      for (uint8_t i = 0; i <= runtime.bootFrame; ++i) {
        matrix.setPixel(i, 2, true);
        matrix.setPixel(i, 5, true);
      }
      runtime.bootFrame = (runtime.bootFrame + 1) % 8;
      matrix.push();
      break;
    }
    case Proto::MATRIX_SCROLL_TEXT: {
      if (millis() - runtime.scrollStepMs > 90) {
        runtime.scrollStepMs = millis();
        runtime.scrollOffset++;
      }
      uint16_t totalWidth = runtime.scrollText.length() * 6 + 8;
      if (runtime.scrollOffset > totalWidth) runtime.scrollOffset = 0;
      drawScrollTextFrame(runtime.scrollText, runtime.scrollOffset);
      break;
    }
    case Proto::MATRIX_TIMER: {
      drawTimer(runtime.timerSeconds);
      matrix.push();
      break;
    }
    case Proto::MATRIX_ICON:
    case Proto::MATRIX_IDLE:
    case Proto::MATRIX_BAR:
    default:
      break;
  }
}

void onReceive(int count) {
  runtime.rxLen = 0;
  while (Wire.available() && runtime.rxLen < sizeof(runtime.rxBuffer)) {
    runtime.rxBuffer[runtime.rxLen++] = Wire.read();
  }
  runtime.packetReady = true;
  runtime.lastMasterPacketMs = millis();
}

void onRequest() {
  Proto::StatusFrame status{};
  status.relaysMask = runtime.relayMask;
  status.mode = runtime.matrixMode;
  status.faults = runtime.faults;
  status.brightness = runtime.brightness;
  status.uptimeSeconds = millis() / 1000UL;
  Wire.write(reinterpret_cast<const uint8_t*>(&status), sizeof(status));
}

void processPacket() {
  if (!runtime.packetReady || runtime.rxLen == 0) return;
  noInterrupts();
  uint8_t buffer[40]{};
  size_t len = runtime.rxLen;
  memcpy(buffer, (const void*)runtime.rxBuffer, len);
  runtime.packetReady = false;
  interrupts();

  uint8_t cmd = buffer[0];
  switch (cmd) {
    case Proto::CMD_SET_RELAYS:
      if (len >= 2) applyRelayMask(buffer[1]);
      break;
    case Proto::CMD_SET_MODE:
      if (len >= 2) runtime.matrixMode = buffer[1];
      break;
    case Proto::CMD_SET_TEXT:
      if (len >= 2) {
        uint8_t textLen = min<uint8_t>(buffer[1], len - 2);
        runtime.scrollText = "";
        for (uint8_t i = 0; i < textLen; ++i) runtime.scrollText += char(buffer[2 + i]);
        runtime.scrollOffset = 0;
        runtime.matrixMode = Proto::MATRIX_SCROLL_TEXT;
      }
      break;
    case Proto::CMD_SYNC_EPOCH:
      if (len >= 5) {
        uint32_t epoch;
        memcpy(&epoch, buffer + 1, sizeof(uint32_t));
        runtime.epochBase = epoch;
        runtime.millisBase = millis();
      }
      break;
    case Proto::CMD_SET_BRIGHTNESS:
      if (len >= 2) {
        runtime.brightness = buffer[1] & 0x0F;
        matrix.setBrightness(runtime.brightness);
      }
      break;
    case Proto::CMD_SET_TIMER:
      if (len >= 5) {
        memcpy(&runtime.timerSeconds, buffer + 1, sizeof(uint32_t));
        runtime.matrixMode = Proto::MATRIX_TIMER;
      }
      break;
    case Proto::CMD_SET_ICON:
      if (len >= 2) {
        runtime.matrixMode = Proto::MATRIX_ICON;
        drawIcon(buffer[1]);
      }
      break;
    case Proto::CMD_PING:
      break;
    default:
      runtime.faults |= 0x01;
      break;
  }
}

void checkFailsafe() {
  bool enabled = digitalRead(Pins::ENABLE_IN) == HIGH;
  runtime.fluxEnabled = enabled;

  bool stale = (millis() - runtime.lastMasterPacketMs) > Config::FAILSAFE_TIMEOUT_MS;
  if (!enabled || stale) {
    uint8_t safeMask = 0;
    if (Config::SAFE_MAIN_PUMP_ON_IF_LINK_LOST) safeMask |= 0x01;
    if (!Config::SAFE_AQUAPONICS_OFF_IF_LINK_LOST) safeMask |= 0x02;
    applyRelayMask(safeMask);
    if (stale) runtime.faults |= 0x02;
    drawIcon(Proto::ICON_ALERT);
  }
}

void logPeriodicState() {
  if (!runtime.sdReady) return;
  if (millis() - runtime.lastLogMs < Config::LOG_PERIOD_MS) return;
  runtime.lastLogMs = millis();
  String line = String(currentEpoch()) + "," +
                String(millis()) + "," +
                String(runtime.relayMask) + "," +
                String(runtime.matrixMode) + "," +
                String(runtime.faults) + "," +
                String(digitalRead(Pins::SD_DET) == LOW ? 1 : 0);
  logLine(line);
}

void heartbeat() {
  if (millis() - runtime.lastHeartbeatMs < Config::HEARTBEAT_PERIOD_MS) return;
  runtime.lastHeartbeatMs = millis();
  digitalWrite(Pins::STATUS_INT, !digitalRead(Pins::STATUS_INT));
}

void setup() {
  Serial.begin(115200);

  pinMode(Pins::RELAY_MAIN_PUMP, OUTPUT);
  pinMode(Pins::RELAY_AQUAPONICS, OUTPUT);
  pinMode(Pins::RELAY_SPARE1, OUTPUT);
  pinMode(Pins::RELAY_SPARE2, OUTPUT);
  pinMode(Pins::STATUS_INT, OUTPUT);
  pinMode(Pins::ENABLE_IN, INPUT_PULLUP);
  pinMode(Pins::SD_DET, INPUT_PULLUP);

  applyRelayMask(0x00);

  matrix.begin();
  matrix.setBrightness(Config::DEFAULT_BRIGHTNESS);

  SPI1.setRX(Pins::SD_MISO);
  SPI1.setTX(Pins::SD_MOSI);
  SPI1.setSCK(Pins::SD_SCK);
  SPI1.begin(false);
  runtime.sdReady = SD.begin(Pins::SD_CS, SPI1);
  if (runtime.sdReady) {
    logLine("epoch,millis,relay_mask,matrix_mode,faults,sd_present");
  }

  Wire.setSDA(Pins::I2C_SDA);
  Wire.setSCL(Pins::I2C_SCL);
  Wire.begin(Config::I2C_ADDRESS);
  Wire.onReceive(onReceive);
  Wire.onRequest(onRequest);

  runtime.lastMasterPacketMs = millis();
}

void loop() {
  processPacket();
  updateMatrix();
  heartbeat();
  checkFailsafe();
  logPeriodicState();

  if (runtime.matrixMode == Proto::MATRIX_TIMER && runtime.timerSeconds > 0) {
    static unsigned long lastTick = 0;
    if (millis() - lastTick >= 1000) {
      lastTick = millis();
      runtime.timerSeconds--;
    }
  }

  delay(10);
}
