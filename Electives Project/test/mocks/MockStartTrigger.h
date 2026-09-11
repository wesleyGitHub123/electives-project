#pragma once
#include <cstddef>
#include "interfaces/IStartTrigger.h"

namespace paddy_test {

// sessionRequested defaults to true so existing batch-controller tests keep
// exercising the free-running (switch-on) path unmodified.
class MockStartTrigger : public paddy::IStartTrigger {
 public:
  bool beginResult = true;
  bool sessionRequested = true;

  bool begin() override { return beginResult; }
  bool isSessionRequested() const override { return sessionRequested; }
};

} // namespace paddy_test
