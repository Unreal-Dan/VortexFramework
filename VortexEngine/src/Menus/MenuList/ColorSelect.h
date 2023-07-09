#ifndef COLOR_SELECT_H
#define COLOR_SELECT_H

#include "../Menu.h"

#include "../../Colors/Colorset.h"

class Pattern;

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
  enum ColorSelectState
  {
    STATE_INIT,
    STATE_PICK_SLOT,
    STATE_PICK_HUE1,
    STATE_PICK_HUE2,
    STATE_PICK_SAT,
    STATE_PICK_VAL
  };

  void showSlotSelection();
  void showSelection(ColorSelectState mode);
  void showFullSet(uint8_t offMs, uint8_t onMs);

  // the options for saturations
  const uint8_t sats[4] = {
    255,
    170,
    85,
    0
  };

  // the options for values
  const uint8_t vals[4] = {
    255,
    170,
    85,
    0
  };

  // the state of the color select menu
  ColorSelectState m_state;
  // the new color being built via hue, sat then val
  HSVColor m_newColor;
  // A copy of the colorset being changed
  Colorset m_colorset;

  // below are the selection indexes for each level

  // the target values selected at each level to build the color, the value
  // selected at the last level isn't stored because you can't go back after
  uint8_t m_targetSlot;
  uint8_t m_targetHue1;

  // adv
  Pattern *m_pattern;
};

#endif
