#pragma once
#include <cstdint>

namespace paddy {

// WiFi status-page configuration. The device hosts its own access point
// (WiFi.softAP), so these are development-only local defaults invented for
// a network this device itself creates -- not real-world secrets and not a
// security model. The status page is observational only (read-only
// snapshots of controller state); it never starts sessions, mutates state,
// or controls actuators.
//
// Switching to station mode later (join an existing network instead) is a
// small change in WifiUiDisplay::begin() -- WiFi.begin(ssid, password) plus
// the real credentials of that network -- the interface does not change.
struct NetworkConfig {
  const char* apSsid = "PaddyMonitor";
  const char* apPassword = "paddy1234"; // WPA2 requires >= 8 characters
  uint16_t httpPort = 80;               // softAP gateway is typically 192.168.4.1
};

} // namespace paddy