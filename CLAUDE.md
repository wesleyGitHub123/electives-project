# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project overview

Firmware for a portable, battery-powered paddy (rice) moisture and
spoilage-risk assessment system for smallholder farmers, in
`Electives Project/` (PlatformIO project root, ESP32-S3 DevKitC-1, Arduino
framework — see [Electives Project/platformio.ini](Electives%20Project/platformio.ini)).

Five spatial moisture sensing points (4 around the sack + 1 central probe)
plus a temperature sensor feed the ESP32-S3, which computes batch features
(mean moisture, moisture variability, temperature), classifies the batch as
**Store Safely / Dry More / High Risk** via an embedded ML classifier
running fully offline, reports the result on an OLED + LEDs, and — if
needed — drives a relay-controlled heater/blower until a safe condition is
reached.

Not a git repository.

## Architecture

Full design doc: [Electives Project/docs/ARCHITECTURE.md](Electives%20Project/docs/ARCHITECTURE.md) — read it before
making non-trivial changes.

The firmware is dependency-inverted: application/domain logic
(`core/BatchController`, the single central state machine) talks only to
interfaces (`interfaces/I*.h` — sensors, display, drying actuator,
classifier, clock), never to Arduino/ESP32 APIs directly. Concrete hardware
adapters live in `hal/` and are the only code allowed to touch
`analogRead()`/GPIO/I2C/`millis()`. `main.cpp` is a thin composition root.
All pin mapping, timing, and threshold constants live in
`include/config/` — never as magic numbers elsewhere.

This is currently a **scaffold**, not a finished product: every HAL
adapter body (except `ArduinoClock`) is a `TODO(hardware)` stub, every pin
in `PinConfig.h` is an unassigned placeholder, the classifier
(`NotImplementedClassifier`) always returns `Unknown` rather than faking a
prediction, and there is no production drying algorithm yet. See
"What is intentionally not implemented" in ARCHITECTURE.md for the full
list before assuming any hardware behavior is real.

## Commands

Run from `Electives Project/` (where `platformio.ini` lives), using the
PlatformIO CLI (`pio`):

- Build firmware: `pio run -e esp32-s3-devkitc-1`
- Upload to a connected board: `pio run -e esp32-s3-devkitc-1 -t upload`
- Serial monitor: `pio device monitor`
- Run native (hardware-free) unit tests: `pio test -e native`
  - Tests live in `test/test_feature_extractor/` and
    `test/test_batch_controller/`, using mocks in `test/mocks/`, against
    the hardware-free `core/` layer.
  - **On this machine**, `pio test -e native` requires `C:\MinGW\bin`
    ahead of other MinGW/MSYS installs on `PATH`, or the compiled test
    binary fails to load (`STATUS_ENTRYPOINT_NOT_FOUND`) due to a
    conflicting `libstdc++`. From Git Bash:
    `PATH="/c/MinGW/bin:$PATH" pio test -e native`.
- Clean build artifacts: `pio run --target clean`

Build output goes to `.pio/build/` (gitignored). Also usable via the
PlatformIO IDE extension for VS Code (see `.vscode/`).
