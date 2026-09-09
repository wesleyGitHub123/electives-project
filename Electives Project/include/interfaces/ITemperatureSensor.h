#pragma once

#include "domain/SensorTypes.h"

namespace paddy {

// Reads all temperature sensing points (currently a single batch probe; see
// kTemperatureSensorCount in domain/SensorTypes.h). Business logic never
// touches OneWire/DS18B20 APIs directly -- only this interface.
class ITemperatureSensor {
 public:
  virtual ~ITemperatureSensor() = default;

  virtual bool begin() = 0;

  // Acquires one reading from every temperature sensing point. Validity is
  // reported per-reading via TemperatureReading::valid.
  virtual void readAll(TemperatureReading (&out)[kTemperatureSensorCount]) = 0;
};

} // namespace paddy
