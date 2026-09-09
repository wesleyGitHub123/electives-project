#pragma once
#include "domain/SensorTypes.h"

namespace paddy {

// Pure, hardware-free computation of BatchFeatures from a raw BatchSample.
// No Arduino/ESP32 dependency -- fully unit-testable on a PC.
class FeatureExtractor {
 public:
  static BatchFeatures extract(const BatchSample& sample);
};

} // namespace paddy
