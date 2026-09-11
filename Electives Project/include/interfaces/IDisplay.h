#pragma once
#include "domain/SensorTypes.h"
#include "domain/BatchStatus.h"
#include "domain/SystemState.h"

namespace paddy {

// Shows system/result information to the user (e.g. an OLED). Business logic
// never calls a display driver library directly -- only this interface.
class IDisplay {
 public:
  virtual ~IDisplay() = default;

  virtual bool begin() = 0;
  virtual void showState(SystemState state) = 0;
  virtual void showResult(BatchStatus status, const BatchFeatures& features) = 0;
  virtual void showFault(const char* message) = 0;

  // Per-cycle adapter servicing. BatchController::update() calls this on
  // every iteration, before any state-machine work (so it runs even in
  // Fault). Exists specifically for display adapters that run their own
  // service loop (e.g. a WiFi status page needing server.handleClient()).
  // Not a generic "do background work" hook. Default no-op: adapters
  // without a service loop never need to override it.
  virtual void poll() {}
};

} // namespace paddy
