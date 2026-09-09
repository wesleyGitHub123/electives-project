#pragma once
#include "interfaces/IClassifier.h"

namespace paddy_test {

class MockClassifier : public paddy::IClassifier {
 public:
  paddy::BatchStatus nextResult = paddy::BatchStatus::Unknown;
  int classifyCallCount = 0;

  paddy::BatchStatus classify(const paddy::BatchFeatures&) override {
    ++classifyCallCount;
    return nextResult;
  }
};

} // namespace paddy_test
