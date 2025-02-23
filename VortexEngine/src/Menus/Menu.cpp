#include "Menu.h"

#include "../VortexEngine.h"

#include "../Time/TimeControl.h"
#include "../Time/Timings.h"
#include "../Buttons/Button.h"
#include "../Menus/Menus.h"
#include "../Modes/Modes.h"
#include "../Modes/Mode.h"
#include "../Leds/Leds.h"
#include "../Log/Log.h"

Menu::Menu(VortexEngine &engine, const RGBColor &col, bool advanced) :
  m_engine(engine),
  m_previewMode(engine),
  m_menuColor(col),
  m_targetLeds(MAP_LED_ALL),
  m_curSelection(0),
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
  if (!m_engine.modes().curMode()) {
    // if you enter a menu and there's no modes, it will add an empty one
    if (m_engine.modes().numModes() > 0) {
      // some kind of serious error
      return false;
    }
    if (!m_engine.modes().addMode(PATTERN_STROBE, RGBColor(RGB_OFF))) {
      // some kind of serious error
      return false;
    }
    if (!m_engine.modes().curMode()) {
      // serious error again
      return false;
    }
  }
  // copy the current mode into the demo mode and initialize it
  m_previewMode = *m_engine.modes().curMode();
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
  if (m_engine.button().onShortClick()) {
    do {
      nextBulbSelection();
    } while (!isValidLedSelection(m_targetLeds));
  }
  // on a long press of the button, lock in the target led
  if (m_engine.button().onLongClick()) {
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
  m_engine.leds().clearAll();
  if (m_targetLeds == MAP_LED(LED_MULTI)) {
    LedPos pos = (LedPos)((m_engine.time().getCurtime() / 30) % LED_COUNT);
    m_engine.leds().blinkIndexOffset(pos, pos * 10, 50, 500, RGB_MAGENTA1);
  } else {
    m_engine.leds().blinkMap(m_targetLeds, BULB_SELECT_OFF_MS, BULB_SELECT_ON_MS, RGB_MAGENTA1);
  }
  // blink when selecting
  m_engine.menus().showSelection(RGB_MAGENTA1);
}

void Menu::showExit()
{
  if (m_engine.button().isPressed() && m_engine.button().holdDuration() > SHORT_CLICK_THRESHOLD_TICKS) {
    m_engine.leds().setAll(RGB_RED);
    return;
  }
  m_engine.leds().clearAll();
  m_engine.leds().setAll(RGB_WHITE0);
  m_engine.leds().blinkAll(EXIT_MENU_OFF_MS, EXIT_MENU_ON_MS, RGB_RED0);
}

void Menu::nextBulbSelection()
{
  Mode *cur = m_engine.modes().curMode();
  // The target led can be 0 through LED_COUNT to represent any led or all leds
  // modulo by LED_COUNT + 1 to include LED_COUNT (all) as a target
  if (m_targetLeds == MAP_LED_ALL) {
    if (cur->isMultiLed()) {
      // do not allow multi led to select anything else
      //break;
    }
    m_targetLeds = MAP_LED(LED_FIRST);
  } else if (m_targetLeds == MAP_LED(LED_LAST)) {
    m_targetLeds = MAP_PAIR_EVENS;
  } else if (m_targetLeds == MAP_PAIR_EVENS) {
    m_targetLeds = MAP_PAIR_ODDS;
  } else if (m_targetLeds == MAP_PAIR_ODDS) {
    m_targetLeds = MAP_LED(LED_MULTI);
  } else if (m_targetLeds == MAP_LED(LED_MULTI)) {
    m_targetLeds = MAP_LED_ALL;
  } else {
    // do not allow multi led to select anything else
    if (cur->isMultiLed()) {
      //m_targetLeds = MAP_LED_ALL;
      //break;
    }
    // iterate as normal
    m_targetLeds = MAP_LED(((m_engine.leds().ledmapGetFirstLed(m_targetLeds) + 1) % (LED_COUNT + 1)));
  }
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
    m_engine.modes().saveStorage();
  }
}
