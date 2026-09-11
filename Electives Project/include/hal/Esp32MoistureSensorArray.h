#pragma once
#include "interfaces/IMoistureSensorArray.h"
#include "config/PinConfig.h"

namespace paddy {

// Real per-channel ADC acquisition for the wired moisture sensing points.
// Unassigned channels (kUnassignedPin) report valid=false rather than
// faulting; readings are deliberately raw 12-bit ADC counts -- calibration
// is NOT applied yet. MoistureCalibrationConfig stays at
// kUnassignedCalibrationValue sentinels on purpose: the bench dry/wet
// endpoints are a soil-sensor reference frame, not yet validated against
// real paddy grain (docs/BENCH_VALIDATION.md) -- scaling now would present
// a number that looks like a calibrated moisture percentage before that
// validation exists.
//
// Swappability note: any future concrete moisture-sensor implementation
// (different capacitive model, muxed inputs, etc.) plugs in by writing a
// new class implementing IMoistureSensorArray -- BatchController and its
// tests never change. The calibration struct is constructor-injected so
// swapping sensor models later is a config edit, not a code edit.
class Esp32MoistureSensorArray : public IMoistureSensorArray {
 public:
  explicit Esp32MoistureSensorArray(const MoistureSensorPinConfig& pins,
                                    const MoistureCalibrationConfig& calibration);

  bool begin() override;
  void readAll(MoistureReading (&out)[kMoistureSensorCount]) override;

 private:
  MoistureSensorPinConfig pins_;
  // Held for the future calibration step (deferred -- see class comment).
  MoistureCalibrationConfig calibration_;
};

} // namespace paddy
