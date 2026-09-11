#pragma once
#include <U8g2lib.h>
#include "interfaces/IDisplay.h"
#include "config/PinConfig.h"

namespace paddy {

// Real 1.3" 128x64 I2C OLED via U8g2 (olikraus/U8g2). Driver chip: SH1106,
// CONFIRMED working on real hardware -- the replacement module renders
// correctly once wiring was fixed (see ARCHITECTURE.md bring-up notes).
//
// Content scope: state name + switch hint, fault message (renderTwoLines);
// result status + temperature + moisture mean (renderThreeLines). Both
// helpers use the same compact font (u8g2_font_7x13_tr) -- the original
// larger font (u8g2_font_10x20_tr) rendered oversized on the actual panel.
// Moisture is raw/unitless, same convention as the WiFi page (see
// WifiUiDisplay).
class Oled128x64Display : public IDisplay {
 public:
  explicit Oled128x64Display(const DisplayPinConfig& pins);

  bool begin() override;
  void showState(SystemState state) override;
  void showResult(BatchStatus status, const BatchFeatures& features) override;
  void showFault(const char* message) override;

 private:
  DisplayPinConfig pins_;
  // U8G2_R0 = no rotation (swap to U8G2_R2 in the constructor call if the
  // panel is mounted upside-down). Default Wire pins come from Wire.begin()
  // in begin(), so the explicit-pin constructor is not used.
  U8G2_SH1106_128X64_NONAME_F_HW_I2C u8g2_{U8G2_R0};

  void renderTwoLines(const char* line1, const char* line2);
  void renderThreeLines(const char* line1, const char* line2, const char* line3);
};

} // namespace paddy
