#include "VortexWipePattern.h"

#include "../../Serial/ByteStream.h"
#include "../../Time/TimeControl.h"
#include "../../Leds/Leds.h"
#include "../../Log/Log.h"

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
  for (int index = 0; index < m_progress; ++index) {
    Leds::setRing((Ring)index, m_colorset.peekNext());
  }
  for (int index = m_progress; index < RING_COUNT; ++index) {
    Leds::setRing((Ring)index, m_colorset.cur());
  }
}

void VortexWipePattern::poststep()
{
  m_progress = (m_progress + 1) % RING_COUNT;
  if (m_progress == 0) {
    m_colorset.getNext();
  }
}
