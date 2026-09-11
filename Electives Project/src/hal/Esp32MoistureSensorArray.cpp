#include "hal/Esp32MoistureSensorArray.h"

namespace paddy {

Esp32MoistureSensorArray::Esp32MoistureSensorArray(
    const MoistureSensorPinConfig& pins, const MoistureCalibrationConfig& calibration)
    : pins_(pins), calibration_(calibration) {}

bool Esp32MoistureSensorArray::begin() {
  // TODO(hardware): configure ADC pins/attenuation once pins_ is finalized.
  return true;
}

void Esp32MoistureSensorArray::readAll(MoistureReading (&out)[kMoistureSensorCount]) {
  // TODO(hardware): replace with real analogRead()-based acquisition +
  // calibration. Placeholder: report "no valid reading yet" for every point.
  // calibration_ is threaded through the constructor now (mechanically) so
  // writing the real driver body later is not a second constructor-signature
  // change -- but its values are kUnassignedCalibrationValue sentinels until
  // a sensor is chosen, wired, and bench-calibrated (docs/BENCH_VALIDATION.md).
  for (auto& reading : out) {
    reading.value = 0.0f;
    reading.valid = false;
  }
  (void)pins_;
  (void)calibration_;
}

} // namespace paddy
