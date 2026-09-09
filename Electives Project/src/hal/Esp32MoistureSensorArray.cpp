#include "hal/Esp32MoistureSensorArray.h"

namespace paddy {

Esp32MoistureSensorArray::Esp32MoistureSensorArray(const MoistureSensorPinConfig& pins)
    : pins_(pins) {}

bool Esp32MoistureSensorArray::begin() {
  // TODO(hardware): configure ADC pins/attenuation once pins_ is finalized.
  return true;
}

void Esp32MoistureSensorArray::readAll(MoistureReading (&out)[kMoistureSensorCount]) {
  // TODO(hardware): replace with real analogRead()-based acquisition +
  // calibration. Placeholder: report "no valid reading yet" for every point.
  for (auto& reading : out) {
    reading.value = 0.0f;
    reading.valid = false;
  }
  (void)pins_;
}

} // namespace paddy
