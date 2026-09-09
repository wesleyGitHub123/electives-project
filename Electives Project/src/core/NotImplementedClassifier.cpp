#include "core/NotImplementedClassifier.h"

namespace paddy {

BatchStatus NotImplementedClassifier::classify(const BatchFeatures& /*features*/) {
  return BatchStatus::Unknown;
}

} // namespace paddy
