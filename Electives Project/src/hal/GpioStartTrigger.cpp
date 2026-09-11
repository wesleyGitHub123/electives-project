#include "hal/GpioStartTrigger.h"

#include <Arduino.h>

namespace paddy {

GpioStartTrigger::GpioStartTrigger(const StartTriggerPinConfig& pins)
    : pins_(pins) {}

bool GpioStartTrigger::begin() {
  pinMode(static_cast<uint8_t>(pins_.switchPin), INPUT_PULLUP);
  return true;
}

bool GpioStartTrigger::isSessionRequested() const {
  const bool pinLow = digitalRead(static_cast<uint8_t>(pins_.switchPin)) == LOW;
  // activeLow == true: pin LOW means "session requested". XOR handles both
  // polarities with the same code path.
  return pinLow == pins_.activeLow;
}

} // namespace paddy
