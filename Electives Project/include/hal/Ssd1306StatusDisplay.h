#pragma once
#include "interfaces/IDisplay.h"
#include "config/PinConfig.h"

namespace paddy {

// TODO(hardware): implement real SSD1306 rendering once the I2C wiring and
// display driver library dependency are confirmed (see
// config::DisplayPinConfig). Name assumes SSD1306 per the project brief's
// "OLED" mention -- confirm the exact panel/driver before wiring it up.
class Ssd1306StatusDisplay : public IDisplay {
 public:
  explicit Ssd1306StatusDisplay(const DisplayPinConfig& pins);

  bool begin() override;
  void showState(SystemState state) override;
  void showResult(BatchStatus status, const BatchFeatures& features) override;
  void showFault(const char* message) override;

 private:
  DisplayPinConfig pins_;
};

} // namespace paddy
