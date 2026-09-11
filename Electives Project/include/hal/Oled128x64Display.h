#pragma once
#include <U8g2lib.h>
#include "interfaces/IDisplay.h"
#include "config/PinConfig.h"

namespace paddy {

// Real 1.3" 128x64 I2C OLED via U8g2 (olikraus/U8g2, supports both the
// SH1106 and SSD1306 driver chips). ASSUMPTION: SSD1306 -- tried SH1106
// first (common for 1.3"/128x64 modules) but the panel stayed blank with a
// confirmed-good I2C link (scan ACKs at 0x3C), which points at a driver
// mismatch rather than wiring; SSD1306 is the next most likely chip. If
// this also renders blank/garbled, swap the U8G2_... constructor below --
// a one-line change, nothing else depends on it.
//
// Deliberately minimal content for the bring-up session (scope): state
// name + switch hint, result status + temperature. No moisture/variability
// UI yet.
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
  U8G2_SSD1306_128X64_NONAME_F_HW_I2C u8g2_{U8G2_R0};

  void renderTwoLines(const char* line1, const char* line2);
};

} // namespace paddy
