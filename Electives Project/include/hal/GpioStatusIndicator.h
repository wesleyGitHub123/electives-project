#pragma once
#include "interfaces/IStatusIndicator.h"
#include "config/PinConfig.h"

namespace paddy {

// TODO(hardware): drive real LED GPIOs once StatusIndicatorPinConfig (how
// many LEDs exist and what each one means) is finalized.
class GpioStatusIndicator : public IStatusIndicator {
 public:
  explicit GpioStatusIndicator(const StatusIndicatorPinConfig& pins);

  bool begin() override;
  void showResult(BatchStatus status) override;
  void showBusy() override;
  void showFault() override;
  void clear() override;

 private:
  StatusIndicatorPinConfig pins_;
};

} // namespace paddy
