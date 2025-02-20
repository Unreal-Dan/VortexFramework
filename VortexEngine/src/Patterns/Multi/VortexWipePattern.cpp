#include "VortexWipePattern.h"

#include "../../Serial/ByteStream.h"
#include "../../Time/TimeControl.h"
#include "../../Leds/Leds.h"
#include "../../Log/Log.h"

#define VORTEX_WIPE_STEPS (LED_COUNT/2)

VortexWipePattern::VortexWipePattern(const PatternArgs &args) :
  BlinkStepPattern(args),
  m_progress(0)
{
  m_patternID = PATTERN_VORTEXWIPE;
  setArgs(args);
}

VortexWipePattern::~VortexWipePattern()
{
}

// init the pattern to initial state
void VortexWipePattern::init()
{
  BlinkStepPattern::init();
  // reset progress
  m_progress = 0;
  // start colorset at index 0 so cur() works
  m_colorset.setCurIndex(0);
}

void VortexWipePattern::blinkOn()
{
  Leds::setRange(LED_FIRST, (LedPos)m_progress, m_colorset.peekNext());
  Leds::setRange((LedPos)(LED_COUNT/2), (LedPos)((LED_COUNT/2) + m_progress), m_colorset.peekNext());
  Leds::setRange((LedPos)m_progress, (LedPos)((LED_COUNT/2) - 1), m_colorset.cur());  
  Leds::setRange((LedPos)((LED_COUNT / 2) + m_progress), (LedPos)(LED_COUNT - 1), m_colorset.cur());
}

void VortexWipePattern::poststep()
{
  m_progress = (m_progress + 1) % VORTEX_WIPE_STEPS;
  if (m_progress == 0) {
    m_colorset.getNext();
  }
}
