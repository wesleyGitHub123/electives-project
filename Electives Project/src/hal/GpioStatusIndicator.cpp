#include "hal/GpioStatusIndicator.h"

namespace paddy {

GpioStatusIndicator::GpioStatusIndicator(const StatusIndicatorPinConfig& pins) : pins_(pins) {}

bool GpioStatusIndicator::begin() {
  // TODO(hardware): configure pins_.ledPins as OUTPUT.
  return true;
}

void GpioStatusIndicator::showResult(BatchStatus /*status*/) {
  // TODO(hardware): light the LED(s) corresponding to status.
}

void GpioStatusIndicator::showBusy() {
  // TODO(hardware): indicate "sampling in progress".
}

void GpioStatusIndicator::showFault() {
  // TODO(hardware): indicate a fault condition.
}

void GpioStatusIndicator::clear() {
  // TODO(hardware): turn all indicator LEDs off.
}

} // namespace paddy
