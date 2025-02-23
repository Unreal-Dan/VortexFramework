#include "VortexWipePattern.h"

#include "../../VortexEngine.h"

#include "../../Serial/ByteStream.h"
#include "../../Time/TimeControl.h"
#include "../../Leds/Leds.h"
#include "../../Log/Log.h"

const LedPos VortexWipePattern::ledStepPositions[] = {
  LED_9,
  LED_7,
  LED_5,
  LED_3,
  LED_1,

  LED_0,
  LED_2,
  LED_4,
  LED_6,
  LED_8
};

VortexWipePattern::VortexWipePattern(VortexEngine &engine, const PatternArgs &args) :
  BlinkStepPattern(engine, args),
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
    m_engine.leds().setIndex(ledStepPositions[index], m_colorset.peekNext());
  }
  for (int index = m_progress; index < LED_COUNT; ++index) {
    m_engine.leds().setIndex(ledStepPositions[index], m_colorset.cur());
  }
}

void VortexWipePattern::poststep()
{
  m_progress = (m_progress + 1) % LED_COUNT;
  if (m_progress == 0) {
    m_colorset.getNext();
  }
}
