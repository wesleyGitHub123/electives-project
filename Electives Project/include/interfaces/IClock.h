#pragma once
#include <cstdint>

namespace paddy {

// Abstraction over elapsed time so core logic never calls millis() directly.
// This is what makes timing-dependent state transitions unit-testable on a
// PC (see test/mocks/MockClock.h).
class IClock {
 public:
  virtual ~IClock() = default;
  virtual uint32_t nowMillis() const = 0;
};

} // namespace paddy
