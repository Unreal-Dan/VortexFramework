#include "Buttons.h"

#ifdef VORTEX_LIB
#include "VortexLib.h"
#else
#include "../Time/TimeControl.h"
#include "../Time/Timings.h"
#endif

// Since there is only one button I am just going to expose a global pointer to
// access it, instead of making the Button class static in case a second button
// is added. This makes it easier to access the button from other places while
// still allowing for a second instance to be added.  I wish there was a more
// elegant way to make the button accessible but not global.
// This will simply point at Buttons::m_button.
Button *g_pButton = nullptr;

bool Buttons::init()
{
  g_pButton = new Button();
  if (!g_pButton) {
    return false;
  }
  // init the button on pin 1
  return g_pButton->init(1);
}

void Buttons::cleanup()
{
  delete g_pButton;
}

void Buttons::update()
{
  g_pButton->update();
#ifdef VORTEX_LIB
  // read input from the vortex lib interface, for example Vortex::shortClick()
  Vortex::handleInputQueue(g_pButton, NUM_BUTTONS);
#endif
}
