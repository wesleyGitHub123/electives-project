#include "hal/WifiUiDisplay.h"

#include <Arduino.h>
#include <WiFi.h>

namespace paddy {

namespace {

// Same minimal naming as Oled128x64Display::stateName/statusName -- kept as
// internal helpers here so neither adapter depends on the other. (This
// "temp + state + result only" naming is the bring-up scope; moisture and
// variability rendering is deferred until real sensor data exists.)
const char* stateName(SystemState state) {
  switch (state) {
    case SystemState::Idle: return "Idle";
    case SystemState::AcquiringBatch: return "AcquiringBatch";
    case SystemState::ExtractingFeatures: return "ExtractingFeatures";
    case SystemState::Classifying: return "Classifying";
    case SystemState::ReportingResult: return "ReportingResult";
    case SystemState::DryingActive: return "DryingActive";
    case SystemState::DryingReSampling: return "DryingReSampling";
    case SystemState::DryingComplete: return "DryingComplete";
    case SystemState::Fault: return "Fault";
  }
  return "Unknown";
}

const char* statusName(BatchStatus status) {
  switch (status) {
    case BatchStatus::Unknown: return "Unknown";
    case BatchStatus::StoreSafely: return "StoreSafely";
    case BatchStatus::DryMore: return "DryMore";
    case BatchStatus::HighRisk: return "HighRisk";
  }
  return "Unknown";
}

} // namespace

WifiUiDisplay::WifiUiDisplay(const NetworkConfig& config)
    : config_(config), server_(config.httpPort) {}

bool WifiUiDisplay::begin() {
  // Lenient on purpose (same policy as Oled128x64Display): a WiFi/AP
  // failure must not Fault the whole system -- the status page just stays
  // unreachable while everything else keeps working. Serial logging here
  // is the bring-up feedback channel.
  const bool apStarted = WiFi.softAP(config_.apSsid, config_.apPassword);
  Serial.print("[paddy][wifi-ui] softAP(\"");
  Serial.print(config_.apSsid);
  Serial.print("\") = ");
  Serial.println(apStarted ? "started" : "FAILED (status page unavailable)");

  Serial.print("[paddy][wifi-ui] browse to http://");
  Serial.println(WiFi.softAPIP());

  server_.on("/", [this]() { handleRoot(); });
  server_.begin();
  Serial.println("[paddy][wifi-ui] HTTP server started");
  return true;
}

void WifiUiDisplay::poll() {
  // Serviced every loop cycle via IDisplay::poll() -- not on transitions.
  server_.handleClient();
}

void WifiUiDisplay::showState(SystemState state) {
  // Cache-only snapshot; the request handler renders it. No hardware, no
  // controller reach-back.
  lastState_ = state;
}

void WifiUiDisplay::showResult(BatchStatus status,
                               const BatchFeatures& features) {
  lastStatus_ = status;
  lastTemperatureValid_ = features.valid;
  lastTemperatureC_ = features.temperatureCelsius;
}

void WifiUiDisplay::showFault(const char* message) {
  strncpy(faultMessage_, message != nullptr ? message : "", sizeof(faultMessage_) - 1);
  faultMessage_[sizeof(faultMessage_) - 1] = '\0';
}

void WifiUiDisplay::handleRoot() {
  ++requestCount_;
  // The proof-of-life signal: verifiable over Serial without ever joining
  // the AP from another device.
  Serial.print("[paddy][wifi-ui] served request #");
  Serial.println(requestCount_);

  // Same Idle -> "waiting" / else "ON" hint the parked OLED used; Idle is
  // the only state reachable while the start switch is off.
  const char* switchHint =
      (lastState_ == SystemState::Idle) ? "waiting" : "ON";

  char tempCell[24];
  if (lastTemperatureValid_) {
    snprintf(tempCell, sizeof(tempCell), "%.1f &deg;C",
             static_cast<double>(lastTemperatureC_));
  } else {
    snprintf(tempCell, sizeof(tempCell), "--");
  }

  String html;
  html += "<!DOCTYPE html><html><head>";
  html += "<meta charset=\"utf-8\">";
  html += "<meta name=\"viewport\" content=\"width=device-width,initial-scale=1\">";
  html += "<meta http-equiv=\"refresh\" content=\"2\">";
  html += "<title>Paddy Monitor</title>";
  html += "<style>body{font-family:monospace;margin:1.5em;background:#111;color:#eee;}"
          "h1{font-size:1.2em;}td{padding:4px 16px 4px 0;}td:first-child{color:#9aa;}</style>";
  html += "</head><body>";
  html += "<h1>Paddy Monitor</h1><table>";
  html += "<tr><td>State</td><td>";
  html += stateName(lastState_);
  html += "</td></tr><tr><td>Last result</td><td>";
  html += statusName(lastStatus_);
  html += "</td></tr><tr><td>Temperature</td><td>";
  html += tempCell;
  html += "</td></tr><tr><td>Start switch</td><td>";
  html += switchHint;
  html += "</td></tr>";
  if (faultMessage_[0] != '\0') {
    html += "<tr><td>Fault</td><td>";
    html += faultMessage_;
    html += "</td></tr>";
  }
  html += "</table></body></html>";

  server_.send(200, "text/html", html);
}

} // namespace paddy