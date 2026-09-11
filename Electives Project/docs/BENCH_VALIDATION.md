# Moisture sensor bench validation

**Status: pre-integration R&D.** This is NOT part of the shipped firmware
and describes work that happens *before* any sensor is chosen, purchased,
or wired. Production behavior is documented in ARCHITECTURE.md only once
hardware is physically confirmed — that separation is deliberate.

## Scope

A generic procedure for characterizing **any single-analog-output
capacitive moisture sensor** on a bench, before it is chosen. The group is
currently comparing several candidate modules (marketplace listings
included); this document is deliberately **not tied to any specific
listing or model** — whichever candidate is under evaluation gets run
through the same steps.

What it produces: raw ADC behavior data (dry baseline, wet baseline,
noise floor, unit-to-unit variance, drift) that lets the group choose a
sensor model on evidence rather than on a listing's marketing copy.

## Safety — read before wiring anything

> **⚠️ Analog output voltage can scale with supply voltage.** Many
> capacitive moisture modules are sold as "3.3–5V", but that range refers
> to the *supply* — the AOUT pin can swing close to VCC. Feeding a
> 5V-referenced AOUT into the ESP32-S3's **3.3V-only** ADC pin risks
> damaging the pin or the whole SoC.
>
> - Bench at **3.3V wherever the module supports it** (most do).
> - Confirm the specific candidate's listing/datasheet **before wiring
>   anything** — do not trust "3.3–5V" marketing copy.
> - If 5V operation is genuinely required, put a resistor divider or
>   logic-level shifter between AOUT and the ESP32 pin.

## Generic wiring

| Sensor pin | Connects to |
|---|---|
| VCC | 3V3 (see safety note above) |
| GND | GND |
| AOUT | any ADC-capable GPIO, set at the top of `bench/moisture_bench_adc.cpp` (`kBenchAdcPin`) |

No pull-ups, no external components expected; the ADC input is high
impedance. If readings look wildly noisy, re-check the common ground first
before suspecting the sensor.

## Companion tool: `bench/moisture_bench_adc`

A standalone diagnostic (deliberately **outside** the DI architecture —
not composed through `main.cpp`/`hal`/`core`; it follows the project's
throwaway-probe precedent from the DS18B20/OLED bring-ups, but kept as a
durable tool). It lives in a **self-contained PlatformIO project** in
`bench/` (its own `platformio.ini` + `src/`), so it is physically isolated
from the production firmware tree — nothing from `src/`, `include/`, or
the DI architecture can leak into a bench build:

```
pio run -d bench -t upload     # separate upload, NOT the main firmware
pio device monitor             # 115200 baud
```

Both commands run from `Electives Project/`. It prints one `raw,voltage`
CSV line every 500 ms. Paste the values straight into the recording table
below. Edit `kBenchAdcPin` at the top of `bench/src/moisture_bench_adc.cpp`
to match whatever GPIO the candidate's AOUT is currently jumpered to — it
is deliberately NOT sourced from `include/config/PinConfig.h`, because
this tool predates any wiring decision.

## Procedure

For each candidate sensor (and each unit, if multiple are bought):

1. **Dry-air baseline.** Sensor in open air, nothing touching the sensing
   face. Let it run ~30 s; record several samples (the tool logs
   continuously — pick 5+ well-separated lines). Note the noise floor
   (max−min of the raw counts). A noisy unit here will be noisy everywhere.
2. **Water-immersion baseline.** Immerse the sensing area **up to the
   module's marked line, not past it onto the PCB**. Again: multiple
   samples after a settle period. This is the other endpoint.
3. **Optional intermediate point** — e.g. a damp paper towel wrapped
   around the sensing area — to sanity-check that the response is
   monotonic between the endpoints rather than a two-value cliff.
4. **Direct paddy samples.** Repeat the sampling against dry paddy and
   deliberately wetted paddy. These are *observations*, not calibration
   points (see caveat below).
5. **Power-cycle repeat.** Re-run the dry-air baseline after a full power
   cycle and compare to step 1. Large drift means warm-up time or supply
   sensitivity must be characterized before trusting any single reading.

### Multi-unit variance check

If more than one unit of a candidate model is purchased, repeat steps 1–2
**per unit** and compare the dry/wet endpoints. Large unit-to-unit spread
is the data that would justify upgrading `MoistureCalibrationConfig` from
one shared struct to a per-channel array in `PinConfig.h`; near-identical
endpoints justify keeping the shared struct.

### Grain vs soil caveat

Capacitive soil-moisture sensors are designed and calibrated for **soil
dielectric behavior** — a granular, compacted medium with characteristic
bulk density and air-gap structure. Loose paddy grain differs in both, so:

- The dry-air / water endpoints are a **necessary reference frame** — they
  confirm the sensor responds at all and bracket its dynamic range.
- They **must not be assumed to map linearly onto grain-moisture
  percentage.** Real calibration needs validation against known-moisture
  reference paddy samples (e.g. oven-dry method) — a later, deliberate
  calibration exercise, not a bench afternoon.

## Recording template

Fill in with real bench numbers; one table per model under evaluation.

| Model | Unit # | Condition | Raw ADC avg | Noise (max−min) | Voltage (V) | Notes |
|---|---|---|---|---|---|---|
| ENGLAB capacitive (candidate) | 1 | dry air | 2648.3 | 96 | 2.134 | n=41, 20s window, GPIO6 |
| ENGLAB capacitive (candidate) | 1 | water immersion (just submerged) | 872.8 | 499 | 0.703 | n=41, 20s window -- transient, see next row |
| ENGLAB capacitive (candidate) | 1 | water immersion (settled +20s) | 707.2 | 59 | 0.570 | n=41, 20s window -- steady-state reading; noise fell 499->59 once settled. Response direction: wetter reads LOWER raw/voltage. Implies readAll() will need a settle/dwell delay after any state change, not just an instant sample. |
| *(TBD)* | 1 | damp towel (intermediate) | | | | |
| *(TBD)* | 1 | dry paddy | | | | |
| *(TBD)* | 1 | wetted paddy | | | | |
| *(TBD)* | 1 | dry air, after power cycle | | | | drift vs row 1 |

## What this procedure does NOT decide

**Final `MoistureCalibrationConfig` values** (`rawDryValue`,
`rawWetValue`, and any grain-specific reference points in
`include/config/PinConfig.h`). Those get filled in only once a specific
sensor is chosen, purchased, **and wired for real** — matching this
project's standing no-invented-numbers rule. Bench data informs the
*choice* of sensor; it does not pre-fill production firmware.