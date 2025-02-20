#include "ColorSelect.h"

#include "../../Time/TimeControl.h"
#include "../../Patterns/Pattern.h"
#include "../../Colors/Colorset.h"
#include "../../Buttons/Button.h"
#include "../../Time/Timings.h"
#include "../../Menus/Menus.h"
#include "../../Modes/Modes.h"
#include "../../Modes/Mode.h"
#include "../../Leds/Leds.h"
#include "../../Log/Log.h"

ColorSelect::ColorSelect(const RGBColor &col, bool advanced) :
  Menu(col, advanced),
  m_state(STATE_PICK_SLOT),
  m_newColor(),
  m_colorset(),
  m_targetSlot(0),
  m_targetHue1(0)
{
  // NOTE! Specifically using hsv_to_rgb_rainbow instead of generic because
  // it will generate nicer looking colors and a nicer rainbow to select
  // from than a regular hsv conversion will.  This is because the rainbow
  // function uses a different algorithm to generate the colors that results
  // in a smaller color space with less bright colors. The tradeoff is you lose
  // the bright colors but the rainbow looks a lot better
  g_hsv_rgb_alg = HSV_TO_RGB_RAINBOW;
}

ColorSelect::~ColorSelect()
{
  // revert the hsv to rgb algorithm to normal
  g_hsv_rgb_alg = HSV_TO_RGB_GENERIC;
}

bool ColorSelect::init()
{
  if (!Menu::init()) {
    return false;
  }
  Mode *cur = Modes::curMode();
  if (cur->isEmpty()) {
    // cannot work with an empty mode
    return false;
  }
  if (cur->isMultiLed()) {
    bypassLedSelection(MAP_LED(LED_MULTI));
  }
  m_state = STATE_INIT;
  DEBUG_LOG("Entered color select");
  return true;
}

Menu::MenuAction ColorSelect::run()
{
  MenuAction result = Menu::run();
  if (result != MENU_CONTINUE) {
    return result;
  }

  // all states start with a blank slate
  Leds::clearAll();
  switch (m_state) {
  case STATE_INIT:
    // reset all the selection members
    m_newColor = HSVColor(0, 255, 255);
    m_targetHue1 = 0;
    m_targetSlot = 0;
    // move on to picking slot
    m_state = STATE_PICK_SLOT;
    break;
  case STATE_PICK_SLOT:
    showSlotSelection();
    break;
  case STATE_PICK_HUE1:
  case STATE_PICK_HUE2:
  case STATE_PICK_SAT:
  case STATE_PICK_VAL:
    showSelection(m_state);
    break;
  }
  // show selections
  Menus::showSelection();
  return MENU_CONTINUE;
}

// callback after the user selects the target led
void ColorSelect::onLedSelected()
{
  Mode *cur = Modes::curMode();
  // grab the colorset from our selected target led
  if (m_targetLeds == MAP_LED_ALL) {
    m_colorset = cur->getColorset();
  } else {
    m_colorset = cur->getColorset(ledmapGetFirstLed(m_targetLeds));
  }
}

void ColorSelect::onShortClickM()
{
  // increment selection
  m_curSelection++;
  if (m_state == STATE_PICK_SLOT) {
    m_curSelection %= (m_colorset.numColors() + 1 + (m_colorset.numColors() < MAX_COLOR_SLOTS));
  } else {
    m_curSelection %= 5;
  }
}

void ColorSelect::onLongClickM()
{
  // if we're on 'exit' and we're on any menu past the slot selection
  if (m_curSelection == 4 && m_state > STATE_PICK_SLOT) {
    // move back to the previous selection
    m_state = (ColorSelectState)(m_state - 1);
    // if we're back to the slot selection then set our position back to the
    // slot we selected, otherwise it's more visually appealing to just return
    // to the first selection on the previous menu
    m_curSelection = (m_state == STATE_PICK_SLOT) ? m_targetSlot : 0;
    return;
  }
  // reuse these variables lots
  uint8_t numColors = m_colorset.numColors();
  uint32_t holdDur = g_pButtonM->holdDuration();
  switch (m_state) {
  case STATE_INIT:
    // nothing
    return;
  case STATE_PICK_SLOT:
    // if the exit is selected then save and exit, this depends on the number
    // of colors in the colorset. If the colorset is full then the exit is
    // just the number of colors (8) but if it's not full then the exit is the
    // number of colors + 1. Example: with 4 cols, cols are on 0, 1, 2, 3,
    // add-color is 4, and exit is 5
    if (m_curSelection == numColors + (numColors < MAX_COLOR_SLOTS)) {
      Mode *cur = Modes::curMode();
      cur->setColorsetMap(m_targetLeds, m_colorset);
      cur->init();
      leaveMenu(true);
      return;
    }
    // handle if user releases during the delete option
    if (m_curSelection < numColors &&
        holdDur >= DELETE_THRESHOLD_TICKS &&
        (holdDur % (DELETE_CYCLE_TICKS * 2)) > (DELETE_CYCLE_TICKS)) {
      // delete current slot
      m_colorset.removeColor(m_curSelection);
      if (m_curSelection > numColors) {
        m_curSelection--;
      }
      return;
    }
    // otherwise store the target slot continue onto the hue selection
    m_targetSlot = m_curSelection;
    break;
  case STATE_PICK_HUE1:
    m_targetHue1 = m_curSelection;
    m_newColor.hue = m_targetHue1 * (255 / 4);
    break;
  case STATE_PICK_HUE2:
    m_newColor.hue = (m_targetHue1 * (255 / 4)) + m_curSelection * (255 / 16);
    break;
  case STATE_PICK_SAT:
    m_newColor.sat = sats[m_curSelection];
    break;
  case STATE_PICK_VAL:
    // no m_targetVal because you can't go back after this
    m_newColor.val = vals[m_curSelection];
    // specifically using hsv to rgb rainbow to generate the color
    m_colorset.set(m_targetSlot, m_newColor);
    m_curSelection = m_targetSlot;
    m_state = STATE_PICK_SLOT;
    return;
  }
  m_state = (ColorSelectState)(m_state + 1);
  m_curSelection = 0;
}

void ColorSelect::showSlotSelection()
{
  uint8_t exitIndex = m_colorset.numColors();
  uint32_t holdDur = g_pButtonM->holdDuration();
  bool withinNumColors = m_curSelection < exitIndex;
  bool holdDurationCheck = g_pButtonM->isPressed() && holdDur >= DELETE_THRESHOLD_TICKS;
  bool holdDurationModCheck = (holdDur % (DELETE_CYCLE_TICKS * 2)) > DELETE_CYCLE_TICKS;
  const RGBColor &col = m_colorset[m_curSelection];
  if (withinNumColors && holdDurationCheck && holdDurationModCheck) {
    // breath red for delete slot
    Leds::breatheIndex(LED_ALL, 0, holdDur);
  } else if (withinNumColors) {
    if (col.empty()) {
      Leds::setAll(RGB_WHITE0);
    }
    // blink the selected slot color
    Leds::blinkAll(150, 650, col);
  } else if (exitIndex < MAX_COLOR_SLOTS) {
    if (m_curSelection == exitIndex) {
      // blink both leds and blink faster to indicate 'add' new color
      Leds::blinkAll(100, 150, RGB_WHITE2);
    }
    exitIndex++;
  }
  if (m_curSelection == exitIndex) {
    showFullSet(50, 100);
  }
}

void ColorSelect::showSelection(ColorSelectState mode)
{
  if (m_curSelection >= 4) {
    showExit();
    return;
  }
  uint32_t now = Time::getCurtime();
  uint8_t hue = m_newColor.hue;
  uint8_t sat = m_newColor.sat;
  uint8_t val = 255;
  switch (mode) {
  default:
    return;
  case STATE_PICK_HUE1:
    hue = m_curSelection * (255 / 4);
    MAP_FOREACH_LED(MAP_PAIR_EVENS) {
      Leds::breatheIndex(pos, hue, (now / 2), 22, 255, 180);
    }
    MAP_FOREACH_LED(MAP_PAIR_ODDS) {
      Leds::breatheIndex(pos, hue, (now / 2) + 125, 22, 255, 180);
    }
    // force sat at hue level1
    sat = 255;
    // NOTE: return here
    return;
  case STATE_PICK_HUE2:
    hue = m_targetHue1 * (255 / 4) + (m_curSelection * (255 / 16));
    Leds::setIndex(LED_1, RGB_WHITE0);
    // force sat at hue level2
    sat = 255;
    break;
  case STATE_PICK_SAT:
    sat = sats[m_curSelection];
    break;
  case STATE_PICK_VAL:
    val = vals[m_curSelection];
    break;
  }
  Leds::setMap(MAP_PAIR_EVENS, HSVColor(hue, sat, val));
}

void ColorSelect::showFullSet(uint8_t offMs, uint8_t onMs)
{
  uint8_t numCols = m_colorset.numColors();
  uint8_t offOnMs = MS_TO_TICKS(offMs + onMs);
  if (!numCols || !offOnMs) {
    return;
  }
  uint32_t now = Time::getCurtime();
  if ((now % offOnMs) < MS_TO_TICKS(onMs)) {
    Leds::setAll(m_colorset.get((now / offOnMs) % numCols));
  }
}

bool ColorSelect::isValidLedSelection(LedMap selection) const
{
  // if we have a multi-led pattern then we can only select LED_MULTI otherwise
  // if we don't have a multi-led pattern then we can't select multi
  bool selectedMulti =  (selection == MAP_LED(LED_MULTI));
  return selectedMulti == m_previewMode.isMultiLed();
}
