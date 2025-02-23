#include "Buttons.h"

#ifdef VORTEX_LIB
#include "VortexLib.h"
#else
#include "../Time/TimeControl.h"
#include "../Time/Timings.h"
#endif

Buttons::Buttons(VortexEngine &engine) :
  m_engine(engine),
  m_buttons{ engine }
{
}

Buttons::~Buttons()
{
}

bool Buttons::init()
{
  // initialize the button on pin 1
  if (!m_buttons[0].init(1)) {
    return false;
  }
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
  // read input from the vortex lib interface, for example m_engine.vortexLib().shortClick()
  m_engine.vortexLib().handleInputQueue(m_buttons, NUM_BUTTONS);
#endif
}
