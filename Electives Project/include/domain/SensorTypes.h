#pragma once
#include <cstddef>

namespace paddy {

// Spatial moisture sensing points: 4 around the sack perimeter + 1 central
// probe, as described in the project brief.
constexpr std::size_t kMoistureSensorCount = 5;

// Temperature sensing points. Pinned to 1 (single batch probe) today; the
// whole temperature chain (BatchSample, ITemperatureSensor, HAL, mocks,
// FeatureExtractor) is parameterized on this constant, so adding probes
// later is a one-constant change -- same pattern as kMoistureSensorCount.
constexpr std::size_t kTemperatureSensorCount = 1;

// One raw reading from a single moisture sensing point. Unit/scale is TBD
// until sensor hardware and a calibration curve are finalized.
struct MoistureReading {
  float value = 0.0f;
  bool valid = false;
};

// One raw reading from a single temperature sensing point. Same
// value+valid convention as MoistureReading.
struct TemperatureReading {
  float value = 0.0f; // degrees Celsius
  bool valid = false;
};

// One full batch sample: all spatial moisture points + all temperature
// readings, captured together during a single AcquiringBatch cycle.
struct BatchSample {
  MoistureReading moisture[kMoistureSensorCount];
  TemperatureReading temperature[kTemperatureSensorCount];
};

// Derived features computed from a BatchSample. Pure data, no hardware
// dependency -- this is what gets handed to IClassifier.
struct BatchFeatures {
  float meanMoisture = 0.0f;
  float moistureVariability = 0.0f; // "how unevenly dried" the batch is
  float temperatureCelsius = 0.0f;
  bool valid = false; // false if the input sample had insufficient valid data
};

} // namespace paddy
