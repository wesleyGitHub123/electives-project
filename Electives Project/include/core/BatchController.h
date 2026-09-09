#pragma once
#include <cstdint>
#include "domain/SensorTypes.h"
#include "domain/BatchStatus.h"
#include "domain/SystemState.h"
#include "interfaces/IMoistureSensorArray.h"
#include "interfaces/ITemperatureSensor.h"
#include "interfaces/IDisplay.h"
#include "interfaces/IStatusIndicator.h"
#include "interfaces/IDryingActuator.h"
#include "interfaces/IClassifier.h"
#include "interfaces/IClock.h"

namespace paddy {

// The single central state machine tying sensing -> feature extraction ->
// classification -> reporting -> (optional) closed-loop drying together.
//
// This class talks ONLY to the interfaces above. It never includes
// Arduino.h and never calls an ESP32/Arduino API directly, which is what
// makes it constructible and fully drivable from native unit tests using
// mocks (see test/mocks/ and test/test_native/test_batch_controller.cpp).
//
// See docs/ARCHITECTURE.md for the full state transition diagram.
class BatchController {
 public:
  struct Config {
    // How long ReportingResult is held before the controller moves on.
    // TBD: needs real-world tuning; 0 = advance on the next update() call.
    uint32_t resultDisplayHoldMs = 0;

    // How often to re-sample/re-classify while DryingActive.
    // TBD: needs real-world tuning; 0 = re-check on the next update() call.
    uint32_t dryingRecheckIntervalMs = 0;
  };

  // Note: `config` has no default argument here deliberately -- some older
  // toolchains (e.g. GCC 6.x, used by PlatformIO's `native` test platform on
  // this machine) mis-parse a nested-struct default argument declared
  // inside its own enclosing class. Callers that want defaults should pass
  // `BatchController::Config{}` explicitly (see config::DefaultControllerConfig()).
  BatchController(IMoistureSensorArray& moistureSensors,
                   ITemperatureSensor& temperatureSensor,
                   IDisplay& display,
                   IStatusIndicator& statusIndicator,
                   IDryingActuator& dryingActuator,
                   IClassifier& classifier,
                   IClock& clock,
                   Config config);

  // Initializes all hardware adapters. Enters Fault if any adapter fails.
  void begin();

  // Advances the state machine by exactly one step. Must be called
  // repeatedly (e.g. every loop() iteration) and never blocks.
  void update();

  SystemState state() const { return state_; }
  BatchStatus lastResult() const { return lastResult_; }
  const BatchFeatures& lastFeatures() const { return currentFeatures_; }

 private:
  void enterState(SystemState next);
  bool elapsedSinceStateEntry(uint32_t durationMs) const;

  void handleIdle();
  void handleAcquiringBatch();
  void handleExtractingFeatures();
  void handleClassifying();
  void handleReportingResult();
  void handleDryingActive();
  void handleDryingReSampling();
  void handleDryingComplete();
  void handleFault();

  static bool isSafeCondition(BatchStatus status);

  IMoistureSensorArray& moistureSensors_;
  ITemperatureSensor& temperatureSensor_;
  IDisplay& display_;
  IStatusIndicator& statusIndicator_;
  IDryingActuator& dryingActuator_;
  IClassifier& classifier_;
  IClock& clock_;
  Config config_;

  SystemState state_ = SystemState::Idle;
  uint32_t stateEnteredAtMs_ = 0;

  BatchSample currentSample_{};
  BatchFeatures currentFeatures_{};
  BatchStatus lastResult_ = BatchStatus::Unknown;
};

} // namespace paddy
