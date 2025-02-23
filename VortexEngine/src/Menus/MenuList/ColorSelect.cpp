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
  m_quadrant(0),
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
    //m_newColor.clear();
    m_newColor = HSVColor(0, 255, 255);
    m_curPage = 0;
    m_slot = 0;
    m_quadrant = 0;
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

void ColorSelect::onShortClick()
{
  // keep track of pages when in slot selection
  if (m_state == STATE_PICK_SLOT) {
    // if the current selection is on the index finger then it's at the
    // end of the current page and we might need to go to the next page
    if ((m_curSelection == QUADRANT_4 && (m_curPage == 0 && m_colorset.numColors() > 3))) {
      // increase the page number to 1
      m_curPage = (m_curPage + 1) % NUM_PAGES;
      // skip past the thumb if we're on index
      m_curSelection = QUADRANT_LAST;
      // clear all leds because we went to the next page
      Leds::clearAll();
    } else if (m_curSelection == QUADRANT_LAST && (m_curPage == 1 || (m_curPage == 0 && m_colorset.numColors() <= 3))) {
      m_curPage = 0;
      // skip past the thumb if we're on index
      m_curSelection = QUADRANT_LAST;
      // clear all leds because we went to the next page
      Leds::clearAll();
    }
  }
  // iterate selection forward and wrap after the thumb
  m_curSelection = (Quadrant)((m_curSelection + 1) % (QUADRANT_LAST + 1));
  // only when we're not on thumb calculate the current 'slot' based on page
  if (m_curSelection != QUADRANT_LAST && m_state == STATE_PICK_SLOT) {
    // the slot is an index in the colorset, where as curselection is a finger index
    m_slot = (uint32_t)m_curSelection + (m_curPage * PAGE_SIZE);
  }
  if (m_slot > m_colorset.numColors()) {
    if (m_curSelection != QUADRANT_LAST) {
      m_curSelection = QUADRANT_LAST;
    } else {
      m_curPage = m_slot = 0;
      m_curSelection = QUADRANT_FIRST;
    }
  }
}

void ColorSelect::onShortClick2()
{
  // keep track of pages when in slot selection
  if (m_state == STATE_PICK_SLOT) {
    // if the current selection is on the index finger then it's at the
    // end of the current page and we might need to go to the next page
    if (m_curSelection == QUADRANT_1 && m_curPage > 0) {
      // increase the page number to 1
      if (!m_curPage) {
        m_curPage = NUM_PAGES - 1;
      } else {
        --m_curPage;
      }
      // skip past the
      m_curSelection = QUADRANT_5;
      // clear all leds because we went to the next page
      Leds::clearAll();
    } else if (m_curSelection == QUADRANT_1 && (m_curPage > 0 || (m_curPage == 0 && m_colorset.numColors() > 3))) {
      if (!m_curPage) {
        m_curPage = NUM_PAGES - 1;
      } else {
        --m_curPage;
      }
      // skip past the thumb if we're on index
      m_curSelection = QUADRANT_5;
      // clear all leds because we went to the next page
      Leds::clearAll();
      // if we are moving backwards from the first slot in a 4 color set
    } else if (m_curSelection == QUADRANT_5) {
      m_curSelection = (Quadrant)((m_colorset.numColors() + 1) % PAGE_SIZE);
      Leds::clearAll();
    }
  }
  // iterate selection backward and wrap after the thumb
  if (!m_curSelection) {
    if (!m_curPage) {
      m_curSelection = QUADRANT_5;
    } else {
      m_curSelection = QUADRANT_LAST;
    }
  } else {
    m_curSelection = m_curSelection - 1;
  }
  if (m_curSelection != QUADRANT_5 && m_state == STATE_PICK_SLOT) {
    // the slot is an index in the colorset, where as curselection is a finger index
    m_slot = (uint32_t)m_curSelection + (m_curPage * PAGE_SIZE);
  }
  if (m_slot > m_colorset.numColors()) {
    if (m_curSelection != QUADRANT_5) {
      m_curSelection = QUADRANT_5;
    } else {
      m_curPage = m_slot = 0;
      m_curSelection = QUADRANT_FIRST;
    }
  }
}

void ColorSelect::onLongClick()
{
  bool needsSave = false;
  // if we're exiting a menu
  if (m_curSelection == QUADRANT_LAST) {
    Mode *cur = Modes::curMode();
    // leaving a menu, clear everything
    Leds::clearAll();
    switch (m_state) {
    case STATE_PICK_SLOT:
    default:
      // if we're targetting more than one led then screw
      // checking if the colorset has changed because it's
      // not worth the effort
      needsSave = (!MAP_IS_ONE_LED(m_targetLeds) ||
        !m_colorset.equals(cur->getColorset(ledmapGetFirstLed(m_targetLeds))));
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
      m_curSelection = (Quadrant)((m_slot % PAGE_SIZE));
      m_curPage = m_slot / PAGE_SIZE;
      return;
    case STATE_PICK_HUE2:
      // save the quadrant of hue that was selected so that if
      // they navigate backwards we know where to place them
      m_curSelection = (Quadrant)m_quadrant;
      m_state = (ColorSelectState)(m_state - 1);
      return;
    case STATE_PICK_SAT:
    case STATE_PICK_VAL:
      m_state = (ColorSelectState)(m_state - 1);
      m_curSelection = QUADRANT_FIRST;
      return;
    }
  }
  switch (m_state) {
  default:
    break;
  case STATE_PICK_SLOT:
    if (m_slot < m_colorset.numColors() &&
      g_pButton->holdDuration() >= DELETE_THRESHOLD_TICKS &&
      (g_pButton->holdDuration() % (DELETE_CYCLE_TICKS * 2)) >(DELETE_CYCLE_TICKS)) {
      // delete current slot
      m_colorset.removeColor(m_slot);
      if (m_slot > m_colorset.numColors()) {
        m_slot--;
      }
      m_curSelection = (Quadrant)((m_slot % PAGE_SIZE));
      m_curPage = m_slot / PAGE_SIZE;
      return;
    }
    m_state = STATE_PICK_HUE1;
    // the page is only used for slot selection so reset current page
    // for next time they use the color select
    m_curPage = 0;
    m_quadrant = 0;
    break;
  case STATE_PICK_HUE1:
    // save the quadrant of hue that was selected so that if
    // they navigate backwards we know where to place them
    m_quadrant = m_curSelection;
    // pick a hue1
    m_newColor.hue = m_quadrant * (256 / 4);
    m_state = STATE_PICK_HUE2;
    break;
  case STATE_PICK_HUE2:
    // pick a hue2
    m_newColor.hue = (m_quadrant * (256 / 4)) + (m_curSelection * (256 / 16));
    m_state = STATE_PICK_SAT;
    break;
  case STATE_PICK_SAT:
    // pick a saturation
    m_newColor.sat = sats[m_curSelection];
    m_state = STATE_PICK_VAL;
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
    m_curSelection = (Quadrant)((m_slot % PAGE_SIZE));
    m_curPage = m_slot / PAGE_SIZE;

    // return instead of break so the cur selection isn't reset
    return;
  }
  // reset selection after choosing anything
  m_curSelection = QUADRANT_FIRST;
}

void ColorSelect::onLongClick2()
{
  // leaving a menu, clear everything
  Leds::clearAll();
  switch (m_state) {
  case STATE_PICK_SLOT:
  default:
    // leave menu without saving
    leaveMenu();
    return;
  case STATE_PICK_HUE1:
    m_state = STATE_PICK_SLOT;
    // reset selection and page based on chosen slot
    m_curSelection = (Quadrant)((m_slot % PAGE_SIZE));
    m_curPage = m_slot / PAGE_SIZE;
    return;
  case STATE_PICK_HUE2:
  case STATE_PICK_SAT:
  case STATE_PICK_VAL:
    m_state = (ColorSelectState)(m_state - 1);
    return;
  }
}

void ColorSelect::showSlotSelection()
{
  // the index of the first color to show changes based on the page
  // will be either 0 or 4 for the two page color select
  uint32_t idx = (m_curPage * PAGE_SIZE);
  for (Quadrant f = QUADRANT_FIRST; f <= QUADRANT_4; ++f) {
    // set the current colorset slot color on the current finger
    // display the extra slots as solid blank
    Leds::setQuadrant(f, (idx >= m_colorset.numColors()) ? RGB_WHITE0 : m_colorset[idx]);
    idx++;
  }
}

void ColorSelect::showSelection(ColorSelectState mode)
{
  for (Quadrant f = QUADRANT_FIRST; f <= QUADRANT_4; ++f) {
    HSVColor color;
    switch (mode) {
    case STATE_PICK_HUE1:
      if (f != QUADRANT_FIRST) {
        return;
      }
      for (Pair p = PAIR_FIRST; p < PAIR_COUNT; ++p) {
        Leds::setPair(p, HSVColor((256 / PAIR_COUNT) * p, 255, 255));
      }
      return;
    case STATE_PICK_HUE2:
      // calculate the hue with the stored 'quadrant' variable because
      // they may have gotten here by going back from the next state (pick sat)
      // and the m_newColor.hue value may have been irreversibly changed
      color = HSVColor((m_quadrant * (256 / 4)) + ((256 / 16) * f), 255, 255);
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
    Leds::setQuadrant(f, color);
  }
  if (m_curSelection >= 4) {
    showExit();
  }
}

void ColorSelect::blinkSelection(uint32_t offMs, uint32_t onMs)
{
  // if we're in the slot selection
  if (m_state == STATE_PICK_SLOT) {
    // and the current selected slot is the end slot, except for when we're on the thumb
    if (m_slot == m_colorset.numColors() && m_curSelection != QUADRANT_LAST) {
      // clear the finger so it turns off, then blink this slot to either
      // white or dim white to indicate we can add a color here
      Leds::clearQuadrant(m_curSelection);
      Leds::blinkQuadrant(m_curSelection, 150, 350,
        g_pButton->isPressed() ? RGB_WHITE6 : RGB_WHITE4);
      return;
    } else if (m_slot < m_colorset.numColors() &&
      g_pButton->isPressed() &&
      g_pButton->holdDuration() >= DELETE_THRESHOLD_TICKS) {
      // if we're pressing down on a slot then glow the tip white/red
      if ((g_pButton->holdDuration() % (DELETE_CYCLE_TICKS * 2)) > DELETE_CYCLE_TICKS) {
        // breath red instead of white blink
        Leds::breatheQuadrant(m_curSelection, 0, g_pButton->holdDuration());
        return;
      }
    } else if (m_slot == (uint32_t)(m_colorset.numColors() + 1)) {
      Leds::blinkQuadrant(QUADRANT_5, 150, 350, RGB_WHITE);
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
