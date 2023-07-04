#ifndef MENU_H
#define MENU_H

#include <inttypes.h>

#include "../Colors/ColorTypes.h"
#include "../Leds/LedTypes.h"

class Mode;

class Menu
{
public:
  Menu(const RGBColor &col, bool advanced);
  virtual ~Menu();

  virtual bool init();

  // the action for the menu to execute
  enum MenuAction :uint8_t {
    // quit the menus
    MENU_QUIT,
    // continue running the menu
    MENU_CONTINUE,
    // don't run derived menu code, this is used internally
    // by the menu class itself
    MENU_SKIP
  };
  virtual MenuAction run();

  // optional handlers for clicks
  virtual void onShortClick();
  virtual void onLongClick();

  // close the current menu
  virtual void leaveMenu(bool doSave = false);

protected:
  void showBulbSelection();
  void showExit();

  // iterate to next bulb selection
  void nextBulbSelection();

  // the current mode that was selected
  Mode *m_pCurMode;
  // the color of this menu
  RGBColor m_menuColor;
  // tracks the targetted leds for this menu
  // note this is an led map
  LedMap m_targetLeds;
  // all menus have a 'current selection'
  uint8_t m_curSelection;
  // true once a an led is selected
  bool m_ledSelected;
  // whether advanced menu was activated
  bool m_advanced;

private:
  // internal flag to close the menu
  bool m_shouldClose;
};

#endif
