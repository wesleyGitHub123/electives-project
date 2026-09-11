#pragma once
#include "interfaces/IDisplay.h"

namespace paddy_test {

class MockDisplay : public paddy::IDisplay {
 public:
  bool beginResult = true;
  paddy::SystemState lastState = paddy::SystemState::Idle;
  paddy::BatchStatus lastStatus = paddy::BatchStatus::Unknown;
  int showResultCallCount = 0;
  int showFaultCallCount = 0;
  int pollCallCount = 0;

  bool begin() override { return beginResult; }
  void showState(paddy::SystemState state) override { lastState = state; }

  void poll() override { ++pollCallCount; }

  void showResult(paddy::BatchStatus status, const paddy::BatchFeatures&) override {
    lastStatus = status;
    ++showResultCallCount;
  }

  void showFault(const char*) override { ++showFaultCallCount; }
};

} // namespace paddy_test
