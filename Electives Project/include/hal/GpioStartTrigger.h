#pragma once
#include "interfaces/IStartTrigger.h"
#include "config/PinConfig.h"

namespace paddy {

// Slide switch wired between the GPIO pin and GND. Uses INPUT_PULLUP, so no
// external resistor is needed; polarity is configurable via
// StartTriggerPinConfig::activeLow in case the physical on/off direction
// turns out reversed.
class GpioStartTrigger : public IStartTrigger {
 public:
  explicit GpioStartTrigger(const StartTriggerPinConfig& pins);

  bool begin() override;
  bool isSessionRequested() const override;

 private:
  StartTriggerPinConfig pins_;
};

} // namespace paddy
