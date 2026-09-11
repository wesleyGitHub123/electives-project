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

// CSS class for the color-coded result badge: green=StoreSafely,
// amber=DryMore, red=HighRisk, gray=Unknown (incl. the never-pretend
// classifier default).
const char* badgeClass(BatchStatus status) {
  switch (status) {
    case BatchStatus::StoreSafely: return "b-green";
    case BatchStatus::DryMore: return "b-amber";
    case BatchStatus::HighRisk: return "b-red";
    case BatchStatus::Unknown: return "b-gray";
  }
  return "b-gray";
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
  // Moisture snapshots gated on the moisture-specific independent flag --
  // mirrors the temperature handling below exactly. Never gated on
  // features.valid, which also requires temperature.
  lastMoistureValid_ = features.moistureValid;
  lastMeanMoisture_ = features.meanMoisture;
  lastMoistureVariability_ = features.moistureVariability;
  // Independent of features.valid on purpose -- that flag also requires
  // moisture to be valid (currently always false, stub sensors), which
  // would hide a perfectly good temperature reading. See BatchFeatures.
  lastTemperatureValid_ = features.temperatureValid;
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

  // Temperature cell: live cached snapshot, gated by its own flag.
  char tempCell[24];
  if (lastTemperatureValid_) {
    snprintf(tempCell, sizeof(tempCell), "%.1f &deg;C",
             static_cast<double>(lastTemperatureC_));
  } else {
    snprintf(tempCell, sizeof(tempCell), "--");
  }

  // Moisture cells: live cached snapshots, gated by the moisture-specific
  // flag (NOT the whole-batch gate). Scale deliberately unit-less -- the
  // raw->% calibration does not exist yet (docs/BENCH_VALIDATION.md), so
  // pretending "%" would be inventing a scale.
  char meanCell[24];
  if (lastMoistureValid_) {
    snprintf(meanCell, sizeof(meanCell), "%.1f",
             static_cast<double>(lastMeanMoisture_));
  } else {
    snprintf(meanCell, sizeof(meanCell), "--");
  }
  char variabilityCell[24];
  if (lastMoistureValid_) {
    snprintf(variabilityCell, sizeof(variabilityCell), "%.1f",
             static_cast<double>(lastMoistureVariability_));
  } else {
    snprintf(variabilityCell, sizeof(variabilityCell), "--");
  }

  String html;
  html += "<!DOCTYPE html><html><head>";
  html += "<meta charset=\"utf-8\">";
  html += "<meta name=\"viewport\" content=\"width=device-width,initial-scale=1\">";
  html += "<meta http-equiv=\"refresh\" content=\"2\">";
  html += "<title>Paddy Monitor</title>";
  html += "<style>"
          "body{margin:0;min-height:100vh;display:flex;align-items:center;justify-content:center;"
          "background:radial-gradient(circle at 50% 28%,#143724 0%,#0c2317 55%,#06130c 100%);"
          "color:#e8f5ee;font-family:system-ui,'Segoe UI',Roboto,sans-serif}"
          ".card{background:rgba(16,36,26,.92);border:1px solid #1e4a34;border-radius:16px;"
          "padding:22px 26px;max-width:420px;width:90vw;box-sizing:border-box;"
          "box-shadow:0 10px 34px rgba(0,0,0,.5)}"
          "h1{font-size:1.1em;margin:0;text-align:center;letter-spacing:.05em;color:#cdeedd}"
          ".sub{text-align:center;font-size:.72em;color:#7fae97;margin:4px 0 14px}"
          ".sec{border-top:1px solid #1e4a34;padding:10px 0 6px}"
          ".sec h2{font-size:.68em;text-transform:uppercase;letter-spacing:.14em;"
          "color:#7fae97;margin:0 0 6px;font-weight:600}"
          ".row{display:flex;justify-content:space-between;align-items:center;"
          "padding:3px 0;font-size:.9em}"
          ".k{color:#9fc4b2}"
          ".badge{padding:3px 14px;border-radius:999px;font-size:.85em;font-weight:600}"
          ".b-green{background:#123d24;color:#a7f3c8}"
          ".b-amber{background:#3f2e05;color:#fbd98a}"
          ".b-red{background:#43161a;color:#f5b1b6}"
          ".b-gray{background:#263239;color:#aeb9bf}"
          ".pending{color:#5f7f6f;font-size:.8em;font-style:italic}"
          "</style>";
  html += "</head><body>";
  html += "<div class=\"card\">";
  html += "<h1>Paddy Monitor</h1>";
  html += "<div class=\"sub\">paddy batch moisture &amp; spoilage risk</div>";

  // --- Session section ---
  html += "<div class=\"sec\"><h2>Session</h2>";
  html += "<div class=\"row\"><span class=\"k\">State</span><span>";
  html += stateName(lastState_);
  html += "</span></div>";
  html += "<div class=\"row\"><span class=\"k\">Last result</span>"
          "<span class=\"badge ";
  html += badgeClass(lastStatus_);
  html += "\">";
  html += statusName(lastStatus_);
  html += "</span></div>";
  html += "<div class=\"row\"><span class=\"k\">Start switch</span><span>";
  html += switchHint;
  html += "</span></div>";
  if (faultMessage_[0] != '\0') {
    html += "<div class=\"row\"><span class=\"k\">Fault</span><span>";
    html += faultMessage_;
    html += "</span></div>";
  }
  html += "</div>";

  // --- Temperature (live: real DS18B20, gated by its own validity flag) ---
  html += "<div class=\"sec\"><h2>Temperature</h2>";
  html += "<div class=\"row\"><span class=\"k\">Batch probe</span><span>";
  html += tempCell;
  html += "</span></div></div>";

  // --- Moisture: mean/variability rows read live cached state (gated by
  // lastMoistureValid_); the per-point rows are static placeholders with
  // no backing field. Honesty boundary: IDisplay::showResult only ever
  // receives aggregate BatchFeatures, never the raw 5-element BatchSample,
  // so there is nothing real to render per point yet -- these rows say so
  // instead of faking values.
  html += "<div class=\"sec\"><h2>Moisture</h2>";
  html += "<div class=\"row\"><span class=\"k\">Mean</span><span>";
  html += meanCell;
  html += "</span></div>";
  html += "<div class=\"row\"><span class=\"k\">Variability</span><span>";
  html += variabilityCell;
  html += "</span></div>";
  html += "<div class=\"row pending\"><span class=\"k\">Point 1 (perimeter)</span>"
          "<span>not wired</span></div>";
  html += "<div class=\"row pending\"><span class=\"k\">Point 2 (perimeter)</span>"
          "<span>not wired</span></div>";
  html += "<div class=\"row pending\"><span class=\"k\">Point 3 (perimeter)</span>"
          "<span>not wired</span></div>";
  html += "<div class=\"row pending\"><span class=\"k\">Point 4 (perimeter)</span>"
          "<span>not wired</span></div>";
  html += "<div class=\"row pending\"><span class=\"k\">Point 5 (central)</span>"
          "<span>not wired</span></div>";
  html += "</div>";

  html += "</div></body></html>";

  server_.send(200, "text/html", html);
}

} // namespace paddy