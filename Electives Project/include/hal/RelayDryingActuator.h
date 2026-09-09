#pragma once
#include "interfaces/IDryingActuator.h"
#include "config/PinConfig.h"

namespace paddy {

// TODO(hardware): drive the real relay GPIO once relay pin/polarity
// (config::DryingActuatorPinConfig) is confirmed for this build.
class RelayDryingActuator : public IDryingActuator {
 public:
  explicit RelayDryingActuator(const DryingActuatorPinConfig& pins);

  bool begin() override;
  void start() override;
  void stop() override;
  bool isRunning() const override;

 private:
  DryingActuatorPinConfig pins_;
  bool running_ = false;
};

} // namespace paddy
