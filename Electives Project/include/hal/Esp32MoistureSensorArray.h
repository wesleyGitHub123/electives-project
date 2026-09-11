#pragma once
#include "interfaces/IMoistureSensorArray.h"
#include "config/PinConfig.h"

namespace paddy {

// TODO(hardware): implement real ADC acquisition once sensor pin mapping,
// ADC channel assignment, and the moisture calibration curve are finalized
// (see include/config/PinConfig.h and docs/ARCHITECTURE.md). Deliberately a
// stub for now -- do not guess pins or a calibration formula.
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
  // Held for the future driver body; unused while this stays a stub.
  MoistureCalibrationConfig calibration_;
};

} // namespace paddy
