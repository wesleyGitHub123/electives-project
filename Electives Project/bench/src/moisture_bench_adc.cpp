#include <Arduino.h>

// Set to whatever ADC-capable GPIO the candidate sensor's AOUT is jumpered
// to right now. Deliberately NOT sourced from include/config/PinConfig.h --
// this tool predates any wiring decision. See docs/BENCH_VALIDATION.md.
constexpr int kBenchAdcPin = 6; // matches current breadboard wiring (AOUT -> GPIO6)

constexpr float kAdcMaxCount = 4095.0f; // 12-bit
constexpr float kAdcRefVoltage = 3.3f;

void setup() {
  Serial.begin(115200);
  delay(500);
  analogReadResolution(12);
  analogSetPinAttenuation(kBenchAdcPin, ADC_11db);
  Serial.println("[bench][moisture-adc] ready");
  Serial.println("raw,voltage");
}

void loop() {
  const int raw = analogRead(kBenchAdcPin);
  const float voltage = (raw / kAdcMaxCount) * kAdcRefVoltage;
  Serial.print(raw);
  Serial.print(",");
  Serial.println(voltage, 3);
  delay(500);
}