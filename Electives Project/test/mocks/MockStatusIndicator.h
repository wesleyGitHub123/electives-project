#pragma once
#include "interfaces/IStatusIndicator.h"

namespace paddy_test {

class MockStatusIndicator : public paddy::IStatusIndicator {
 public:
  bool beginResult = true;
  paddy::BatchStatus lastStatus = paddy::BatchStatus::Unknown;
  int showFaultCallCount = 0;

  bool begin() override { return beginResult; }
  void showResult(paddy::BatchStatus status) override { lastStatus = status; }
  void showBusy() override {}
  void showFault() override { ++showFaultCallCount; }
  void clear() override {}
};

} // namespace paddy_test
