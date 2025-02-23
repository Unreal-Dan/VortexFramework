#ifndef GLOBAL_BRIGHTNESS_H
#define GLOBAL_BRIGHTNESS_H

#include "../Menu.h"

class GlobalBrightness : public Menu
{
public:
  GlobalBrightness(VortexEngine &engine, const RGBColor &col, bool advanced);
  ~GlobalBrightness();

  bool init() override;
  MenuAction run() override;

  // handlers for clicks
  void onShortClick() override;
  void onLongClick() override;

private:
  void showBrightnessSelection();

  // the list of brightness options
  const uint8_t m_brightnessOptions[4] = {
    BRIGHTNESS_OPTION_1,
    BRIGHTNESS_OPTION_2,
    BRIGHTNESS_OPTION_3,
    BRIGHTNESS_OPTION_4
  };
};

#endif
