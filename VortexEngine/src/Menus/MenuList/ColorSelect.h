#ifndef COLOR_SELECT_H
#define COLOR_SELECT_H

#include "../Menu.h"

#include "../../Colors/Colorset.h"

class ColorSelect : public Menu
{
public:
  ColorSelect(const RGBColor &col);
  ~ColorSelect();

  bool init() override;
  MenuAction run() override;

  // handlers for clicks
  void onShortClick() override;
  void onShortClick2() override;
  void onLongClick() override;
  void onLongClick2() override;

private:
  // overridden blink logic for the colorselect menu (Controls how m_curSelection blinks)
  void blinkSelection(uint32_t offMs = 350, uint32_t onMs = 500) override;

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
  const uint32_t sats[4] = {
    255,
    170,
    85,
    0
  };

  // the options for values
  const uint32_t vals[4] = {
    255,
    170,
    85,
    0
  };

  // the current state of the color selection menu
  ColorSelectState m_state;

  // A copy of the colorset from the current mode
  Colorset m_colorset;

  // the colorselect has multiple pages
  uint32_t m_curPage;

  // the target color slot to change
  uint32_t m_slot;

  // the chosen quadrant
  uint32_t m_quadrant;

  // the new color to set
  HSVColor m_newColor;
};

#endif
