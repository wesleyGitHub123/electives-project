#include "core/BatchController.h"
#include "core/FeatureExtractor.h"

namespace paddy {

BatchController::BatchController(IMoistureSensorArray& moistureSensors,
                                  ITemperatureSensor& temperatureSensor,
                                  IDisplay& display,
                                  IStatusIndicator& statusIndicator,
                                  IDryingActuator& dryingActuator,
                                  IClassifier& classifier,
                                  IClock& clock,
                                  IStartTrigger& startTrigger,
                                  Config config)
    : moistureSensors_(moistureSensors),
      temperatureSensor_(temperatureSensor),
      display_(display),
      statusIndicator_(statusIndicator),
      dryingActuator_(dryingActuator),
      classifier_(classifier),
      clock_(clock),
      startTrigger_(startTrigger),
      config_(config) {}

void BatchController::begin() {
  const bool ok = moistureSensors_.begin() &&
                   temperatureSensor_.begin() &&
                   display_.begin() &&
                   statusIndicator_.begin() &&
                   dryingActuator_.begin() &&
                   startTrigger_.begin();

  enterState(ok ? SystemState::Idle : SystemState::Fault);
}

void BatchController::update() {
  // Serviced every cycle, regardless of state -- e.g. a WiFi display adapter
  // needs server.handleClient() even while sitting in Idle or Fault.
  display_.poll();

  switch (state_) {
    case SystemState::Idle:               handleIdle(); break;
    case SystemState::AcquiringBatch:     handleAcquiringBatch(); break;
    case SystemState::ExtractingFeatures: handleExtractingFeatures(); break;
    case SystemState::Classifying:        handleClassifying(); break;
    case SystemState::ReportingResult:    handleReportingResult(); break;
    case SystemState::DryingActive:       handleDryingActive(); break;
    case SystemState::DryingReSampling:   handleDryingReSampling(); break;
    case SystemState::DryingComplete:     handleDryingComplete(); break;
    case SystemState::Fault:              handleFault(); break;
  }
}

void BatchController::enterState(SystemState next) {
  state_ = next;
  stateEnteredAtMs_ = clock_.nowMillis();
  display_.showState(state_);
}

bool BatchController::elapsedSinceStateEntry(uint32_t durationMs) const {
  return (clock_.nowMillis() - stateEnteredAtMs_) >= durationMs;
}

void BatchController::handleIdle() {
  // Level-triggered, checked only here: a session already in progress
  // always runs to completion even if the switch flips off mid-cycle.
  // This gates when a session *starts*, not whether hardware may keep
  // running -- an abort/e-stop behavior would need its own safety design.
  if (startTrigger_.isSessionRequested()) {
    enterState(SystemState::AcquiringBatch);
  }
}

void BatchController::handleAcquiringBatch() {
  statusIndicator_.showBusy();
  moistureSensors_.readAll(currentSample_.moisture);
  temperatureSensor_.readAll(currentSample_.temperature);
  enterState(SystemState::ExtractingFeatures);
}

void BatchController::handleExtractingFeatures() {
  currentFeatures_ = FeatureExtractor::extract(currentSample_);
  enterState(SystemState::Classifying);
}

void BatchController::handleClassifying() {
  lastResult_ = classifier_.classify(currentFeatures_);
  enterState(SystemState::ReportingResult);
}

void BatchController::handleReportingResult() {
  display_.showResult(lastResult_, currentFeatures_);
  statusIndicator_.showResult(lastResult_);

  if (!elapsedSinceStateEntry(config_.resultDisplayHoldMs)) {
    return;
  }

  if (lastResult_ == BatchStatus::DryMore) {
    enterState(SystemState::DryingActive);
  } else {
    enterState(SystemState::Idle);
  }
}

void BatchController::handleDryingActive() {
  if (!dryingActuator_.isRunning()) {
    dryingActuator_.start();
  }

  if (elapsedSinceStateEntry(config_.dryingRecheckIntervalMs)) {
    enterState(SystemState::DryingReSampling);
  }
}

void BatchController::handleDryingReSampling() {
  moistureSensors_.readAll(currentSample_.moisture);
  temperatureSensor_.readAll(currentSample_.temperature);

  currentFeatures_ = FeatureExtractor::extract(currentSample_);
  lastResult_ = classifier_.classify(currentFeatures_);

  if (isSafeCondition(lastResult_)) {
    enterState(SystemState::DryingComplete);
  } else {
    enterState(SystemState::DryingActive);
  }
}

void BatchController::handleDryingComplete() {
  dryingActuator_.stop();
  display_.showResult(lastResult_, currentFeatures_);
  statusIndicator_.showResult(lastResult_);
  enterState(SystemState::Idle);
}

void BatchController::handleFault() {
  display_.showFault("hardware init failed");
  statusIndicator_.showFault();
  // Terminal for now -- TBD: define a recovery/retry policy.
}

bool BatchController::isSafeCondition(BatchStatus status) {
  // TBD: current definition of "safe to stop drying" is deliberately
  // minimal -- revisit once the real classifier and drying algorithm are
  // designed.
  return status == BatchStatus::StoreSafely;
}

} // namespace paddy
