#pragma once
#include <cstdint>
#include "domain/SensorTypes.h"

namespace paddy {

// Central hardware pin/channel mapping. Every value here is a placeholder
// (TBD) until the physical wiring for the ESP32-S3 DevKitC-1 build is
// confirmed. No code outside include/hal/ and src/hal/ should ever
// reference a GPIO number directly -- everything hardware-specific is
// configured here, once known.

constexpr int kUnassignedPin = -1; // sentinel meaning "not wired yet"

struct MoistureSensorPinConfig {
  // 4 perimeter points + 1 central probe, in sensor-index order.
  // TBD: confirm physical placement <-> array index mapping, and whether
  // these are direct ADC pins or go through an analog mux.
  int adcPins[kMoistureSensorCount] = {
      kUnassignedPin, kUnassignedPin, kUnassignedPin,
      kUnassignedPin, kUnassignedPin,
  };
};

struct TemperatureSensorPinConfig {
  // DS18B20 (1-Wire) data line. GPIO4 chosen because it is broken out on
  // every ESP32-S3-DevKitC-1 variant, is not a strapping/USB/flash/PSRAM
  // pin, and is the common default for this sensor in tutorials/examples.
  // Requires an external ~4.7k pull-up resistor between DAT and 3V3 --
  // see docs/ARCHITECTURE.md wiring notes; most bare DS18B20 breakout
  // boards do not include one on-board.
  int oneWirePin = 4;
};

struct DisplayPinConfig {
  int i2cSdaPin = kUnassignedPin; // TBD
  int i2cSclPin = kUnassignedPin; // TBD
  uint8_t i2cAddress = 0x00;      // TBD: confirm OLED module's I2C address
};

struct StatusIndicatorPinConfig {
  // TBD: how many LEDs and what each one means has not been decided.
  int ledPins[3] = {kUnassignedPin, kUnassignedPin, kUnassignedPin};
};

struct DryingActuatorPinConfig {
  int relayPin = kUnassignedPin; // TBD
  bool relayActiveHigh = true;   // TBD: confirm relay module polarity
};

struct PinConfig {
  MoistureSensorPinConfig moisture;
  TemperatureSensorPinConfig temperature;
  DisplayPinConfig display;
  StatusIndicatorPinConfig statusIndicator;
  DryingActuatorPinConfig dryingActuator;
};

} // namespace paddy
