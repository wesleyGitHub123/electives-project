#pragma once
#include "interfaces/ITemperatureSensor.h"
#include "config/PinConfig.h"
#include <OneWire.h>
#include <DallasTemperature.h>

namespace paddy {

// DS18B20 (1-Wire) temperature sensor via paulstoffregen/OneWire +
// milesburton/DallasTemperature (see platformio.ini lib_deps). Requires an
// external ~4.7k pull-up resistor between the data line and 3V3 -- see
// docs/ARCHITECTURE.md wiring notes; most bare DS18B20 breakout boards do
// not include one on-board.
class Ds18b20TemperatureSensor : public ITemperatureSensor {
 public:
  explicit Ds18b20TemperatureSensor(const TemperatureSensorPinConfig& pins);

  bool begin() override;
  void readAll(TemperatureReading (&out)[kTemperatureSensorCount]) override;

 private:
  TemperatureSensorPinConfig pins_;
  OneWire oneWire_;
  DallasTemperature sensors_;
};

} // namespace paddy
