#pragma once
#include <cstdint>
#include "core/BatchController.h"

namespace paddy {
namespace config {

// Serial baud rate for the debug/bring-up console. Not hardware-specific
// (standard USB-CDC default), safe to fix now.
constexpr uint32_t kSerialBaudRate = 115200;

// Default BatchController timing configuration. See BatchController::Config
// for what each field controls.
inline BatchController::Config DefaultControllerConfig() {
  BatchController::Config cfg;
  // 3s so ReportingResult is actually observable (Serial trace, WiFi UI)
  // before the state machine moves on -- with this at 0, a free-running
  // switch cycles through every state roughly every ~200ms (main.cpp's
  // debug pacing), so a page load just catches an arbitrary instant of an
  // already-fast-moving cycle rather than a stable result. Still a
  // placeholder pending real-world tuning once sensors are wired for real.
  cfg.resultDisplayHoldMs = 3000;
  cfg.dryingRecheckIntervalMs = 0; // TBD -- drying not exercised yet (classifier is a stub)
  return cfg;
}

} // namespace config
} // namespace paddy
