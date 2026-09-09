#pragma once
#include "interfaces/IDryingActuator.h"

namespace paddy_test {

class MockDryingActuator : public paddy::IDryingActuator {
 public:
  bool beginResult = true;
  bool running = false;
  int startCallCount = 0;
  int stopCallCount = 0;

  bool begin() override { return beginResult; }
  void start() override { running = true; ++startCallCount; }
  void stop() override { running = false; ++stopCallCount; }
  bool isRunning() const override { return running; }
};

} // namespace paddy_test
