#include "hal/Ssd1306StatusDisplay.h"

namespace paddy {

Ssd1306StatusDisplay::Ssd1306StatusDisplay(const DisplayPinConfig& pins) : pins_(pins) {}

bool Ssd1306StatusDisplay::begin() {
  // TODO(hardware): initialize I2C + the OLED driver on pins_.
  return true;
}

void Ssd1306StatusDisplay::showState(SystemState /*state*/) {
  // TODO(hardware): render the current state on the OLED.
}

void Ssd1306StatusDisplay::showResult(BatchStatus /*status*/, const BatchFeatures& /*features*/) {
  // TODO(hardware): render the classification result + key features on the OLED.
}

void Ssd1306StatusDisplay::showFault(const char* /*message*/) {
  // TODO(hardware): render the fault message on the OLED.
}

} // namespace paddy
