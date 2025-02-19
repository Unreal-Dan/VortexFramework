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

private:
  // don't worry about this stuff
  enum keychain_mode_state : uint8_t
  {
    // sleeping / fake off
    KEYCHAIN_MODE_STATE_OFF = 0,

    // solid/tracer
    KEYCHAIN_MODE_STATE_SOLID,

    // dops blink 4 / 16
    KEYCHAIN_MODE_STATE_DOPS,

    // signal blink 16 / 120
    KEYCHAIN_MODE_STATE_SIGNAL,

    // total states
    KEYCHAIN_MODE_STATE_COUNT
  };

  keychain_mode_state m_keychain_modeState;
  uint32_t m_lastStateChange;
  uint8_t m_colorIndex;

  void setKeychainModeState(keychain_mode_state newState);
  Menu::MenuAction runKeychainMode();
};

#endif
