#include "Menus.h"

// menus
#include "MenuList/GlobalBrightness.h"
#include "MenuList/EditorConnection.h"
#include "MenuList/FactoryReset.h"
#include "MenuList/ModeSharing.h"
#include "MenuList/ColorSelect.h"
#include "MenuList/PatternSelect.h"
#include "MenuList/Randomizer.h"

#include "../Time/TimeControl.h"
#include "../Time/Timings.h"
#include "../VortexEngine.h"
#include "../Buttons/Button.h"
#include "../Serial/Serial.h"
#include "../Modes/Modes.h"
#include "../Leds/Leds.h"
#include "../Log/Log.h"

// static members
Menus::MenuState Menus::m_menuState = MENU_STATE_NOT_OPEN;
uint8_t Menus::m_selection = 0;
uint32_t Menus::m_openTime = 0;
Menu *Menus::m_pCurMenu = nullptr;

// typedef for the menu initialization function
typedef Menu *(*initMenuFn_t)(const RGBColor &col, bool advanced);

// entries for the ring menu
struct MenuEntry {
#if LOGGING_LEVEL > 2
  // only store the name if the logging is enabled
  const char *menuName;
#endif
  initMenuFn_t initMenu;
  RGBColor color;
};

// a template to initialize ringmenu functions
template <typename T>
Menu *initMenu(const RGBColor &col, bool advanced) { return new T(col, advanced); }

// a simple macro to simplify the entries in the menu list
#if LOGGING_LEVEL > 2
// if the logging is enabled then we need to store the name of the menu
#define ENTRY(classname, color) { #classname, initMenu<classname>, color }
#else
#define ENTRY(classname, color) { initMenu<classname>, color }
#endif

// The list of menus that are registered with colors to show in ring menu
const MenuEntry menuList[] = {
  // =========================
  //  Default menu setup:
  ENTRY(Randomizer, RGB_MENU_RANDOMIZER),
  ENTRY(ModeSharing, RGB_MENU_MODE_SHARING),
#if ENABLE_EDITOR_CONNECTION == 1
  ENTRY(EditorConnection, RGB_MENU_EDITOR_CONNECTION),
#endif
  ENTRY(ColorSelect, RGB_MENU_COLOR_SELECT),
  ENTRY(PatternSelect, RGB_MENU_PATTERN_SELECT),
  ENTRY(GlobalBrightness, RGB_MENU_BRIGHTNESS_SELECT),
  ENTRY(FactoryReset, RGB_MENU_FACTORY_RESET),
};

// the number of menus in the above array
#define NUM_MENUS (sizeof(menuList) / sizeof(menuList[0]))

bool Menus::init()
{
  m_menuState = MENU_STATE_NOT_OPEN;
  // sub-menus are initialized right before they run
  return true;
}

void Menus::cleanup()
{
  if (m_pCurMenu) {
    delete m_pCurMenu;
    m_pCurMenu = nullptr;
  }
  m_menuState = MENU_STATE_NOT_OPEN;
  m_selection = 0;
  m_openTime = 0;
}

bool Menus::run()
{
  // if there are no menus, then we don't need to run
  switch (m_menuState) {
  case MENU_STATE_NOT_OPEN:
  default:
    // nothing to run
    return false;
  case MENU_STATE_MENU_SELECTION:
    return runMenuSelection();
  case MENU_STATE_IN_MENU:
    return runCurMenu();
  }
}

bool Menus::runMenuSelection()
{
  if (g_pButtonR->onShortClick()) {
    // otherwise increment selection and wrap around at num menus
    m_selection = (m_selection + 1) % NUM_MENUS;
#if ENABLE_EDITOR_CONNECTION == 1
    // Hide the editor connection menu because it opens automatically
    // TODO: Create a better way to hide this menu color, this menu
    //       will automatically open when the device is plugged in
    if (m_selection == MENU_EDITOR_CONNECTION) {
      m_selection++;
    }
#endif
    DEBUG_LOGF("Cyling to ring menu %u", m_selection);
    // reset the open time so that it starts again
    m_openTime = Time::getCurtime();
    // clear the leds
    Leds::clearAll();
    return true;
  }
  if (g_pButtonL->onShortClick()) {
    // decrement selection and wrap around at num menus
    m_selection = m_selection ? m_selection - 1 : NUM_MENUS - 1;
#if ENABLE_EDITOR_CONNECTION == 1
    // Hide the editor connection menu because it opens automatically
    // TODO: Create a better way to hide this menu color, this menu
    //       will automatically open when the device is plugged in
    if (m_selection == MENU_EDITOR_CONNECTION) {
      m_selection = MENU_EDITOR_CONNECTION - 1;
    }
#endif
    DEBUG_LOGF("Cyling backwards to ring menu %u", m_selection);
    // reset the open time so that it starts again
    m_openTime = Time::getCurtime();
    // clear the leds
    Leds::clearAll();
    return true;
  }
  // clear the leds so it always fills instead of replacing
  Leds::clearAll();
  // timings for blink later
  uint8_t offtime = 200;
  uint8_t ontime = 200;
  // whether advanced menus are enabled
  bool advMenus = Modes::advancedMenusEnabled();
  // if the button was long pressed then select this menu, but we
  // need to check the presstime to ensure we don't catch the initial
  // release after opening the ringmenu
  if (g_pButtonM->pressTime() >= m_openTime) {
    // whether to open advanced menus or not
    bool openAdv = (g_pButtonM->holdDuration() > ADV_MENU_DURATION_TICKS) && advMenus;
    if (g_pButtonM->onRelease()) {
      // ringmenu is open so select the menu
      DEBUG_LOGF("Selected ringmenu %s", menuList[m_selection].menuName);
      // open the menu we have selected
      if (!openMenu(m_selection, openAdv)) {
        DEBUG_LOGF("Failed to initialize %s menu", menuList[m_selection].menuName);
        return false;
      }
      // display the newly opened menu
      return true;
    }
    // if holding down to select the menu option
    if (g_pButtonM->isPressed() && openAdv) {
      // make it strobe aw yiss
      offtime = HYPERSTROBE_OFF_DURATION;
      ontime = HYPERSTROBE_ON_DURATION;
    }
  }
  // blink every even/odd of every pair
  for (Pair p = PAIR_FIRST; p < PAIR_COUNT; ++p) {
    if (pairEven(p) < LED_COUNT) {
      Leds::blinkIndex(pairEven(p), offtime, ontime, menuList[m_selection].color);
    }
    if (pairOdd(p) < LED_COUNT) {
      Leds::setIndex(pairOdd(p), menuList[m_selection].color);
      Leds::blinkIndex(pairOdd(p), offtime, ontime, RGB_OFF);
    }
  }
  // check if the advanced menus have been enabled
  if (g_pButtonR->onConsecutivePresses(ADVANCED_MENU_CLICKS)) {
    // toggle the advanced menu
    Modes::setAdvancedMenus(!advMenus);
    // display a pink or red depending on whether the menu was enabled
    for (uint16_t i = 0; i < 2; ++i) {
      Leds::holdAll(advMenus ? RGB_RED : RGB_PINK);
    }
  }
  // show when the user selects a menu option
  showSelection(RGBColor(
    menuList[m_selection].color.red << 3,
    menuList[m_selection].color.green << 3,
    menuList[m_selection].color.blue << 3));
  // continue in the menu
  return true;
}

bool Menus::runCurMenu()
{
  // if the menu run handler returns false that signals the
  // menu was closed by the user leaving the menu
  switch (m_pCurMenu->run()) {
  case Menu::MENU_QUIT:
    // close the current menu when run returns false
    closeCurMenu();
    // return false to let the modes play
    return false;
  case Menu::MENU_CONTINUE:
    // if Menu continue run the click handlers for the menu
    if (g_pButtonL->onShortClick()) {
      m_pCurMenu->onShortClickL();
    }
    if (g_pButtonL->onLongClick()) {
      m_pCurMenu->onLongClickL();
    }
    if (g_pButtonM->onShortClick()) {
      m_pCurMenu->onShortClickM();
    }
    if (g_pButtonM->onLongClick()) {
      m_pCurMenu->onLongClickM();
    }
    if (g_pButtonR->onShortClick()) {
      m_pCurMenu->onShortClickR();
    }
    if (g_pButtonR->onLongClick()) {
      m_pCurMenu->onLongClickR();
    }
    break;
  case Menu::MENU_SKIP:
    // dont run click handlers in this case
    // so the core menu class can handle them
    break;
  }
  // the opened menu and don't play modes
  return true;
}

// open the menu selection ring
bool Menus::openMenuSelection()
{
  if (m_menuState != MENU_STATE_NOT_OPEN) {
    return false;
  }
  // save the time of when we open the menu so we can fill based on curtime from then
  m_openTime = Time::getCurtime();
  // open the menu
  m_menuState = MENU_STATE_MENU_SELECTION;
  // clear the leds
  Leds::clearAll();
  return true;
}

bool Menus::openMenu(uint32_t index, bool advanced)
{
  if (index >= NUM_MENUS) {
    return false;
  }
  m_selection = index;
  Menu *newMenu = menuList[m_selection].initMenu(menuList[m_selection].color, advanced);
  if (!newMenu) {
    return false;
  }
  // initialiaze the new menu with the current mode
  if (!newMenu->init()) {
    DEBUG_LOGF("Failed to initialize %s menu", menuList[m_selection].menuName);
    // if the menu failed to init, don't open it
    delete newMenu;
    return false;
  }
  if (m_pCurMenu) {
    delete m_pCurMenu;
  }
  // assign the new menu
  m_pCurMenu = newMenu;
  // and the menus are open just in case
  // clear all the leds
  Leds::clearAll();
  m_menuState = MENU_STATE_IN_MENU;
  return true;
}

void Menus::showSelection(RGBColor colval)
{
  // blink the tip led white for 150ms when the short
  // click threshold has been surpassed
  if (g_pButtonM->isPressed() &&
    g_pButtonM->holdDuration() > SHORT_CLICK_THRESHOLD_TICKS &&
    g_pButtonM->holdDuration() < (SHORT_CLICK_THRESHOLD_TICKS + MS_TO_TICKS(250))) {
    Leds::setAll(colval);
  }
}

bool Menus::checkOpen()
{
  return m_menuState != MENU_STATE_NOT_OPEN && g_pButtonM->releaseTime() > m_openTime;
}

bool Menus::checkInMenu()
{
  return m_menuState == MENU_STATE_IN_MENU;
}

Menu *Menus::curMenu()
{
  return m_pCurMenu;
}

MenuEntryID Menus::curMenuID()
{
  return (MenuEntryID)m_selection;
}

void Menus::closeCurMenu()
{
  // delete the currently open menu object
  if (m_pCurMenu) {
    delete m_pCurMenu;
    m_pCurMenu = nullptr;
  }
  m_menuState = MENU_STATE_NOT_OPEN;
  m_selection = 0;
  // clear the leds
  Leds::clearAll();
}
