#pragma once

namespace paddy {
namespace config {

// Placeholder for future rule-based/ML threshold constants (e.g. moisture %
// or variability cutoffs used to decide StoreSafely vs. DryMore vs.
// HighRisk). Not consumed by any code yet -- IClassifier's current
// implementation is NotImplementedClassifier (see include/core/NotImplementedClassifier.h),
// which ignores thresholds entirely.
//
// TBD: real values require calibration against real paddy samples. Do not
// guess numbers here -- add fields only once they are actually known and
// wired into a classifier implementation.
struct ThresholdConfig {
  // Example future fields (intentionally left undefined):
  // float safeMoisturePercent;
  // float highRiskMoisturePercent;
  // float maxAcceptableVariability;
};

} // namespace config
} // namespace paddy
