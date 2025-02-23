#include "Menu.h"

#include "../Time/TimeControl.h"
#include "../Time/Timings.h"
#include "../Buttons/Button.h"
#include "../Menus/Menus.h"
#include "../Modes/Modes.h"
#include "../Modes/Mode.h"
#include "../Leds/Leds.h"
#include "../Log/Log.h"

// this is an array of possible LED maps for LED selection
static LedMap ledPermutations[] = {
  MAP_LED_ALL,
  MAP_LED(LED_MULTI),
  MAP_PAIR_EVENS,
  MAP_PAIR_ODDS,
  MAP_ODD_FINGERS,
  MAP_EVEN_FINGERS,
  MAP_FINGER(FINGER_THUMB),
  MAP_FINGER(FINGER_INDEX),
  MAP_FINGER(FINGER_MIDDLE),
  MAP_FINGER(FINGER_RING),
  MAP_FINGER(FINGER_PINKIE),
  MAP_LED(LED_9),
  MAP_LED(LED_8),
  MAP_LED(LED_7),
  MAP_LED(LED_6),
  MAP_LED(LED_5),
  MAP_LED(LED_4),
  MAP_LED(LED_3),
  MAP_LED(LED_2),
  MAP_LED(LED_1),
  MAP_LED(LED_0)
};

#define NUM_PERMUTATIONS (sizeof(ledPermutations)/ sizeof(ledPermutations[0]))

Menu::Menu(const RGBColor &col, bool advanced) :
  m_previewMode(),
  m_menuColor(col),
  m_targetLeds(MAP_LED_ALL),
  m_ledSelection(0),
  m_curSelection(FINGER_FIRST),
  m_ledSelected(false),
  m_advanced(advanced),
  m_shouldClose(false)
{
}

Menu::~Menu()
{
}

bool Menu::init()
{
  // menu is initialized before being run
  if (!Modes::curMode()) {
    // if you enter a menu and there's no modes, it will add an empty one
    if (Modes::numModes() > 0) {
      // some kind of serious error
      return false;
    }
    if (!Modes::addMode(PATTERN_STROBE, RGBColor(RGB_OFF))) {
      // some kind of serious error
      return false;
    }
    if (!Modes::curMode()) {
      // serious error again
      return false;
    }
  }
  // reset the current selection
  m_curSelection = FINGER_FIRST;
  // copy the current mode into the demo mode and initialize it
  m_previewMode = *Modes::curMode();
  m_previewMode.init();
  // just in case
  m_shouldClose = false;
  return true;
}

Menu::MenuAction Menu::run()
{
  // check whether the close flag has been set
  if (m_shouldClose) {
    // reset flag and close
    m_shouldClose = false;
    return MENU_QUIT;
  }

  // if the led has already been selected continue as normal
  // and allow derived menu logic to perform work
  if (m_ledSelected) {
    return MENU_CONTINUE;
  }

  // using manual onShortClick and onLongClick check here because
  // there is no guarantee the child class will call the parent
  // class's onShortClick and onLongClick functions so

  // every time the button is clicked, change the target led
  if (g_pButton->onShortClick()) {
    do {
      m_ledSelection = (m_ledSelection + 1) % NUM_PERMUTATIONS;
    } while (!isValidLedSelection(ledPermutations[m_ledSelection]));
  }
  // on a long press of the button, lock in the target led
  if (g_pButton->onLongClick()) {
    m_targetLeds = ledPermutations[m_ledSelection];
    m_ledSelected = true;
    // call led selected callback
    onLedSelected();
    // log about the selection
    DEBUG_LOGF("Led Selected: 0x%x (%s)", m_targetLeds,
      (m_targetLeds == MAP_LED(LED_MULTI)) ? "multi"
        : (m_targetLeds == MAP_LED_ALL) ? "all"
          : "some singles");
  }

  // render the bulb selection
  showBulbSelection();

  // return MENU_SKIP to indicate to derived menu classes that
  // they should skip their logic and just return so that the
  // bulb selection can be rendered
  return MENU_SKIP;
}

void Menu::showBulbSelection()
{
  Leds::clearAll();
  if (ledPermutations[m_ledSelection] == MAP_LED(LED_MULTI)) {
    LedPos pos = (LedPos)((Time::getCurtime() / 30) % LED_COUNT);
    Leds::blinkIndexOffset(pos, pos * 10, 50, 500, RGB_MAGENTA1);
  } else {
    Leds::blinkMap(ledPermutations[m_ledSelection], BULB_SELECT_OFF_MS, BULB_SELECT_ON_MS, RGB_MAGENTA1);
  }
  // blink when selecting
  Menus::showSelection(RGB_MAGENTA1);
}

void Menu::showExit()
{
  Leds::setIndex(THUMB_TIP, RGB_RED0);
  Leds::setIndex(THUMB_TOP, RGB_WHITE0);
}

void Menu::bypassLedSelection(LedMap map)
{
  m_ledSelected = true;
  m_targetLeds = map;
  onLedSelected();
}

void Menu::onLedSelected()
{
}

void Menu::onShortClick()
{
}

void Menu::onLongClick()
{
  leaveMenu(false);
}

void Menu::leaveMenu(bool doSave)
{
  m_shouldClose = true;
  if (doSave) {
    Modes::saveStorage();
  }
}

void Menu::blinkSelection(uint32_t offMs, uint32_t onMs)
{
  uint32_t blinkCol = RGB_OFF;
  if (g_pButton->isPressed() && g_pButton->holdDuration() > SHORT_CLICK_THRESHOLD_TICKS) {
    // blink green if long pressing on a selection
    blinkCol = RGB_WHITE6;
  }
  switch (m_curSelection) {
  case FINGER_THUMB:
    // exit thumb breathes red on the tip and is either blank or red on the top
    // depending on whether you've held for the short click threshold or not
    Leds::breatheIndex(THUMB_TIP, 250, (uint32_t)(Time::getCurtime() / 2), 10, 255, 180);
    if (g_pButton->isPressed() && g_pButton->holdDuration() > SHORT_CLICK_THRESHOLD_TICKS) {
      Leds::setIndex(THUMB_TOP, RGB_RED);
    } else {
      Leds::clearIndex(THUMB_TOP);
      Leds::blinkIndex(THUMB_TOP, 250, 500, RGB_WHITE0);
    }
    break;
  case FINGER_COUNT:
    // special selection clause 'select all' do nothing
    break;
  default:
    // otherwise just blink the selected finger to off from whatever
    // color or pattern it's currently displaying
    if (blinkCol == RGB_OFF && Leds::getLed(fingerTip(m_curSelection)).empty()) {
      // if the blink color is 'off' and the led is a blank then we
      // need to blink to a different color
      blinkCol = RGB_WHITE0;
    }
    // blink the target finger to the target color
    uint32_t blinkDur = g_pButton->isPressed() ? g_pButton->holdDuration() : Time::getCurtime();
    Leds::blinkFingerOffset(m_curSelection, blinkDur, offMs, onMs, blinkCol);
    break;
  }
}
