#pragma once
#include <cstdint>
#include "core/BatchController.h"

namespace paddy {
namespace config {

// Serial baud rate for the debug/bring-up console. Not hardware-specific
// (standard USB-CDC default), safe to fix now.
constexpr uint32_t kSerialBaudRate = 115200;

// Default BatchController timing configuration. Every value is a
// placeholder (TBD) pending real-world tuning once sensors are wired and
// the drying hardware is characterized. See BatchController::Config for
// what each field controls.
inline BatchController::Config DefaultControllerConfig() {
  BatchController::Config cfg;
  cfg.resultDisplayHoldMs = 0;     // TBD
  cfg.dryingRecheckIntervalMs = 0; // TBD
  return cfg;
}

} // namespace config
} // namespace paddy
