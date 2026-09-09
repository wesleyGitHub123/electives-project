#pragma once
#include "domain/BatchStatus.h"

namespace paddy {

// Simple at-a-glance indication (e.g. LEDs) mirroring the current result.
class IStatusIndicator {
 public:
  virtual ~IStatusIndicator() = default;

  virtual bool begin() = 0;
  virtual void showResult(BatchStatus status) = 0;
  virtual void showBusy() = 0;
  virtual void showFault() = 0;
  virtual void clear() = 0;
};

} // namespace paddy
