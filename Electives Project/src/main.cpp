#include <Arduino.h>

#include "config/NetworkConfig.h"
#include "config/PinConfig.h"
#include "config/SystemConfig.h"
#include "core/BatchController.h"
#include "core/NotImplementedClassifier.h"
#include "hal/ArduinoClock.h"
#include "hal/Esp32MoistureSensorArray.h"
#include "hal/Ds18b20TemperatureSensor.h"
#include "hal/WifiUiDisplay.h"
#include "hal/GpioStartTrigger.h"
#include "hal/GpioStatusIndicator.h"
#include "hal/RelayDryingActuator.h"

// Composition root only: wire concrete HAL adapters to the one central
// BatchController and pump update() from loop(). No business logic lives
// here -- see include/core/BatchController.h for the state machine and
// docs/ARCHITECTURE.md for the overall design.
//
// Display: WifiUiDisplay (WiFi-hosted status page) is the go-forward status
// surface for now. The Oled128x64Display driver is parked, not deleted --
// confirmed flaky physical I2C connection (endTransmission() flip-flopping
// between 0/2 across resets on the same wiring, multiple boards), pending a
// replacement module. Re-test it later by swapping this one line:
//   WifiUiDisplay display(networkConfig);
// -> Oled128x64Display display(pinConfig.display);
// Nothing else changes; BatchController talks only to IDisplay.

using namespace paddy;

namespace {

// Hardware pin/channel mapping -- see include/config/PinConfig.h. Every
// value is currently a TBD placeholder until wiring is finalized.
PinConfig pinConfig{};

// WiFi status-page config -- development-only local AP created by the
// device itself (see include/config/NetworkConfig.h).
NetworkConfig networkConfig{};

ArduinoClock clock;
Esp32MoistureSensorArray moistureSensors(pinConfig.moisture);
Ds18b20TemperatureSensor temperatureSensor(pinConfig.temperature);
WifiUiDisplay display(networkConfig);
GpioStartTrigger startTrigger(pinConfig.startTrigger);
GpioStatusIndicator statusIndicator(pinConfig.statusIndicator);
RelayDryingActuator dryingActuator(pinConfig.dryingActuator);
NotImplementedClassifier classifier; // TBD: real embedded classifier plugs in here

BatchController controller(moistureSensors, temperatureSensor, display,
                           statusIndicator, dryingActuator, classifier,
                           clock, startTrigger,
                           config::DefaultControllerConfig());

SystemState lastLoggedState = SystemState::Fault; // forces a log on first loop()

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

void setup() {
  Serial.begin(config::kSerialBaudRate);
  delay(1000); // grace period for ESP32-S3 native USB CDC to enumerate (debug-only)

  controller.begin();

  Serial.println("[paddy] scaffold boot -- moisture/LED/relay adapters are stubs (TBD real hardware wiring)");
  Serial.print("[paddy] initial state: ");
  Serial.println(stateName(controller.state()));
}

void loop() {
  controller.update();
  // Note: the WiFi display adapter is serviced inside controller.update()
  // via IDisplay::poll() -- loop() does not (and must not) touch the
  // adapter directly.

  // Debug-only: trace state transitions over Serial so the architecture can
  // be exercised end-to-end on real hardware before any sensor/actuator
  // drivers are implemented. BatchController::update() itself is
  // non-blocking; the delay() below is purely to keep this log readable.
  const SystemState state = controller.state();
  if (state != lastLoggedState) {
    lastLoggedState = state;
    Serial.print("[paddy] state -> ");
    Serial.print(stateName(state));
    if (state == SystemState::Idle) {
      // Duplicate the WiFi page's switch-hint heuristic here so the Serial
      // trace alone can sanity-check the session gate without a browser.
      Serial.print(" | startSwitch=");
      Serial.print(startTrigger.isSessionRequested() ? "ON" : "OFF");
    }
    if (state == SystemState::ReportingResult || state == SystemState::DryingComplete) {
      Serial.print(" | result: ");
      Serial.print(statusName(controller.lastResult()));

      // Debug-only: surface the raw computed features so real sensor
      // wiring can be sanity-checked over Serial (moisture sensors are
      // still stubbed and will show valid=0; temperature reflects the
      // real DS18B20 once wired).
      const BatchFeatures& f = controller.lastFeatures();
      Serial.print(" | features valid=");
      Serial.print(f.valid ? 1 : 0);
      Serial.print(" meanMoisture=");
      Serial.print(f.meanMoisture);
      Serial.print(" moistureVariability=");
      Serial.print(f.moistureVariability);
      Serial.print(" temperatureC=");
      Serial.print(f.temperatureCelsius);
    }
    Serial.println();
  }

  delay(200); // debug-only pacing; remove once real per-sensor timing is tuned
}
