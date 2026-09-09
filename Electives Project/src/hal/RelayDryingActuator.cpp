#include "hal/RelayDryingActuator.h"

namespace paddy {

RelayDryingActuator::RelayDryingActuator(const DryingActuatorPinConfig& pins) : pins_(pins) {}

bool RelayDryingActuator::begin() {
  // TODO(hardware): configure pins_.relayPin as OUTPUT, set to the inactive
  // level for pins_.relayActiveHigh.
  return true;
}

void RelayDryingActuator::start() {
  // TODO(hardware): drive the relay GPIO to its active level.
  running_ = true;
}

void RelayDryingActuator::stop() {
  // TODO(hardware): drive the relay GPIO to its inactive level.
  running_ = false;
}

bool RelayDryingActuator::isRunning() const {
  return running_;
}

} // namespace paddy
