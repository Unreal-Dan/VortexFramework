#include "Buttons.h"

#ifdef VORTEX_LIB
#include "VortexLib.h"
#else
#include "../Time/TimeControl.h"
#include "../Time/Timings.h"
#endif

// spark button pin
#define BUTTON_PIN 5

// Since there is only one button I am just going to expose a global pointer to
// access it, instead of making the Button class static in case a second button
// is added. This makes it easier to access the button from other places while
// still allowing for a second instance to be added.  I wish there was a more
// elegant way to make the button accessible but not global.
// This will simply point at Buttons::m_button.

// Button
Button *g_pButton = nullptr;

// static members
Button Buttons::m_buttons[NUM_BUTTONS];

bool Buttons::init()
{
  // initialize the button on pins 9/10/11
  if (!m_buttons[0].init(BUTTON_PIN)) {
    return false;
  }
  g_pButton = &m_buttons[0];
  return true;
}

void Buttons::cleanup()
{
}

void Buttons::update()
{
  // would iterate all buttons and check them here
  // but there's only one button so
  for (uint8_t i = 0; i < NUM_BUTTONS; ++i) {
    m_buttons[i].update();
  }
#ifdef VORTEX_LIB
  // read input from the vortex lib interface, for example Vortex::shortClick()
  Vortex::handleInputQueue(m_buttons, NUM_BUTTONS);
#endif
}
