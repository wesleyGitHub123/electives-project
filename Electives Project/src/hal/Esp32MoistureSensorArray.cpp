#include "hal/Esp32MoistureSensorArray.h"

#include <Arduino.h>

namespace paddy {

Esp32MoistureSensorArray::Esp32MoistureSensorArray(
    const MoistureSensorPinConfig& pins, const MoistureCalibrationConfig& calibration)
    : pins_(pins), calibration_(calibration) {}

bool Esp32MoistureSensorArray::begin() {
  // Lenient on purpose (same policy as every other hal/ adapter): an
  // unassigned/miswired channel must not Fault the whole system --
  // readAll() reports per-channel validity instead.
  analogReadResolution(12);
  for (int pin : pins_.adcPins) {
    if (pin != kUnassignedPin) {
      analogSetPinAttenuation(pin, ADC_11db);
    }
  }
  return true;
}

void Esp32MoistureSensorArray::readAll(MoistureReading (&out)[kMoistureSensorCount]) {
  // Deliberately raw ADC counts, NOT calibration_-scaled. calibration_
  // stays at kUnassignedCalibrationValue on purpose: the bench dry/wet
  // endpoints are a soil-sensor reference frame, not yet validated
  // against real paddy grain (see docs/BENCH_VALIDATION.md). Applying
  // that math now would present a number that looks like a calibrated
  // moisture percentage before that validation exists.
  //
  // Known bench finding (BENCH_VALIDATION.md): wetter reads LOWER, and the
  // module's reading settles over ~20s after a state change (transient
  // noise 499 -> steady 59) -- a settle/dwell delay may be needed here
  // later; not added yet because single-point per-cycle sampling matches
  // the rest of the acquisition pipeline today.
  for (std::size_t i = 0; i < kMoistureSensorCount; ++i) {
    const int pin = pins_.adcPins[i];
    // Member assignment, not brace-init: the installed arduino-esp32 core
    // (2.0.17) compiles with -std=gnu++11, under which NSDMI makes
    // MoistureReading non-aggregate and 2-element brace-init ill-formed.
    if (pin == kUnassignedPin) {
      out[i].value = 0.0f;
      out[i].valid = false;
    } else {
      out[i].value = static_cast<float>(analogRead(pin));
      out[i].valid = true;
    }
  }
  (void)calibration_; // still unused -- see comment above.
}

} // namespace paddy
