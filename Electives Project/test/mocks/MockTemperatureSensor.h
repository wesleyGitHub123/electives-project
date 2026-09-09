#pragma once
#include "interfaces/ITemperatureSensor.h"

namespace paddy_test {

// Mock temperature sensor following the same array-read pattern as
// MockMoistureSensorArray.
class MockTemperatureSensor : public paddy::ITemperatureSensor {
 public:
  bool beginResult = true;
  paddy::TemperatureReading nextReadings[paddy::kTemperatureSensorCount]{};

  bool begin() override { return beginResult; }

  void readAll(paddy::TemperatureReading (&out)[paddy::kTemperatureSensorCount]) override {
    for (std::size_t i = 0; i < paddy::kTemperatureSensorCount; ++i) {
      out[i] = nextReadings[i];
    }
  }

  // Convenience for single-probe tests (kTemperatureSensorCount == 1).
  void setNext(float celsius, bool valid = true) {
    static_assert(paddy::kTemperatureSensorCount == 1,
                  "setNext() only makes sense with a single temperature probe");
    nextReadings[0] = paddy::TemperatureReading{celsius, valid};
  }
};

} // namespace paddy_test
