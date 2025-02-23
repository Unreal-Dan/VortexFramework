#ifndef MENU_H
#define MENU_H

#include <inttypes.h>

#include "../Colors/ColorTypes.h"
#include "../Leds/LedTypes.h"
#include "../Modes/Mode.h"

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

  // optional handler for initializing data after user selected led
  virtual void onLedSelected();

  // optional handlers for clicks
  virtual void onShortClickL();
  virtual void onShortClickM();
  virtual void onShortClickR();
  virtual void onLongClickL();
  virtual void onLongClickM();
  virtual void onLongClickR();

  // close the current menu
  virtual void leaveMenu(bool doSave = false);

protected:
  void showBulbSelection();
  void showExit();

  // iterate to next bulb selection
  void nextBulbSelection();
  void prevBulbSelection();

  // skip led selection
  void bypassLedSelection(LedMap map);
  // an overridable api that allows derived menus to decide which led selections
  // should be available before they have actually opened
  virtual bool isValidLedSelection(LedMap selection) const { return true; }

  // the mode copied from the current mode used to preview changes
  Mode m_previewMode;
  // the color of this menu
  RGBColor m_menuColor;
  // tracks the targetted leds for this menu
  // note this is an led map
  LedMap m_targetLeds;
  // current index of led maps
  uint16_t m_ledSelection;
  // all menus have a 'current selection'
  uint8_t m_curSelection;
  // true once a an led is selected
  bool m_ledSelected;
  // whether advanced menu was activated
  bool m_advanced;

private:
  // internal flag to close the menu
  bool m_shouldClose;
  // add to the current selection of leds
  void addSelectionMask();

#ifdef VORTEX_LIB
  friend class Vortex;
#endif
};

#endif
