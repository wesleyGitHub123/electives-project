#include "hal/Ds18b20TemperatureSensor.h"

namespace paddy {

namespace {
constexpr uint8_t kResolutionBits = 12; // DallasTemperature default; ~750ms conversion
} // namespace

Ds18b20TemperatureSensor::Ds18b20TemperatureSensor(const TemperatureSensorPinConfig& pins)
    : pins_(pins),
      oneWire_(static_cast<uint8_t>(pins.oneWirePin)),
      sensors_(&oneWire_) {}

bool Ds18b20TemperatureSensor::begin() {
  sensors_.begin();
  sensors_.setResolution(kResolutionBits);
  // Lenient on purpose: don't hard-fail BatchController::begin() (which
  // would leave the whole system stuck in Fault) just because no DS18B20
  // answered yet -- e.g. during bring-up before it's wired. readAll()
  // reports per-read validity instead, via TemperatureReading::valid.
  return true;
}

void Ds18b20TemperatureSensor::readAll(
    TemperatureReading (&out)[kTemperatureSensorCount]) {
  // Blocking ~750ms at 12-bit resolution (DallasTemperature's default) --
  // acceptable for now since sampling isn't latency-sensitive yet. Revisit
  // with setWaitForConversion(false) if AcquiringBatch needs to stay fast.
  // Today kTemperatureSensorCount == 1, so this reads the single device on
  // the bus (getTempCByIndex(0)); when the count grows, this should iterate
  // device addresses instead of indices.
  sensors_.requestTemperatures();

  for (auto& reading : out) {
    const float celsius = sensors_.getTempCByIndex(0);
    if (celsius == DEVICE_DISCONNECTED_C) {
      reading.value = 0.0f;
      reading.valid = false;
    } else {
      reading.value = celsius;
      reading.valid = true;
    }
  }
}

} // namespace paddy
