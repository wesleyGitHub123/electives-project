#pragma once

namespace paddy {

// Controls the relay-driven heater/blower drying hardware. Deliberately
// on/off only: the actual closed-loop drying algorithm (cycle timing, when
// it is safe to stop, etc.) is domain logic that lives in BatchController /
// future modules, not in this interface -- and is NOT implemented yet
// (TBD), per project scope.
class IDryingActuator {
 public:
  virtual ~IDryingActuator() = default;

  virtual bool begin() = 0;
  virtual void start() = 0;
  virtual void stop() = 0;
  virtual bool isRunning() const = 0;
};

} // namespace paddy
