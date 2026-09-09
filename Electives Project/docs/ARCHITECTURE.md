# Architecture

Firmware for a portable, battery-powered paddy moisture and spoilage-risk
assessment system. Five spatial moisture sensing points (4 around the sack
+ 1 central probe) plus one temperature sensor feed an ESP32-S3, which
computes batch features, classifies the batch (Store Safely / Dry More /
High Risk), reports the result on an OLED + LEDs, and — if needed — drives
a relay-controlled heater/blower until a safe condition is reached.

This document describes the **scaffold**: a compiling, flashable,
dependency-inverted skeleton with the real state machine and pure logic in
place, but no sensor drivers, no trained classifier, and no drying
algorithm yet. See "What is intentionally not implemented" below.

## Layering and the dependency-inversion rule

```
main.cpp (composition root)
   |
   v
core/BatchController  <-- talks only to interfaces/*, never to hardware
   |            \
   v             v
core/FeatureExtractor   interfaces/I*.h  (IMoistureSensorArray, ITemperatureSensor,
   (pure math)              IDisplay, IStatusIndicator, IDryingActuator,
                             IClassifier, IClock)
                                   ^
                                   |
                          hal/*  (Esp32MoistureSensorArray, Ds18b20TemperatureSensor,
                                   Ssd1306StatusDisplay, GpioStatusIndicator,
                                   RelayDryingActuator, ArduinoClock)
                                   |
                                   v
                          Arduino/ESP32 APIs (analogRead, GPIO, I2C, millis, ...)
```

- `include/domain/` — plain data/enums (`MoistureReading`, `BatchSample`,
  `BatchFeatures`, `BatchStatus`, `SystemState`). No behavior.
- `include/interfaces/` — the only vocabulary `BatchController` is allowed
  to use to reach the outside world. Business logic never calls
  `analogRead()`, `digitalWrite()`, an OLED driver, or `millis()` directly.
- `include/core/` + `src/core/` — hardware-free application logic:
  `FeatureExtractor` (pure math) and `BatchController` (the one central
  state machine). Zero `#include <Arduino.h>`. This is what native unit
  tests exercise directly.
- `include/hal/` + `src/hal/` — concrete adapters implementing the
  interfaces using Arduino/ESP32 APIs. This is the *only* place allowed to
  touch hardware. `Ds18b20TemperatureSensor` is a real driver; the rest are
  still stubs (see below).
- `include/config/` — the single visible place for hardware pin mapping
  (`PinConfig.h`), timing tunables (`SystemConfig.h`), and future
  classification thresholds (`ThresholdConfig.h`). No magic numbers
  anywhere else.
- `src/main.cpp` — thin composition root: constructs the HAL adapters,
  wires them into one `BatchController`, and pumps `begin()`/`update()`.
  No business logic here beyond debug Serial tracing.
- `test/mocks/` + `test/test_feature_extractor/`, `test/test_batch_controller/`
  — native (PC-hosted) unit tests against `core/`, using mock adapters
  instead of real hardware.

Swapping hardware (a different moisture sensor, a different display) means
writing a new class in `hal/` that implements the same interface —
`BatchController` and its tests never change.

## The state machine

`BatchController::update()` advances exactly one step per call (never
blocks). States:

```
Idle -> AcquiringBatch -> ExtractingFeatures -> Classifying -> ReportingResult
                                                                    |
                                             DryMore -----> DryingActive
                                             other   -----> Idle
                                                                    |
                                     DryingActive <-> DryingReSampling
                                             (loops until "safe")
                                                                    |
                                                          DryingComplete -> Idle

begin() -> Fault   (if any hardware adapter fails to initialize; terminal)
```

- `AcquiringBatch`: reads all 5 moisture points + temperature via
  `IMoistureSensorArray` / `ITemperatureSensor`.
- `ExtractingFeatures`: `FeatureExtractor::extract()` computes mean
  moisture, moisture variability (population standard deviation across
  valid readings), and passes through temperature. Pure function, fully
  unit tested.
- `Classifying`: `IClassifier::classify()` produces a `BatchStatus`.
- `ReportingResult`: shows the result (`IDisplay`, `IStatusIndicator`),
  held for `Config::resultDisplayHoldMs`, then branches to `DryingActive`
  (on `DryMore`) or back to `Idle`.
- `DryingActive` / `DryingReSampling`: while drying, re-samples and
  re-classifies every `Config::dryingRecheckIntervalMs`; stops once
  `isSafeCondition()` is true.

Timing is driven entirely through `IClock`, never `millis()` directly, so
every transition — including the hold/recheck delays — is deterministically
testable with `test/mocks/MockClock.h`.

## What is intentionally not implemented (TBD)

Per project scope, this scaffold stops short of a finished product:

- **Hardware pin mapping** (`include/config/PinConfig.h`): all pin/I2C
  addresses are still the sentinel `kUnassignedPin`/`0x00` **except**
  `TemperatureSensorPinConfig::oneWirePin = 4` (DS18B20), which is real —
  see "Wiring notes" below for why GPIO4 and what's required.
- **HAL adapter bodies** (`src/hal/*.cpp`): every method beyond
  `ArduinoClock` (wraps `millis()`) and `Ds18b20TemperatureSensor` (real
  OneWire/DallasTemperature driver) is a `TODO(hardware)` stub returning
  safe placeholder values (`valid = false`, no-op renders, etc.). They
  compile and run safely on real hardware but do not yet read/drive
  anything.
- **Classifier** (`include/core/NotImplementedClassifier.h`): always
  returns `BatchStatus::Unknown`. Deliberately not a fake Random Forest —
  it never pretends to have a real prediction, so `BatchController` can
  never accidentally start drying based on a bogus classification. A real
  embedded classifier plugs in by implementing `IClassifier` and swapping
  it in `main.cpp`.
- **Classification thresholds** (`include/config/ThresholdConfig.h`):
  placeholder struct, no fields yet — do not invent moisture/temperature
  cutoffs before they're calibrated against real data.
- **Production drying algorithm**: `isSafeCondition()` in
  `BatchController.cpp` currently just checks `status == StoreSafely`.
  Real cycle timing/safety logic is future work.

## Wiring notes

### DS18B20 temperature sensor (implemented)

Driver: `include/hal/Ds18b20TemperatureSensor.h` /
`src/hal/Ds18b20TemperatureSensor.cpp`, using `paulstoffregen/OneWire@2.3.8`
+ `milesburton/DallasTemperature@4.0.6` (pinned in `platformio.ini`).

- **Data pin: GPIO4.** Chosen because it's broken out on every
  ESP32-S3-DevKitC-1 variant and isn't a strapping/USB/flash/PSRAM pin —
  not a hardware-mandated choice, just the first safe/free pin; change
  `PinConfig::temperature.oneWirePin` if you'd rather use a different one.
- **Power: 3V3**, not 5V — matches the ESP32-S3's 3.3V GPIO logic level
  directly (DS18B20 supports 3.0–5.5V, so 3.3V is within spec).
- **Pull-up resistor: an external ~4.7kΩ resistor between DAT and 3V3 is
  required** unless the breakout board already has one on-board — check
  the physical board before wiring; most bare 3-pin (VCC/DAT/GND) terminal
  breakouts do not include one, and it becomes more important, not less,
  over a longer cable (this module ships with a ~100cm probe cable).
- `begin()` is lenient (always returns `true`) so a not-yet-connected
  sensor doesn't put the whole system in `Fault`; `readAll()` reports
  per-read validity instead via `TemperatureReading::valid`.
- `requestTemperatures()` blocks for ~750ms at the default 12-bit
  resolution. Acceptable for now; revisit with
  `setWaitForConversion(false)` if `AcquiringBatch` needs to stay fast.

Debug builds print the live `BatchFeatures` (including `temperatureC=`)
over Serial on every `ReportingResult`/`DryingComplete` — see `main.cpp` —
so real sensor values can be sanity-checked without extra tooling.

## Current on-device behavior

With all adapters stubbed, `begin()` succeeds (every stub returns `true`),
and the firmware free-runs: `Idle -> AcquiringBatch -> ExtractingFeatures ->
Classifying -> ReportingResult (Unknown) -> Idle`, forever, logged over
Serial at 115200 baud. This was verified by flashing an ESP32-S3
DevKitC-1. `DryingActive` is never reached in this state because the
classifier never reports `DryMore` — this is the safe, expected behavior
until a real classifier is wired in.

## Extending this scaffold

- **New sensor/actuator hardware**: implement the relevant `hal/`
  interface; fill in `include/config/PinConfig.h` with real pins once
  wiring is finalized; construct it in `main.cpp`.
- **Real classifier**: implement `IClassifier` (e.g. wrapping a
  micromlgen/emlearn-generated Random Forest), populate
  `ThresholdConfig.h` if it uses rule-based thresholds, and swap it in for
  `NotImplementedClassifier` in `main.cpp`.
- **Drying algorithm**: extend `BatchController::isSafeCondition()` and/or
  the `DryingActive`/`DryingReSampling` handlers once the real algorithm is
  designed — the state machine shape is already in place.
