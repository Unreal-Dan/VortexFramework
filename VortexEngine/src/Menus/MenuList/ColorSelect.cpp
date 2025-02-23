#include "ColorSelect.h"

#include "../../Time/TimeControl.h"
#include "../../Colors/Colorset.h"
#include "../../Buttons/Button.h"
#include "../../Time/Timings.h"
#include "../../Menus/Menus.h"
#include "../../Modes/Modes.h"
#include "../../Modes/Mode.h"
#include "../../Leds/Leds.h"
#include "../../Log/Log.h"

// the number of slots in a page
#define PAGE_SIZE 4
// the number of pages
#define NUM_PAGES 2

ColorSelect::ColorSelect(const RGBColor &col, bool advanced) :
  Menu(col, advanced),
  m_state(STATE_PICK_SLOT),
  m_curPage(0),
  m_slot(0),
  m_newColor()
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
    // this is separate from the init function because the target led
    // hasn't been chosen yet at the time of the init function running
    // where as this will run after the target led has been chosen and
    // we can fetch the correct colorset to work with
    m_newColor = HSVColor(0, 255, 255);
    m_curSelection = FINGER_FIRST;
    m_curPage = 0;
    m_slot = 0;
    // grab the colorset from our selected target led
    if (m_targetLeds == MAP_LED_ALL) {
      m_colorset = Modes::curMode()->getColorset();
    } else {
      m_colorset = Modes::curMode()->getColorset(ledmapGetFirstLed(m_targetLeds));
    }
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

  // blink whichever slot is currently selected regardless of state
  blinkSelection();

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

void ColorSelect::onShortClick()
{
  // keep track of pages when in slot selection
  if (m_state == STATE_PICK_SLOT) {
    // if the current selection is on the index finger then it's at the
    // end of the current page and we might need to go to the next page
    if ((m_curSelection == FINGER_INDEX && (m_curPage == 0 && m_colorset.numColors() > 3))) {
      // increase the page number to 1
      m_curPage = (m_curPage + 1) % NUM_PAGES;
      // skip past the thumb if we're on index
      m_curSelection = FINGER_THUMB;
      // clear all leds because we went to the next page
      Leds::clearAll();
    } else if (m_curSelection == FINGER_THUMB && (m_curPage == 1 || (m_curPage == 0 && m_colorset.numColors() <= 3))) {
      m_curPage = 0;
      // skip past the thumb if we're on index
      m_curSelection = FINGER_THUMB;
      // clear all leds because we went to the next page
      Leds::clearAll();
    }
  }
  // iterate selection forward and wrap after the thumb
  m_curSelection = (Finger)((m_curSelection + 1) % (FINGER_THUMB + 1));
  // only when we're not on thumb calculate the current 'slot' based on page
  if (m_curSelection != FINGER_THUMB && m_state == STATE_PICK_SLOT) {
    // the slot is an index in the colorset, where as curselection is a finger index
    m_slot = (uint32_t)m_curSelection + (m_curPage * PAGE_SIZE);
  }
  if (m_slot > m_colorset.numColors()) {
    if (m_curSelection != FINGER_THUMB) {
      m_curSelection = FINGER_THUMB;
    } else {
      m_curPage = m_slot = 0;
      m_curSelection = FINGER_FIRST;
    }
  }
}

void ColorSelect::onLongClick()
{
  bool needsSave = false;
  // if we're exiting a menu
  if (m_curSelection == FINGER_THUMB) {
    Mode *cur = Modes::curMode();
    // leaving a menu, clear everything
    Leds::clearAll();
    switch (m_state) {
    case STATE_PICK_SLOT:
    default:
      // need to save if the colorset is not equal
      needsSave = !m_colorset.equals(cur->getColorset());
      // if we need to save, then actually update the colorset
      if (needsSave) {
        // save the colorset
        cur->setColorsetMap(m_targetLeds, m_colorset);
        cur->init();
      }
      // leave menu and save if we made changes
      leaveMenu(needsSave);
      return;
    case STATE_PICK_HUE1:
      m_state = STATE_PICK_SLOT;
      // reset selection and page based on chosen slot
      m_curSelection = (Finger)(m_slot % PAGE_SIZE);
      m_curPage = m_slot / PAGE_SIZE;
      return;
    case STATE_PICK_HUE2:
    case STATE_PICK_SAT:
    case STATE_PICK_VAL:
      m_state = (ColorSelectState)(m_state - 1);
      m_curSelection = FINGER_THUMB;
      return;
    }
  }
  switch (m_state) {
  case STATE_PICK_SLOT:
  default:
    if (m_slot < m_colorset.numColors() &&
        g_pButton->holdDuration() >= DELETE_THRESHOLD_TICKS &&
       (g_pButton->holdDuration() % (DELETE_CYCLE_TICKS * 2)) > (DELETE_CYCLE_TICKS)) {
      // delete current slot
      m_colorset.removeColor(m_slot);
      if (m_slot > m_colorset.numColors()) {
        m_slot--;
      }
      m_curSelection = (Finger)(m_slot % PAGE_SIZE);
      m_curPage = m_slot / PAGE_SIZE;
      return;
    }
    // the page is only used for slot selection so reset current page
    // for next time they use the color select
    m_curPage = 0;
    break;
  case STATE_PICK_HUE1:
    // pick a hue1
    m_newColor.hue = m_curSelection * (255 / 4);
    break;
  case STATE_PICK_HUE2:
    // pick a hue2
    m_newColor.hue += m_curSelection * (255 / 16);
    break;
  case STATE_PICK_SAT:
    // pick a saturation
    m_newColor.sat = sats[m_curSelection];
    break;
  case STATE_PICK_VAL:
    // pick a value
    m_newColor.val = vals[m_curSelection];
    // replace the slot with the new color, this might
    // result in adding a new color if the slot is at
    // hightest color index + 1
    m_colorset.set(m_slot, m_newColor);
    // go back to beginning for next time
    m_state = STATE_PICK_SLOT;
    // reset the color
    m_newColor.clear();
    // go back to the slot we just added
    m_curSelection = (Finger)(m_slot % PAGE_SIZE);
    m_curPage = m_slot / PAGE_SIZE;

    // return instead of break so the cur selection isn't reset
    return;
  }
  // reset selection after choosing anything
  m_state = (ColorSelectState)(m_state + 1);
  m_curSelection = FINGER_FIRST;
}

void ColorSelect::showSlotSelection()
{
  // the index of the first color to show changes based on the page
  // will be either 0 or 4 for the two page color select
  uint32_t idx = (m_curPage * PAGE_SIZE);
  for (Finger f = FINGER_PINKIE; f <= FINGER_INDEX; ++f) {
    // set the current colorset slot color on the current finger
    // display the extra slots as solid blank
    Leds::setFinger(f, (idx >= m_colorset.numColors()) ? RGB_WHITE0 : m_colorset[idx]);
    idx++;
  }
}

void ColorSelect::showSelection(ColorSelectState mode)
{
  for (Finger f = FINGER_PINKIE; f <= FINGER_INDEX; ++f) {
    HSVColor color;
    switch (mode) {
    case STATE_PICK_HUE1:
      if (f == FINGER_PINKIE) {
        for (LedPos p = PINKIE_TIP; p <= INDEX_TOP; ++p) {
          Leds::setIndex(p, HSVColor((256 / 8) * p, 255, 255));
        }
      }
      return;
    case STATE_PICK_HUE2:
      color = HSVColor(m_newColor.hue + ((255 / 16) * f), 255, 255);
      break;
    case STATE_PICK_SAT:
      color = HSVColor(m_newColor.hue, sats[f], 255);
      break;
    case STATE_PICK_VAL:
      color = HSVColor(m_newColor.hue, m_newColor.sat, vals[f]);
      break;
    default:
      return;
    }
    Leds::setFinger(f, color);
  }
}

void ColorSelect::blinkSelection(uint32_t offMs, uint32_t onMs)
{
  // if we're in the slot selection
  if (m_state == STATE_PICK_SLOT) {
    // and the current selected slot is the end slot, except for when we're on the thumb
    if (m_slot == m_colorset.numColors() && m_curSelection != FINGER_THUMB) {
      // clear the finger so it turns off, then blink this slot to either
      // white or dim white to indicate we can add a color here
      Leds::clearPair(m_curSelection);
      Leds::blinkPair(m_curSelection, 150, 350,
        g_pButton->isPressed() ? RGB_WHITE6 : RGB_WHITE4);
      return;
    } else if (m_slot < m_colorset.numColors() &&
               g_pButton->isPressed() &&
               g_pButton->holdDuration() >= DELETE_THRESHOLD_TICKS) {
      // if we're pressing down on a slot then glow the tip white/red
      if ((g_pButton->holdDuration() % (DELETE_CYCLE_TICKS * 2)) > DELETE_CYCLE_TICKS) {
        // breath red instead of white blink
        Leds::breatheFinger(m_curSelection, 0, g_pButton->holdDuration());
        return;
      }
    }
  }
  // otherwise run the default blink logic
  Menu::blinkSelection(offMs, onMs);
}

bool ColorSelect::isValidLedSelection(LedMap selection) const
{
  // if we have a multi-led pattern then we can only select LED_MULTI otherwise
  // if we don't have a multi-led pattern then we can't select multi
  bool selectedMulti =  (selection == MAP_LED(LED_MULTI));
  return selectedMulti == m_previewMode.isMultiLed();
}
