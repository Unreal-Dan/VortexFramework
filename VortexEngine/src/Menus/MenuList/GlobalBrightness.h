#ifndef GLOBAL_BRIGHTNESS_H
#define GLOBAL_BRIGHTNESS_H

#include "../Menu.h"

class GlobalBrightness : public Menu
{
public:
  GlobalBrightness(const RGBColor &col, bool advanced);
  ~GlobalBrightness();

  bool init() override;
  MenuAction run() override;

  // handlers for clicks
  void onShortClickL() override;
  void onShortClickR() override;
  void onShortClickM() override;
  void onLongClickM() override;

private:
  void showBrightnessSelection();

  // the list of brightness options
  const uint8_t m_brightnessOptions[8] = {
    BRIGHTNESS_OPTION_1,
    BRIGHTNESS_OPTION_2,
    BRIGHTNESS_OPTION_3,
    BRIGHTNESS_OPTION_4,
    BRIGHTNESS_OPTION_5,
    BRIGHTNESS_OPTION_6,
    BRIGHTNESS_OPTION_7,
    BRIGHTNESS_OPTION_8,
  };
};

#endif
