#ifndef BUTTONS_H
#define BUTTONS_H

#include "Button.h"

// the number of buttons, this isn't in VortexConfig.h because
// changing it won't really work without updating other things
// like which pins the buttons are attached to. So this is more
// of a hardcoded constant than a configuration setting
#define NUM_BUTTONS 1

class Buttons
{
  // private unimplemented constructor
  Buttons();

public:
  // initialize all buttons
  static bool init();
  static void cleanup();

  // poll the buttons
  static void update();

  static uint8_t numButtons() { return NUM_BUTTONS; }

private:
  // feel free to add more I guess
  static Button m_buttons[NUM_BUTTONS];
};

// Button Mid
extern Button *g_pButton;

#endif
