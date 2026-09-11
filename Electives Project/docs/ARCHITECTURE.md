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

## Status

**Working:**
- Native unit tests: `pio test -e native` — 13 passing (feature extractor +
  batch controller, incl. switch gating and per-cycle display servicing).
- ESP32-S3 firmware build (`pio run -e esp32-s3-devkitc-1`).
- DS18B20 temperature driver; slide-switch session trigger (GPIO5,
  active-low, sampled in `Idle` only).
- 1.3" 128x64 I2C OLED (SH1106) — the active display (see "Wiring notes"
  below). An earlier module never rendered anything despite a working I2C
  link; that turned out to be a bad physical connection on that specific
  module/wiring, not a driver bug — the replacement module works with the
  same driver code once rewired correctly.
- Moisture sensing: one of five channels is real (raw 12-bit ADC on
  GPIO6); the other four remain unwired stubs (`kUnassignedPin`). Values
  are deliberately raw/unscaled — see `MoistureCalibrationConfig` in
  `PinConfig.h` and [docs/BENCH_VALIDATION.md](BENCH_VALIDATION.md) for why.
- WiFi status UI — kept in the tree as a proven fallback display (see
  "WiFi status UI" under Wiring notes), not currently instantiated in
  `main.cpp`.

**Next step:** continue the remaining stub adapters — the other 4 moisture
channels, relay, status LEDs, real classifier (see "What is intentionally
not implemented"). Moisture sensor model selection is still pre-wiring
R&D for the unwired channels: characterize any candidate analog sensor on
the bench first — see [docs/BENCH_VALIDATION.md](BENCH_VALIDATION.md).

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
                                   Oled128x64Display [active], WifiUiDisplay [fallback],
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
  `Oled128x64Display` (the active display) are real drivers;
  `WifiUiDisplay` is also a real driver, kept as a proven fallback display
  (currently not instantiated); `Esp32MoistureSensorArray` is real for one
  of five channels (raw ADC, GPIO6) with the rest stubbed; the remaining
  adapters are still stubs (see below).
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
  addresses are still the sentinel `kUnassignedPin`/`0x00` **except** real
  mappings for `TemperatureSensorPinConfig::oneWirePin = 4` (DS18B20),
  `DisplayPinConfig` (OLED on I2C GPIO8=SDA / GPIO9=SCL @ 0x3C),
  `StartTriggerPinConfig::switchPin = 5` (slide switch), and
  `MoistureSensorPinConfig::adcPins[0] = 6` (one of five moisture
  channels) — see "Wiring notes" below.
- **HAL adapter bodies** (`src/hal/*.cpp`): every adapter beyond
  `ArduinoClock` (wraps `millis()`), `Ds18b20TemperatureSensor` (real
  OneWire/DallasTemperature driver), `Oled128x64Display` (real U8g2/SH1106
  driver, the active display), `GpioStartTrigger` (real GPIO level read),
  and `Esp32MoistureSensorArray` (real raw-ADC read, one of five channels)
  is a `TODO(hardware)` stub returning safe placeholder values
  (`valid = false`, no-op renders, etc.). `WifiUiDisplay` is also a real
  driver (WiFi AP + HTTP status page), kept as a fallback but not
  currently instantiated. The stubs compile and run safely on real
  hardware but do not yet read/drive anything.
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

### 1.3" 128x64 I2C OLED (implemented, active)

Driver: `include/hal/Oled128x64Display.h` / `src/hal/Oled128x64Display.cpp`,
using `olikraus/U8g2@^2.36.18` (pinned in `platformio.ini`).

**Working, as of a replacement module (2026-09-11).** An earlier module
never rendered anything despite a working-looking I2C link (0x3C ACKs,
`u8g2.begin()` succeeds), with `endTransmission()` observed flip-flopping
between ACK/NACK across resets on the same wiring — strong evidence of a
bad physical connection on that specific module/wiring, not a driver bug.
The replacement module (confirmed SH1106 via its listing) hit the same
"I2C scan finds nothing" symptom at first, traced to loose/incorrect
jumper wiring; once rewired, it renders correctly with the same driver
code, no logic changes needed. Lesson: this class of "I2C link looks
broken" failure has twice now turned out to be a connection problem, not
a chip/driver problem — worth checking wiring thoroughly before
suspecting the code or the chip.

- **Pins: GPIO8=SDA, GPIO9=SCL** (ESP32-S3 Arduino-core default Wire pins,
  free, non-strapping). **Power: 3V3 + GND.** I2C pull-ups are required —
  most 4-pin OLED breakout boards include them on-board.
- **Driver chip: SH1106**, confirmed by the module's listing and by it
  rendering correctly — `U8G2_SH1106_128X64_NONAME_F_HW_I2C` in the `.cpp`.
- **Address: 0x3C configured, not trusted.** `begin()` scans the whole bus
  and logs every device it finds (`[paddy][oled] found device at 0x..`);
  if the real address differs, update `DisplayPinConfig::i2cAddress`.
- `begin()` is lenient: a missing/miswired OLED does not Fault the system —
  it just stays blank (Serial remains the feedback channel).
- **Content:** state name + switch hint (`showState`), fault message
  (`showFault`) — both via `renderTwoLines()`; result + temperature +
  moisture mean (`showResult`) via `renderThreeLines()`. Both helpers use
  the same compact font (`u8g2_font_7x13_tr`) for a consistent look — an
  earlier larger font (`u8g2_font_10x20_tr`) rendered oversized on the
  actual panel. Moisture is raw/unitless, gated on the independent
  `moistureValid` flag (never the whole-batch `valid` gate) — same
  convention as the WiFi page. Per-point moisture still isn't renderable:
  no interface carries per-point data to the display layer yet.

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

### WiFi status UI (implemented, kept as fallback display)

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
- **Page content:** a centered dark card showing state, a color-coded
  result badge, start-switch hint (`waiting` while `Idle`, `ON`
  otherwise), temperature, and moisture mean/variability (raw/unitless,
  gated on `moistureValid`) plus five static "not wired" per-point rows
  (no interface carries real per-point data yet). Plain server-side HTML,
  auto-refresh via `<meta http-equiv="refresh" content="2">` — no
  JavaScript, no client build step.
- **Currently not instantiated in `main.cpp`** — `Oled128x64Display` is
  the active display now that it's confirmed working. Swap back with one
  line if needed: `Oled128x64Display display(pinConfig.display);` ->
  `WifiUiDisplay display(networkConfig);` (and use `networkConfig`, kept
  declared in `main.cpp` for exactly this).
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
silence, mirrored on the OLED (`Idle` / `Switch: waiting`). Flip the
switch on and the cycle runs continuously: `Idle -> AcquiringBatch ->
ExtractingFeatures -> Classifying -> ReportingResult (Unknown) -> Idle`,
with the OLED's result screen showing `Res: Unknown`, the live GPIO6
moisture reading, and the real DS18B20 temperature. `DryingActive` is
never reached because the classifier never reports `DryMore` — the safe,
expected behavior until a real classifier is wired in. Confirmed on real
hardware (ESP32-S3 DevKitC-1, replacement SH1106 module). The WiFi UI
described above is flash-verified but not currently active in `main.cpp`.

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
