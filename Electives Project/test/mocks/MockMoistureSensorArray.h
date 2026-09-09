#pragma once
#include "interfaces/IMoistureSensorArray.h"

namespace paddy_test {

class MockMoistureSensorArray : public paddy::IMoistureSensorArray {
 public:
  bool beginResult = true;
  paddy::MoistureReading nextReadings[paddy::kMoistureSensorCount];
  int readAllCallCount = 0;

  bool begin() override { return beginResult; }

  void readAll(paddy::MoistureReading (&out)[paddy::kMoistureSensorCount]) override {
    ++readAllCallCount;
    for (std::size_t i = 0; i < paddy::kMoistureSensorCount; ++i) {
      out[i] = nextReadings[i];
    }
  }
};

} // namespace paddy_test
