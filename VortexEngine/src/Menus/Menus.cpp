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
uint32_t Menus::m_openTime = 0;
uint8_t Menus::m_selection = 0;
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
  ENTRY(Randomizer,         RGB_LOW_WHITE),  // 0 (dim white)
  ENTRY(ModeSharing,        RGB_LOW_TEAL),   // 5 (dim teal)
  ENTRY(EditorConnection,   RGB_LOW_PURPLE), // 6 (dim purple)
  ENTRY(ColorSelect,        RGB_LOW_GREEN),  // 1 (dim green)
  ENTRY(PatternSelect,      RGB_LOW_BLUE),   // 2 (dim blue)
  ENTRY(GlobalBrightness,   RGB_LOW_YELLOW), // 3 (dim yellow)
  ENTRY(FactoryReset,       RGB_LOW_RED),    // 4 (dim red)
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
  if (g_pButton->onShortClick()) {
    // otherwise increment selection and wrap around at num menus
    m_selection = (m_selection + 1) % NUM_MENUS;
    DEBUG_LOGF("Cyling to ring menu %u", m_selection);
    // reset the open time so that it starts again
    m_openTime = Time::getCurtime();
    // clear the leds
    Leds::clearAll();
    return true;
  }
  if (g_pButton2->onShortClick()) {
    // increment down the menu list and wrap at 0
    if (!m_selection) {
      m_selection = MENU_COUNT - 1;
    } else {
      --m_selection;
    }
    Leds::clearAll();
    return true;
  }
  // clear the leds so it always fills instead of replacing
  Leds::clearAll();
  // if the button was long pressed then select this menu, but we
  // need to check the presstime to ensure we don't catch the initial
  // release after opening the ringmenu
  if (g_pButton->pressTime() >= m_openTime) {
    if (g_pButton->onLongClick()) {
      // ringmenu is open so select the menu
      DEBUG_LOGF("Selected ringmenu %s", menuList[m_selection].menuName);
      // open the menu we have selected
      if (!openMenu(m_selection, g_pButton->holdDuration() > ADV_MENU_DURATION_TICKS)) {
        DEBUG_LOGF("Failed to initialize %s menu", menuList[m_selection].menuName);
        return false;
      }
      // display the newly opened menu
      return true;
    }
    // if you long click the 2nd button exit the menu list
    if (g_pButton2->onLongClick()) {
      // close the current menu when run returns false
      closeCurMenu();
      // return false to let the modes play
      return false;
    }
    // if holding down to select the menu option
    if (g_pButton->isPressed() && g_pButton->holdDuration() > ADV_MENU_DURATION_TICKS) {
      Leds::setAll(RGB_DARK_ORANGE);
    }
  }
  // blink every even/odd of every pair
  for (Pair p = PAIR_FIRST; p < PAIR_COUNT; ++p) {
    if (pairEven(p) < LED_COUNT) {
      Leds::blinkIndex(pairEven(p), Time::getCurtime(), 200, 200, menuList[m_selection].color);
    }
    if (pairOdd(p) < LED_COUNT) {
      Leds::setIndex(pairOdd(p), menuList[m_selection].color);
      Leds::blinkIndex(pairOdd(p), Time::getCurtime(), 200, 200, RGB_OFF);
    }
  }
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
    if (g_pButton->onShortClick()) {
      m_pCurMenu->onShortClick();
    }
    if (g_pButton->onLongClick()) {
      m_pCurMenu->onLongClick();
    }
    if (g_pButton2->onShortClick()) {
      m_pCurMenu->onShortClick2();
    }
    if (g_pButton2->onLongClick()) {
      m_pCurMenu->onLongClick2();
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

bool Menus::checkOpen()
{
  return m_menuState != MENU_STATE_NOT_OPEN && g_pButton->releaseTime() > m_openTime;
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
