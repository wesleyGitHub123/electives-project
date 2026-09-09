#pragma once
#include "domain/SensorTypes.h"
#include "domain/BatchStatus.h"

namespace paddy {

// Boundary for the embedded ML classifier. The real Random Forest model is
// NOT part of this scaffold (TBD) -- see include/core/NotImplementedClassifier.h
// for the current placeholder and docs/ARCHITECTURE.md for how a real one
// plugs in later.
class IClassifier {
 public:
  virtual ~IClassifier() = default;
  virtual BatchStatus classify(const BatchFeatures& features) = 0;
};

} // namespace paddy
