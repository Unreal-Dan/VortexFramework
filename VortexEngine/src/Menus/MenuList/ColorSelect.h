#ifndef COLOR_SELECT_H
#define COLOR_SELECT_H

#include "../Menu.h"

#include "../../Colors/Colorset.h"

class ColorSelect : public Menu
{
public:
  ColorSelect(const RGBColor &col, bool advanced);
  ~ColorSelect();

  bool init() override;
  MenuAction run() override;

  // callback after the user selects the target led
  void onLedSelected() override;

  // handlers for clicks
  void onShortClick() override;
  void onLongClick() override;

private:
  // overridden blink logic for the colorselect menu (Controls how m_curSelection blinks)
  void blinkSelection(uint32_t offMs = 350, uint32_t onMs = 500) override;

  // override the led selection api to choose which led maps can be selected
  bool isValidLedSelection(LedMap selection) const override;

  // private enumeration for internal state of color selection
  enum ColorSelectState : uint32_t
  {
    STATE_INIT,

    // currently picking the color slot to change
    STATE_PICK_SLOT,

    // first pick a quadrant 0, 90, 180, 240
    STATE_PICK_HUE1,

    // next pick a quadrant within that quadrant 0, 25, 50, 70
    STATE_PICK_HUE2,

    // picking a saturation for the color
    STATE_PICK_SAT,

    // picking a value for the color
    STATE_PICK_VAL,
  };

  // internal routines for the color select
  void showSlotSelection();
  void showSelection(ColorSelectState mode);

  // the options for saturations
  const uint8_t sats[4] = {
    SAT_OPTION_4,
    SAT_OPTION_3,
    SAT_OPTION_2,
    SAT_OPTION_1
  };

  // the options for values
  const uint8_t vals[4] = {
    VAL_OPTION_4,
    VAL_OPTION_3,
    VAL_OPTION_2,
    VAL_OPTION_1
  };

  // the current state of the color selection menu
  ColorSelectState m_state;

  // A copy of the colorset from the current mode
  Colorset m_colorset;

  // the colorselect has multiple pages
  uint32_t m_curPage;

  // the target color slot to change
  uint32_t m_slot;

  // the new color to set
  HSVColor m_newColor;
};

#endif
