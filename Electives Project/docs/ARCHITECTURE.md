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

## Project status: paused

Work is checkpointed and paused here (2026-09-11). Resume context:

**Working:**
- Native unit tests: `pio test -e native` — 13 passing (feature extractor +
  batch controller, incl. switch gating and per-cycle display servicing).
- ESP32-S3 firmware build (`pio run -e esp32-s3-devkitc-1`).
- DS18B20 temperature driver; slide-switch session trigger (GPIO5,
  active-low, sampled in `Idle` only).
- WiFi status UI (see "WiFi status UI" under Wiring notes) — the
  go-forward visual sanity-check surface.

**Parked — 1.3" 128x64 I2C OLED.** A hardware/connectivity issue is
strongly indicated, not a driver/software bug: the I2C link works (0x3C
ACKs, `u8g2.begin()` succeeds) yet nothing ever renders, and
`endTransmission()` was observed flip-flopping between 0 (ACK) and 2 (NACK)
across resets on the same wiring, across multiple boards. Power integrity
and bus-level electrical issues were not conclusively ruled out, but the
module itself is now the prime suspect. The driver
(`Oled128x64Display`) is **kept and still compiles** — it is simply not
instantiated. Pending a replacement module.

**Next step when resuming** (pick one):
- Re-test the OLED with new hardware: swap
  `WifiUiDisplay display(networkConfig);` back for
  `Oled128x64Display display(pinConfig.display);` in `main.cpp` — one
  line, `IDisplay` does not change, nothing else moves.
- Continue the remaining stub adapters: real moisture-sensor driver, relay,
  status LEDs, real classifier (see "What is intentionally not
  implemented"). Sensor selection is pre-wiring R&D: characterize any
  candidate analog moisture sensor on the bench first — see
  [docs/BENCH_VALIDATION.md](BENCH_VALIDATION.md).

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
                                   Oled128x64Display [parked], WifiUiDisplay,
                                   RelayDryingActuator, ArduinoClock, GpioStartTrigger)
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
  touch hardware. `Ds18b20TemperatureSensor`, `GpioStartTrigger`, and
  `WifiUiDisplay` are real drivers; `Oled128x64Display` is a real driver
  but **parked** (flaky physical I2C — kept, still compiling, currently
  not instantiated); the rest are still stubs (see below).
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
  addresses are still the sentinel `kUnassignedPin`/`0x00` **except** three
  real mappings — `TemperatureSensorPinConfig::oneWirePin = 4` (DS18B20),
  `DisplayPinConfig` (OLED on I2C GPIO8=SDA / GPIO9=SCL @ 0x3C), and
  `StartTriggerPinConfig::switchPin = 5` (slide switch) — see "Wiring
  notes" below.
- **HAL adapter bodies** (`src/hal/*.cpp`): every adapter beyond
  `ArduinoClock` (wraps `millis()`), `Ds18b20TemperatureSensor` (real
  OneWire/DallasTemperature driver), `WifiUiDisplay` (real WiFi AP +
  HTTP status page), and `GpioStartTrigger` (real GPIO level read) is a
  `TODO(hardware)` stub returning safe placeholder values
  (`valid = false`, no-op renders, etc.). `Oled128x64Display` is also a
  real driver but **parked** (flaky physical I2C — see "Project status:
  paused"). The stubs compile and run safely on real hardware but do not
  yet read/drive anything.
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

### 1.3" 128x64 I2C OLED (implemented, parked)

Driver: `include/hal/Oled128x64Display.h` / `src/hal/Oled128x64Display.cpp`,
using `olikraus/U8g2@^2.36.18` (pinned in `platformio.ini`).

**Parked (2026-09-11): this module never rendered anything.** Evidence from
an extensive bring-up session (I2C scans, SH1106/SSD1306 driver swaps, an
isolated raw-U8g2 diagnostic sketch, multiple boards): the I2C link works
(0x3C ACKs, `u8g2.begin()` succeeds) but nothing renders, and
`endTransmission()` was observed flip-flopping between 0 (ACK) and 2 (NACK)
across resets on the same wiring — a hardware/connectivity issue is
strongly indicated (power integrity and bus-level electrical issues not
fully ruled out), not a driver/software bug. The driver is kept in the
tree and still compiles (the esp32 env has no `build_src_filter`, so it
builds even though `main.cpp` no longer instantiates it — deliberate, so
parked code cannot bit-rot). Resume with a replacement module; the content
scope below still applies.

- **Pins: GPIO8=SDA, GPIO9=SCL** (ESP32-S3 Arduino-core default Wire pins,
  free, non-strapping). **Power: 3V3 + GND.** I2C pull-ups are required —
  the FEIYANG module includes them on-board.
- **Driver chip: both SH1106 and SSD1306 were tried — neither rendered**
  (consistent with the parking rationale above). The .cpp currently
  instantiates SSD1306 (`U8G2_SSD1306_128X64_NONAME_F_HW_I2C`); when
  re-testing with a replacement module, swap that single constructor line
  for `U8G2_SH1106_128X64_NONAME_F_HW_I2C` — one line, nothing else
  changes.
- **Address: 0x3C configured, not trusted.** `begin()` scans the whole bus
  and logs every device it finds (`[paddy][oled] found device at 0x..`);
  if the real address differs, update `DisplayPinConfig::i2cAddress`.
- `begin()` is lenient: a missing/miswired OLED does not Fault the system —
  it just stays blank (Serial remains the feedback channel).
- **Content is deliberately minimal (bring-up scope):** state name + switch
  hint (`showState`), result status + temperature (`showResult`), fault
  message. No moisture/variability UI yet — stub sensors would only render
  noise. Expand `showResult` once real moisture data exists.

### Start slide switch (implemented)

Driver: `include/hal/GpioStartTrigger.h` / `src/hal/GpioStartTrigger.cpp`.

- **Pin: GPIO5** — free, not a strapping/USB/flash pin. **Wiring: one side
  of the slide switch to GPIO5, the other to GND** — `INPUT_PULLUP` makes
  an external resistor unnecessary.
- **Polarity: `activeLow = true`** (pin LOW = session requested). Confirm
  which physical position is "on" via the Serial `startSwitch=` trace; if
  reversed, flip the one bool in `PinConfig.h`.
- **Gating semantics (deliberate):** the switch is sampled only in `Idle`
  (`BatchController::handleIdle`). A session in progress always runs to
  completion even if the switch flips off mid-cycle — it only prevents the
  *next* session. An abort/e-stop behavior would need its own safety
  design; not guessed here.

### WiFi status UI (implemented, replaces the parked OLED for now)

Driver: `include/hal/WifiUiDisplay.h` / `src/hal/WifiUiDisplay.cpp`, using
the `WiFi.h` / `WebServer.h` headers shipped with the `espressif32`
platform (no new `lib_deps`). Config:
`include/config/NetworkConfig.h`.

- **The device hosts its own access point** (`WiFi.softAP`): SSID
  `PaddyMonitor`, WPA2 password `paddy1234` — development-only defaults
  for a local AP the device itself creates, not real-world secrets and not
  a security model. Join the AP from a phone/laptop and browse to
  **http://192.168.4.1**. Switching to station mode (join an existing
  network) is a small change in `WifiUiDisplay::begin()` if ever wanted.
- **Page content (same minimal bring-up scope the parked OLED had):**
  state, last result, temperature (`--` until real data), start-switch
  hint (`waiting` while `Idle`, `ON` otherwise — same heuristic the OLED
  used), and the fault message if any. Plain server-side HTML, auto-refresh
  via `<meta http-equiv="refresh" content="2">` — no JavaScript, no client
  build step.
- **Observational only:** the request handler renders cached read-only
  snapshots fed by `showState`/`showResult`/`showFault` on transitions; it
  never reaches back into `BatchController` or any live internals. The
  slide switch remains the sole session-start control path.
- **Servicing:** `server_.handleClient()` runs on every `update()` call via
  the `IDisplay::poll()` hook — the first line of
  `BatchController::update()`, so it runs even in `Fault`. `main.cpp`
  never touches the adapter directly.
- **Lenient like every other display adapter:** a WiFi/AP failure does not
  Fault the system; Serial remains the feedback channel.
- **Serial lines to expect** (the proof-of-life, verifiable without ever
  joining the AP): on boot —
  `[paddy][wifi-ui] softAP("PaddyMonitor") = started`,
  `[paddy][wifi-ui] browse to http://192.168.4.1`,
  `[paddy][wifi-ui] HTTP server started` — then
  `[paddy][wifi-ui] served request #N` per page load.

## Current on-device behavior

With the start switch off, the firmware sits in `Idle` — one
`[paddy] state -> Idle | startSwitch=OFF` line over Serial (115200), then
silence. The device simultaneously hosts the `PaddyMonitor` WiFi AP: a
phone/laptop joining it can watch state / last result / temperature at
http://192.168.4.1 (auto-refresh every 2s), and each page load increments
the `[paddy][wifi-ui] served request #N` Serial line. Flip the switch on
and the cycle runs continuously: `Idle -> AcquiringBatch ->
ExtractingFeatures -> Classifying -> ReportingResult (Unknown) -> Idle`,
mirrored on the WiFi status page. `DryingActive` is never reached because
the classifier never reports `DryMore` — the safe, expected behavior until
a real classifier is wired in. Builds clean for an ESP32-S3 DevKitC-1;
the WiFi page itself has not yet been flash-verified (next step on
resume), whereas the parked-OLED observations above date from physical
bring-up sessions.

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
