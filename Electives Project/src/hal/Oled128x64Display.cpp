#include "hal/Oled128x64Display.h"

#include <Arduino.h>
#include <Wire.h>

namespace paddy {

namespace {

const char* stateName(SystemState state) {
  switch (state) {
    case SystemState::Idle: return "Idle";
    case SystemState::AcquiringBatch: return "Acquiring";
    case SystemState::ExtractingFeatures: return "Extracting";
    case SystemState::Classifying: return "Classifying";
    case SystemState::ReportingResult: return "Reporting";
    case SystemState::DryingActive: return "Drying";
    case SystemState::DryingReSampling: return "ReSampling";
    case SystemState::DryingComplete: return "Dried";
    case SystemState::Fault: return "Fault";
  }
  return "Unknown";
}

const char* statusName(BatchStatus status) {
  switch (status) {
    case BatchStatus::Unknown: return "Unknown";
    case BatchStatus::StoreSafely: return "StoreSafely";
    case BatchStatus::DryMore: return "DryMore";
    case BatchStatus::HighRisk: return "HighRisk";
  }
  return "Unknown";
}

} // namespace

Oled128x64Display::Oled128x64Display(const DisplayPinConfig& pins)
    : pins_(pins) {}

bool Oled128x64Display::begin() {
  // Lenient on purpose (same policy as Ds18b20TemperatureSensor): a missing
  // or miswired OLED must not Fault the whole system -- it just stays blank
  // while everything else keeps working. Serial logging here is the real
  // bring-up feedback channel.
  Wire.begin(pins_.i2cSdaPin, pins_.i2cSclPin);

  Serial.print("[paddy][oled] I2C scan (SDA=");
  Serial.print(pins_.i2cSdaPin);
  Serial.print(" SCL=");
  Serial.print(pins_.i2cSclPin);
  Serial.println("):");
  bool foundAny = false;
  for (uint8_t addr = 1; addr < 127; ++addr) {
    Wire.beginTransmission(addr);
    if (Wire.endTransmission() == 0) {
      Serial.print("[paddy][oled]   found device at 0x");
      Serial.println(addr, HEX);
      foundAny = true;
    }
  }
  if (!foundAny) {
    Serial.println("[paddy][oled]   no I2C devices found (OLED not wired yet?)");
    return true; // lenient: continue without a display
  }

  // U8g2 expects the 8-bit write address (7-bit address shifted left).
  u8g2_.setI2CAddress(pins_.i2cAddress << 1);
  const uint8_t u8g2Ok = u8g2_.begin();
  // Note: over I2C this is a weak signal (u8g2's init sequence is
  // write-only, no readback), but worth logging -- a hard 0 here would at
  // least rule out an allocation/bus-claim failure, distinct from "wrong
  // driver chip accepted the commands but rendered nothing".
  Serial.print("[paddy][oled] u8g2.begin() returned ");
  Serial.println(u8g2Ok);
  u8g2_.clearBuffer();
  return true;
}

void Oled128x64Display::renderTwoLines(const char* line1, const char* line2) {
  u8g2_.clearBuffer();
  u8g2_.setFont(u8g2_font_10x20_tr);
  u8g2_.drawStr(0, 20, line1);
  u8g2_.setFont(u8g2_font_10x20_tr);
  u8g2_.drawStr(0, 50, line2);
  u8g2_.sendBuffer();
}

void Oled128x64Display::showState(SystemState state) {
  // Idle is the only state reachable while the start switch is off, so the
  // hint line doubles as a visual sanity check of the session gate.
  const char* hint = (state == SystemState::Idle) ? "Switch: waiting"
                                                  : "Switch: ON";
  renderTwoLines(stateName(state), hint);
}

void Oled128x64Display::showResult(BatchStatus status,
                                   const BatchFeatures& features) {
  // Temperature is the sanity-check surface for this bring-up session;
  // moisture/variability rendering is deliberately deferred (stub sensors
  // would just render noise).
  char tempLine[24];
  if (features.valid) {
    snprintf(tempLine, sizeof(tempLine), "Temp: %.1f C",
             static_cast<double>(features.temperatureCelsius));
  } else {
    snprintf(tempLine, sizeof(tempLine), "Temp: --");
  }
  char resultLine[24];
  snprintf(resultLine, sizeof(resultLine), "Result: %s", statusName(status));
  renderTwoLines(resultLine, tempLine);
}

void Oled128x64Display::showFault(const char* message) {
  renderTwoLines("FAULT", message);
}

} // namespace paddy
