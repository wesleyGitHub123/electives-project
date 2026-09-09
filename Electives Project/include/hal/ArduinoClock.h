#pragma once
#include "interfaces/IClock.h"

namespace paddy {

// The one genuinely functional adapter in this scaffold: millis() carries
// no pin/calibration ambiguity, so it is safe to wire up now.
class ArduinoClock : public IClock {
 public:
  uint32_t nowMillis() const override;
};

} // namespace paddy
