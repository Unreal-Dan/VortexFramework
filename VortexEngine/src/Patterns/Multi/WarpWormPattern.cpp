#include "WarpWormPattern.h"

#include "Patterns/PatternBuilder.h"
#include "../../Serial/ByteStream.h"
#include "../../Time/TimeControl.h"
#include "../../Leds/Leds.h"
#include "../../Log/Log.h"

WarpWormPattern::WarpWormPattern(uint8_t onDuration, uint8_t offDuration, uint8_t stepDuration) :
  BlinkStepPattern(onDuration, offDuration, stepDuration),
  m_progress()
{
  m_patternID = PATTERN_WARPWORM;
}

WarpWormPattern::WarpWormPattern(const PatternArgs &args) :
  WarpWormPattern(args.arg1, args.arg2, args.arg3)
{
}

WarpWormPattern::~WarpWormPattern()
{
}

// init the pattern to initial state
void WarpWormPattern::init()
{
  BlinkStepPattern::init();
  // start colorset at index 0 so cur() works
  m_colorset.setCurIndex(1);
}

void WarpWormPattern::blinkOn()
{
  int wormSize = 6;
  Leds::setAll(m_colorset.get(0));
  for (int body = 0; body < wormSize; ++body) {
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

void WarpWormPattern::poststep()
{
  m_progress = (m_progress + 1) % LED_COUNT;
  if (m_progress == 0) {
    m_colorset.getNext();
    if (m_colorset.curIndex() == 0) {
      m_colorset.getNext();
    }
  }
}
