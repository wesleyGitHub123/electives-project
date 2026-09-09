#include "core/FeatureExtractor.h"
#include <cmath>

namespace paddy {

BatchFeatures FeatureExtractor::extract(const BatchSample& sample) {
  BatchFeatures features;

  float sum = 0.0f;
  int validCount = 0;
  for (const auto& reading : sample.moisture) {
    if (reading.valid) {
      sum += reading.value;
      ++validCount;
    }
  }

  // Temperature: mean of valid readings (identical to the single-probe
  // behavior when kTemperatureSensorCount == 1). When multiple probes exist,
  // this fold policy (mean) is the single place to revisit -- e.g. min/max
  // or per-probe features may be more useful for drying decisions.
  float tempSum = 0.0f;
  int tempValidCount = 0;
  bool allTempsValid = true;
  for (const auto& reading : sample.temperature) {
    if (reading.valid) {
      tempSum += reading.value;
      ++tempValidCount;
    } else {
      allTempsValid = false;
    }
  }

  // Conservative validity rule: every temperature probe must report a valid
  // reading for the features to be trusted. With a single probe this is
  // exactly the old temperatureValid gate.
  if (validCount == 0 || tempValidCount == 0 || !allTempsValid) {
    features.meanMoisture = 0.0f;
    features.moistureVariability = 0.0f;
    features.temperatureCelsius = 0.0f;
    features.valid = false;
    return features;
  }

  const float mean = sum / static_cast<float>(validCount);

  // Population standard deviation across valid moisture points, used as the
  // initial "how unevenly dried" variability metric. This is an algorithmic
  // choice, not a hardware one -- revisit if a different metric (e.g.
  // max-min spread) proves more useful once real batch data exists.
  float sumSquaredDiff = 0.0f;
  for (const auto& reading : sample.moisture) {
    if (reading.valid) {
      const float diff = reading.value - mean;
      sumSquaredDiff += diff * diff;
    }
  }
  const float variance = sumSquaredDiff / static_cast<float>(validCount);

  features.meanMoisture = mean;
  features.moistureVariability = std::sqrt(variance);
  features.temperatureCelsius = tempSum / static_cast<float>(tempValidCount);
  features.valid = true;

  return features;
}

} // namespace paddy
