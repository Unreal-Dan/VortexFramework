#ifndef RING_MENU_H
#define RING_MENU_H

#include <inttypes.h>

#include "../Colors/ColorTypes.h"
#include "../Leds/LedTypes.h"
#include "../Modes/Mode.h"

enum MenuEntryID
{
  MENU_NONE = -1,
  MENU_FIRST = 0,

  MENU_RANDOMIZER = MENU_FIRST,
  MENU_MODE_SHARING,
#if ENABLE_EDITOR_CONNECTION == 1
  MENU_EDITOR_CONNECTION,
#endif
  MENU_COLOR_SELECT,
  MENU_PATTERN_SELECT,
  MENU_GLOBAL_BRIGHTNESS,
  MENU_FACTORY_RESET,
  // add new menus here

  // the total number of menus
  MENU_COUNT
};

class VortexEngine;
class Menu;

class Menus
{
public:
  Menus(VortexEngine &engine);
  ~Menus();

  // opting for class here because there should only ever be one
  // Menu control object and I don't like singletons
  bool init();
  void cleanup();

  // Run the menus returns true if the menu remains open, false if closed
  bool run();

  // open the ring menu
  bool openMenuSelection();

  // open a menu by index in the menu table, optionally specify to
  // open the advanced version of the menu (default basic version)
  bool openMenu(uint32_t index, bool advanced = false);

  // if you call this then a blink will be shown when the user has
  // held the button for at least the long click duration
  void showSelection(RGBColor colval = RGB_WHITE5);

  // whether the menus are actually open (The user has let go after opening)
  bool checkOpen();
  // whether the user is actually in a menu
  bool checkInMenu();
  Menu *curMenu();
  MenuEntryID curMenuID();

private:
  // run the currently open menu
  bool runCurMenu();
  // run the ring filling logic
  bool runMenuSelection();
  // helper to calculate the relative hold time for the current menu
  LedPos calcLedPos();
  // close the currently open menu
  void closeCurMenu();

  // =====================
  //  private members

  // reference to engine
  VortexEngine &m_engine;

  enum MenuState {
    // menus aren't open at all
    MENU_STATE_NOT_OPEN = 0,

    // menus are open and the user is choosing a menu
    MENU_STATE_MENU_SELECTION,

    // the menus are open and we opened a menu
    MENU_STATE_IN_MENU
  };

  // the current menus states
  MenuState m_menuState;

  // the ring menu section
  uint8_t m_selection;
  // the time when we first opened the ringmenu
  uint32_t m_openTime;
  // the current sub menu that is open
  Menu *m_pCurMenu;
};

#endif
