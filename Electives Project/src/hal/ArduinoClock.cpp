#include "hal/ArduinoClock.h"
#include <Arduino.h>

namespace paddy {

uint32_t ArduinoClock::nowMillis() const {
  return millis();
}

} // namespace paddy
