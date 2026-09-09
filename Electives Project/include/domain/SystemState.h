#pragma once

namespace paddy {

// States of the single central state machine driven by BatchController.
// See docs/ARCHITECTURE.md for the full transition diagram.
enum class SystemState {
  Idle,
  AcquiringBatch,
  ExtractingFeatures,
  Classifying,
  ReportingResult,
  DryingActive,
  DryingReSampling,
  DryingComplete,
  Fault,
};

} // namespace paddy
