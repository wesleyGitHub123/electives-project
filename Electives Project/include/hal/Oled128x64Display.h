#pragma once
#include <U8g2lib.h>
#include "interfaces/IDisplay.h"
#include "config/PinConfig.h"

namespace paddy {

// Real 1.3" 128x64 I2C OLED via U8g2 (olikraus/U8g2). Driver chip: SH1106,
// CONFIRMED by the replacement module's listing -- no longer a guess
// against the original, abandoned module (whose never-rendered failure is
// believed to be hardware/connectivity, not a chip mismatch). The
// constructor below has been updated ahead of physical re-testing since
// this is now a known fact, not something to wait on wiring for. If the
// panel still renders blank/garbled once wired, swap the U8G2_...
// constructor below -- a one-line change, nothing else depends on it.
//
// Content scope: state name + switch hint, fault message
// (renderTwoLines, large font); result status + temperature + moisture
// mean (renderThreeLines, compact font). Raw/unitless moisture, same
// convention as the WiFi page (see WifiUiDisplay).
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
