#pragma once
#include <WebServer.h>
#include "interfaces/IDisplay.h"
#include "config/NetworkConfig.h"

namespace paddy {

// WiFi-hosted status page, standing in for the parked Oled128x64Display
// (see docs/ARCHITECTURE.md "Project status: paused"). The ESP32-S3 hosts
// its own AP (default SSID "PaddyMonitor"); a phone/laptop joining it can
// browse to http://192.168.4.1 for the same minimal bring-up scope the
// OLED had: state, last result, temperature, switch hint, fault message.
//
// Design constraints (deliberate):
// - Observational only: handlers never reach back into BatchController or
//   any live internals. showState/showResult/showFault cache read-only
//   snapshots, and the request handler renders only those snapshots. The
//   slide switch remains the sole session-start control path.
// - Lenient on purpose (same policy as Oled128x64Display): a WiFi/AP
//   failure does not Fault the system -- Serial remains the feedback
//   channel.
// - Plain server-side HTML with <meta http-equiv="refresh" content="2">:
//   no JavaScript, no client build step, works in any phone browser.
class WifiUiDisplay : public IDisplay {
 public:
  explicit WifiUiDisplay(const NetworkConfig& config);

  bool begin() override;

  // Per-cycle servicing (called from BatchController::update() via
  // IDisplay::poll()): services pending HTTP requests.
  void poll() override;

  void showState(SystemState state) override;
  void showResult(BatchStatus status, const BatchFeatures& features) override;
  void showFault(const char* message) override;

 private:
  void handleRoot();

  NetworkConfig config_;
  WebServer server_;
  uint32_t requestCount_ = 0;

  // Read-only snapshots of the latest controller-reported state, cached by
  // the show*() callbacks on transitions (same pattern Oled128x64Display
  // used) and rendered on demand by the request handler.
  SystemState lastState_ = SystemState::Idle;
  BatchStatus lastStatus_ = BatchStatus::Unknown;
  float lastTemperatureC_ = 0.0f;
  bool lastTemperatureValid_ = false;
  char faultMessage_[64] = "";
};

} // namespace paddy