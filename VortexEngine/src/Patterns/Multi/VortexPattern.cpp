#include "VortexPattern.h"

#include "../../Serial/ByteStream.h"
#include "../../Time/TimeControl.h"
#include "../../Leds/Leds.h"
#include "../../Log/Log.h"

// add 1 to prevent the middle point from being led 0
#define MIDDLE_POINT ((LED_COUNT + 1) / 2)

VortexPattern::VortexPattern(const PatternArgs& args) :
  BlinkStepPattern(args),
  m_progress(0)
{
  m_patternID = PATTERN_VORTEXWIPE;
  setArgs(args);
}

VortexPattern::~VortexPattern()
{
}

// init the pattern to initial state
void VortexPattern::init()
{
  BlinkStepPattern::init();
  // reset progress
  m_progress = 0;
  // start colorset at index 0 so cur() works
  m_colorset.setCurIndex(0);
}

void VortexPattern::blinkOn()
{
  // Sets an LED at opposite ends of the strip and progresses towards the center
  if (MIDDLE_POINT + m_progress != LED_COUNT) {
    Leds::setIndex((LedPos)(MIDDLE_POINT + m_progress), m_colorset.cur());
  }
  Leds::setIndex((LedPos)(MIDDLE_POINT - m_progress), m_colorset.cur());
}

void VortexPattern::poststep()
{
  // step till the middle point
  m_progress = (m_progress + 1) % (MIDDLE_POINT + 1);
  // each cycle progress to the next color
  if (m_progress == 0) {
    m_colorset.getNext();
  }
}
