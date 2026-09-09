#pragma once
#include "interfaces/IMoistureSensorArray.h"
#include "config/PinConfig.h"

namespace paddy {

// TODO(hardware): implement real ADC acquisition once sensor pin mapping,
// ADC channel assignment, and the moisture calibration curve are finalized
// (see include/config/PinConfig.h and docs/ARCHITECTURE.md). Deliberately a
// stub for now -- do not guess pins or a calibration formula.
class Esp32MoistureSensorArray : public IMoistureSensorArray {
 public:
  explicit Esp32MoistureSensorArray(const MoistureSensorPinConfig& pins);

  bool begin() override;
  void readAll(MoistureReading (&out)[kMoistureSensorCount]) override;

 private:
  MoistureSensorPinConfig pins_;
};

} // namespace paddy
