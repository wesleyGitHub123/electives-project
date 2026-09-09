#pragma once
#include "domain/SensorTypes.h"

namespace paddy {

// Reads all spatial moisture sensing points (4 perimeter + 1 central probe).
// Business/application logic (BatchController) only ever talks to this
// interface -- never to analogRead()/ADC/GPIO directly.
class IMoistureSensorArray {
 public:
  virtual ~IMoistureSensorArray() = default;

  // Initializes underlying hardware. Called once during controller startup.
  virtual bool begin() = 0;

  // Acquires one reading from every moisture sensing point.
  virtual void readAll(MoistureReading (&out)[kMoistureSensorCount]) = 0;
};

} // namespace paddy
