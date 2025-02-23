#include "VortexWipePattern.h"

#include "../../Serial/ByteStream.h"
#include "../../Time/TimeControl.h"
#include "../../Leds/Leds.h"
#include "../../Log/Log.h"

// add 1 to prevent the middle point from being led 0
#define MIDDLE_POINT ((LED_COUNT + 1) / 2)

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
  Leds::setAll(m_colorset.cur());
  if (!m_progress) {
    // none
  }
  if (m_progress) {
    Leds::setRange((LedPos)(MIDDLE_POINT - (m_progress - 1)), (LedPos)(MIDDLE_POINT + (m_progress - 1)), m_colorset.peekNext());
  }
}

void VortexWipePattern::poststep()
{
  // step till the middle point
  m_progress = (m_progress + 1) % (MIDDLE_POINT + 1);
  // each cycle progress to the next color
  if (m_progress == 0) {
    m_colorset.getNext();
  }
}
