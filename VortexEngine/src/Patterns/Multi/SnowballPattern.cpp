#include "SnowballPattern.h"

#include "../../Leds/Leds.h"

#define WORM_SIZE LED_COUNT / 3

SnowballPattern::SnowballPattern(const PatternArgs &args) :
  BlinkStepPattern(args),
  m_progress()
{
  m_patternID = PATTERN_SNOWBALL;
  setArgs(args);
}

SnowballPattern::~SnowballPattern()
{
}

// init the pattern to initial state
void SnowballPattern::init()
{
  BlinkStepPattern::init();

  // start at index 1
  m_colorset.setCurIndex(1);
}

void SnowballPattern::blinkOn()
{
  Leds::setAll(m_colorset.get(0));
  for (int body = 0; body < WORM_SIZE; ++body) {
    if (body + m_progress < LED_COUNT) {
      Leds::setIndex((LedPos)(body + m_progress), m_colorset.cur());
    } else {
      RGBColor col = m_colorset.peekNext();
      if (m_colorset.curIndex() == m_colorset.numColors() - 1) {
        col = m_colorset.peek(2);
      }
      Leds::setIndex((LedPos)((body + m_progress) % LED_COUNT), col);
    }
  }
}

void SnowballPattern::poststep()
{
  m_colorset.skip();
  if (m_colorset.curIndex() == 0) {
    m_progress = (m_progress + 1) % LED_COUNT;
    m_colorset.skip();
  }
}
