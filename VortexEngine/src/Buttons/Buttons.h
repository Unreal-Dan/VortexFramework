#ifndef BUTTONS_H
#define BUTTONS_H

#include "Button.h"

// the number of buttons, this isn't in VortexConfig.h because
// changing it won't really work without updating other things
// like which pins the buttons are attached to. So this is more
// of a hardcoded constant than a configuration setting
#define NUM_BUTTONS 1

class VortexEngine;

class Buttons
{
public:
  Buttons(VortexEngine &engine);
  ~Buttons();

  // initialize all buttons
  bool init();
  void cleanup();

  // poll the buttons
  void update();

  // get a button
  Button &button(uint8_t index = 0) { return m_buttons[index]; }

  uint8_t numButtons() { return NUM_BUTTONS; }

private:
  // reference to engine
  VortexEngine &m_engine;

  // feel free to add more I guess
  Button m_buttons[NUM_BUTTONS];
};

#endif
