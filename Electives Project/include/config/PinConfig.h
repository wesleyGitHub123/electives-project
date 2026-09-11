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
  // ESP32-S3 Arduino-core default Wire pins (GPIO8=SDA, GPIO9=SCL); free,
  // non-strapping. I2C needs external pull-ups -- most OLED breakout boards
  // (including the FEIYANG 1.3" module) include them on-board.
  int i2cSdaPin = 8;
  int i2cSclPin = 9;
  // Near-universal default for these 128x64 I2C OLED modules. Not trusted
  // blindly: the display adapter scans the bus at boot and logs what it
  // actually finds (see Oled128x64Display::begin()).
  uint8_t i2cAddress = 0x3C;
};

struct StartTriggerPinConfig {
  // GPIO5: free per repo pin audit, not a strapping/USB/flash pin. Wired
  // switch-to-GND with INPUT_PULLUP, so no external resistor is needed.
  int switchPin = 5;
  // True = switch "on" position pulls the pin LOW (typical slide-switch to
  // GND wiring). Flip this one bool if the physical on/off feels reversed.
  bool activeLow = true;
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
  StartTriggerPinConfig startTrigger;
};

} // namespace paddy
