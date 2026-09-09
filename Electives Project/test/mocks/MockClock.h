#pragma once
#include "interfaces/IClock.h"

namespace paddy_test {

class MockClock : public paddy::IClock {
 public:
  uint32_t nowMillis() const override { return millis_; }
  void advance(uint32_t deltaMs) { millis_ += deltaMs; }
  void set(uint32_t ms) { millis_ = ms; }

 private:
  uint32_t millis_ = 0;
};

} // namespace paddy_test
