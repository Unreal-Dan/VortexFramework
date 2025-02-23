#include "FillPattern.h"

#include "../../Serial/ByteStream.h"
#include "../../Time/TimeControl.h"
#include "../../Leds/Leds.h"
#include "../../Log/Log.h"

FillPattern::FillPattern(const PatternArgs &args) :
  BlinkStepPattern(args),
  m_progress(0)
{
  m_patternID = PATTERN_FILL;
  setArgs(args);
}

FillPattern::~FillPattern()
{
}

void FillPattern::init()
{
  BlinkStepPattern::init();
  // reset progress
  m_progress = 0;
  // start colorset at index 0 so cur() works
  m_colorset.setCurIndex(0);
}

void FillPattern::blinkOn()
{
  LedPos edgePos = (LedPos)(m_progress * (LED_COUNT / 4));
  Leds::setRange(LED_FIRST, edgePos, m_colorset.peekNext());
  Leds::setRange(edgePos, LED_LAST, m_colorset.cur());
}

void FillPattern::poststep()
{
  m_progress = (m_progress + 1) % 4;
  if (m_progress == 0) {
    m_colorset.getNext();
  }
}
