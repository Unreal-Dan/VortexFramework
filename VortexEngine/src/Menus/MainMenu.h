#ifndef MAINMENU_H
#define MAINMENU_H

#include <inttypes.h>

class MainMenu
{
public:
  static bool init();
  static bool run();
  static void show();

  // open the main menu
  static void open();
  static void close();
  static bool isOpen();
  static void pressLeft();
  static void pressRight();
  static void select();

private:
  static bool m_isOpen;
  static uint8_t m_curSelection;
};

#endif
