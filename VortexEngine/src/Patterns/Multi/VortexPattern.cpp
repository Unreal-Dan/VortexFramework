#include "VortexPattern.h"

#include "../../VortexEngine.h"

#include "../../Serial/ByteStream.h"
#include "../../Time/TimeControl.h"
#include "../../Leds/Leds.h"
#include "../../Log/Log.h"

// add 1 to prevent the middle point from being led 0
#define MIDDLE_POINT ((LED_COUNT + 1) / 2)

VortexPattern::VortexPattern(VortexEngine &engine, const PatternArgs& args) :
  BlinkStepPattern(engine, args),
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
  m_engine.leds().setIndex((LedPos)m_progress, m_colorset.peekNext());
  m_engine.leds().setIndex((LedPos)(LED_LAST - m_progress), m_colorset.peekNext());
}

void VortexPattern::poststep()
{
  // step till the middle point
  m_progress = (m_progress + 1) % MIDDLE_POINT;
  // each cycle progress to the next color
  if (m_progress == 0) {
    m_colorset.getNext();
  }
}
